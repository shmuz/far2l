#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h> 
#include <ScopeHelpers.h>
#include <TTYRawMode.h>
#include "TTY/TTYRevive.h"

#include "Backend.h"
#include "ConsoleOutput.h"
#include "ConsoleInput.h"
#include "WinPortHandle.h"
#include "PathHelpers.h"
#include "../sudo/sudo_askpass_ipc.h"
#include "SudoAskpassImpl.h"

#include <memory>

ConsoleOutput g_winport_con_out;
ConsoleInput g_winport_con_in;

#ifndef NOWX
bool WinPortMainWX(int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);
#endif

bool WinPortMainTTY(int std_in, int std_out, bool far2l_tty, int argc, char **argv, int(*AppMain)(int argc, char **argv), int *result);

extern "C" void WinPortInitRegistry();

class FScope
{
	FILE *_f;
public:
	FScope(const FScope &) = delete;
	inline FScope &operator = (const FScope &) = delete;

	FScope(FILE *f) : _f(f) {}
	~FScope()
	{
		if (_f)
			fclose(_f);
	}

	operator FILE *() const
	{
		return _f;
	}
};



static std::string GetStdPath(const char *env1, const char *env2)
{
	const char *path = getenv(env1);
	if (!path)
		path = getenv(env2);
	if (path) {
		std::string s;
		if (*path == '~') {
			const char *home = getenv("HOME");
			s = home ? home : "/tmp";
			s+= path + 1;
		} else
			s = path;

		return s;
	}

	return DEVNULL;
}

static void SetupStdHandles()
{
	const std::string &out = GetStdPath("FAR2L_STDOUT", "FAR2L_STD");
	const std::string &err = GetStdPath("FAR2L_STDERR", "FAR2L_STD");
	unsigned char reopened = 0;
	if (!out.empty() && out != "-") {
		if (freopen(out.c_str(), "a", stdout)) {
			reopened|= 1;
		} else
			perror("freopen stdout");
	}
	if (!err.empty() && err != "-") {
		if (freopen(err.c_str(), "a", stderr)) {
			reopened|= 2;
		} else
			perror("freopen stderr");
	}
	
	if ( reopened == 3 && out == DEVNULL && err == DEVNULL) {
		if (!freopen(DEVNULL, "r", stdin)) {
			perror("freopen stdin");
		}
#if !defined(__CYGWIN__) //TODO
		ioctl(0, TIOCNOTTY, NULL);
#endif
	}
}


static bool WriteStr2TC(int fd, const char *str)
{
	for (size_t len = strlen(str), ofs = 0; ofs < len; ) {
		ssize_t written = write(fd, str + ofs, len - ofs);
		if (written <= 0)
			return false;

		ofs+= (size_t) written;
	}
	tcdrain(fd);
	return true;
}

static bool NegotiateFar2lTTY(int fdin, int fdout, bool enable)
{
	// far2l supports both BEL and ST APC finalizers, however screen supports only ST,
	// so use it as most compatible
	if (!WriteStr2TC(fdout, enable ? "\x1b_far2l1\x1b\\\x1b[5n" : "\x1b_far2l0\x07\x1b[5n"))
		return false;

	if (enable) {
		if (!WriteStr2TC(fdout, "Press <ENTER> if tired watching this message"))
			return false;
	}

	std::string s;
	for (bool status_replied = false; !status_replied;) {
		fd_set fds, fde;
		FD_ZERO(&fds);
		FD_ZERO(&fde);
		FD_SET(fdin, &fds); 
		FD_SET(fdin, &fde); 
		struct timeval tv = {10, 0};

		if (select(fdin + 1, &fds, nullptr, &fde, &tv) <= 0) {
			break;
		}

		char c = 0;
		if (read(fdin, &c, 1) <= 0)
			break;
		s+= c;
		if (s.find("\x1b\x1b") != std::string::npos // ..if terminal doesnt report status and user in panic
		|| s.find_first_of("\r\n") != std::string::npos) {
			break;
		}

		// status reply typically looks like "\x1b[0n"
		// but parse anything that is "\x1b[???n" where ??? is empty or sequence of nubers with ;
		for (size_t p = s.find("\x1b["); p != std::string::npos; p = s.find("\x1b[", p + 1)) {
			size_t e = s.find('n', p + 1);
			if (e != std::string::npos) {
				status_replied = true;
				for (size_t i = p + 2; i < e; ++i) {
					if (!isdigit(s[i]) && s[i] != ';') {
						status_replied = false;
					}
				}
				if (status_replied) break;
			}
		}
	}

	if (enable) { // erase stuff printed for detection
		if (!WriteStr2TC(fdout, "\x1b[2K\r\n"))
			perror("write");
	}

	return (s.find("\x1b_far2lok\x07") != std::string::npos);
}

class NormalizeTerminalState
{
	int _std_in, _std_out;
	bool _far2l_tty;
	std::unique_ptr<TTYRawMode> &_tty_raw_mode;
	bool _raw;
	bool _far2l_tty_actual;

public:
	NormalizeTerminalState(int std_in, int std_out, bool far2l_tty, std::unique_ptr<TTYRawMode> &tty_raw_mode)
		: _std_in(std_in), _std_out(std_out),
		_far2l_tty(far2l_tty),
		_tty_raw_mode(tty_raw_mode),
		_raw(tty_raw_mode),
		_far2l_tty_actual(far2l_tty)
	{
		Apply();
	}

	~NormalizeTerminalState()
	{
		Revert();
	}

	void Apply()
	{
		if (_far2l_tty && _far2l_tty_actual) {
			NegotiateFar2lTTY(_std_in, _std_out, false);
			_far2l_tty_actual = false;
		}
		_tty_raw_mode.reset();
	}

	void Revert()
	{
		if (_raw && !_tty_raw_mode) {
			_tty_raw_mode.reset(new TTYRawMode(_std_out));
		}

		if (_far2l_tty && !_far2l_tty_actual) {
			_far2l_tty_actual = NegotiateFar2lTTY(_std_in, _std_out, true);
		}
	}
};

static bool TTYTryReviveSome(int std_in, int std_out, bool far2l_tty, std::unique_ptr<TTYRawMode> &tty_raw_mode)
{
	for (;;) {
		std::vector<TTYRevivableInstance> instances;
		TTYRevivableEnum(instances);
		if (instances.empty()) {
			return false;
		}

		NormalizeTerminalState nts(std_in, std_out, far2l_tty, tty_raw_mode);
		FScope f_out(fdopen(dup(std_out), "w"));

		fprintf(f_out, "\nSome far2l-s lost in space-time nearby:\n");
		for (size_t i = 0; i < instances.size(); ++i) {
			fprintf(f_out, " %lu: %s\n", i, instances[i].info.c_str());
		}

		fprintf(f_out, "Input instance index to revive or empty string to spawn new far2l\n");

		char buf[32] = {};
		{
			FScope f_in(fdopen(dup(std_in), "r"));
			if (!fgets(buf, sizeof(buf) - 1, f_in)) {
				return false;
			}
		}
		if (buf[0] == 0 || buf[0] == '\r' || buf[0] == '\n') {
			return false;
		}

		size_t index = atoi(buf);
		if (buf[0] < '0' || buf[0] > '9' || index >= instances.size()) {
			fprintf(f_out, "Wrong input\n");
			continue;
		}

		nts.Revert();
		bool r = TTYReviveIt(instances[index], std_in, std_out, far2l_tty);

		if (r) {
			return true;
		}

		nts.Apply();

		fprintf(f_out, "Revival failed\n");
		sleep(1);
	}
}


extern "C" int WinPortMain(int argc, char **argv, int(*AppMain)(int argc, char **argv))
{
	bool tty = false, far2l_tty = false, nodetect = false, help = false, notty = false;
	bool nohup = true;

	for (int i = 0; i < argc; ++i) {

		if (strstr(argv[i], "--mortal") == argv[i]) {
			nohup = false;

		} else if (strstr(argv[i], "--notty") == argv[i]) {
			notty = true;

		} else if (strstr(argv[i], "--tty") == argv[i]) {
			tty = true;

		} else if (strstr(argv[i], "--nodetect") == argv[i]) {
			nodetect = true;

		} else if (strcmp(argv[i], "/?") == 0 || strcmp(argv[i], "--help") == 0){
			help = true;
		}
	}

	FDScope std_in(dup(0));
	FDScope std_out(dup(1));

	fcntl(std_in, F_SETFD, FD_CLOEXEC);
	fcntl(std_out, F_SETFD, FD_CLOEXEC);

	std::unique_ptr<TTYRawMode> tty_raw_mode;
	if (!nodetect) {
		tty_raw_mode.reset(new TTYRawMode(std_out));
		if (tty_raw_mode->Applied()) {
			far2l_tty = NegotiateFar2lTTY(std_in, std_out, true);
			if (!far2l_tty)
				tty_raw_mode.reset();
		}
	}
		
	if (far2l_tty) {
		tty = true;
	}

	if (!help) {
		SetupStdHandles();
		if (nohup) {
			signal(SIGHUP, SIG_IGN);
		}
	}

	WinPortInitRegistry();
	WinPortInitWellKnownEnv();
//      g_winport_con_out.WriteString(L"Hello", 5);

	SudoAskpassImpl askass_impl;
	SudoAskpassServer askpass_srv(&askass_impl);

	int result = -1;
#ifndef NOWX
	if (!tty) {
		if (!WinPortMainWX(argc, argv, AppMain, &result) ) {
			fprintf(stderr, "Cannot use WX backend\n");
			tty = !notty;
		}
	}
#else
	tty = true;
#endif

	if (tty) {
		if (!TTYTryReviveSome(std_in, std_out, far2l_tty, tty_raw_mode)) {
			if (!WinPortMainTTY(std_in, std_out, far2l_tty, argc, argv, AppMain, &result)) {
				fprintf(stderr, "Cannot use TTY backend\n");
			}
		}
		if (far2l_tty) {
			NegotiateFar2lTTY(std_in, std_out, false);
		}
	}

	WinPortHandle_FinalizeApp();

	return result;
}
