/*
main.cpp

Функция main.
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#include <sys/ioctl.h>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <locale.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>

#include "lang.hpp"
#include "keys.hpp"
#include "chgprior.hpp"
#include "colors.hpp"
#include "filepanels.hpp"
#include "panel.hpp"
#include "fileedit.hpp"
#include "fileview.hpp"
#include "exitcode.hpp"
#include "lockscrn.hpp"
#include "hilight.hpp"
#include "manager.hpp"
#include "ctrlobj.hpp"
#include "scrbuf.hpp"
#include "language.hpp"
#include "syslog.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "clipboard.hpp"
#include "pathmix.hpp"
#include "strmix.hpp"
#include "dirmix.hpp"
#include "Bookmarks.hpp"
#include "cmdline.hpp"
#include "console.hpp"
#include "vtshell.h"
#include "execute.hpp"
#include "mix.hpp"
#include "InterThreadCall.hpp"
#include "SafeMMap.hpp"
#include "ConfigRW.hpp"
#include "ConfigOpt.hpp"
#include "udlist.hpp"
#include "message.hpp"

#ifdef DIRECT_RT
int DirectRT=0;
#endif

static void print_help(const char *self)
{
	printf("FAR2M - dual-panel file manager with built-in terminal\n"
		"Usage: %s [switches] [[-cd] apath [[-cd] ppath]]\n\n"

		"where\n"
		"  apath - path to folder or file, or plugin command with prefix\n"
		"          for the active panel\n"
		"  ppath - path to folder or file, or plugin command with prefix\n"
		"          for the passive panel\n\n"

		"The following switches may be used in the command line:\n\n"

		"  -h                         This help\n"
		"  -a                         Disable display of characters with codes\n"
		"                             0 - 31 and 255\n"
		"  -ag                        Disable display of pseudographics with codes > 127\n"
		"  -an                        Disable display of pseudographics characters\n"
		"                             completely\n"
		"  -co                        Load plugins from the cache only\n"
		"  -cd <path>                 Change panel's directory to specified path\n"
		"  -m                         Do not load macros\n"
		"  -ma                        Do not execute auto run macros\n"
		"  -p[<path>]                 Search for \"common\" plugins in the directory\n"
		"                             specified by <path>; several search paths can\n"
		"                             be specified separated by ‘:’\n"
		"  -u <identity> OR\n"
		"  -u </path/name>            Specify separate settings via identity\n"
		"                             or FS location\n"
		"  -v <filename>              View the specified file\n"
		"  -v - <command line>        Execute given command line and open viewer\n"
		"                             with its output\n"
		"  -e[<line>[:<pos>]] [filename]\n"
		"                             Edit the specified file with optional cursor\n"
		"                             position specification or empty new file\n"
		"  -e[<line>[:<pos>]] - <command line>\n"
		"                             Execute given command line and open editor\n"
		"                             with its output\n"
		"  -set:<parameter>=<value>   Override the configuration parameter\n"
		"                             Example: far2m -set:Language.Main=English\n"
		"                                            -set:Screen.Clock=false\n"
		"                                            -set:XLat.Flags=0xff\n"
		"\n",
		self);
	WinPortHelp();
}

static FARString ReconstructCommandLine(int argc, char **argv)
{
	FARString cmd;
	for (;argc; --argc, ++argv) {
		if (*argv) {
			if (!cmd.IsEmpty()) {
				cmd+= L" ";
			}
			std::string arg = *argv;
			QuoteCmdArg(arg);
			cmd+= arg;
		}
	}
	return cmd;
}

static void UpdatePathOptions(const FARString &strDestName, bool IsActivePanel)
{
	FARString *outFolder, *outCurFile;

	// Та панель, которая имеет фокус - активна (начнем по традиции с Левой Панели ;-)
	if (IsActivePanel == (bool)Opt.LeftPanel.Focus) {
		Opt.LeftPanel.Type = FILE_PANEL;  // сменим моду панели
		Opt.LeftPanel.Visible = TRUE;     // и включим ее
		outFolder = &Opt.strLeftFolder;
		outCurFile = &Opt.strLeftCurFile;
	}
	else {
		Opt.RightPanel.Type = FILE_PANEL;
		Opt.RightPanel.Visible = TRUE;
		outFolder = &Opt.strRightFolder;
		outCurFile = &Opt.strRightCurFile;
	}

	auto Attr = apiGetFileAttributes(strDestName);
	if (Attr != INVALID_FILE_ATTRIBUTES) {
		if (Attr & FILE_ATTRIBUTE_DIRECTORY) {
			outCurFile->Clear();
			*outFolder = strDestName;
		}
		else {
			*outCurFile = PointToName(strDestName);
			*outFolder = strDestName;
			CutToSlash(*outFolder, true);
			if (outFolder->IsEmpty())
				*outFolder = L"/";
		}
	}
}


static int MainProcess(
    FARString strEditViewArg,
    const FARString &strDestName1,
    const FARString &strDestName2,
    int StartLine,
    int StartChar)
{
	SCOPED_ACTION(InterThreadCallsDispatcherThread);
	{
		clock_t cl_start = clock();
		SCOPED_ACTION(ChangePriority)(ChangePriority::NORMAL);
		ControlObject CtrlObj;
		uint64_t InitAttributes = 0;
		Console.GetTextAttributes(InitAttributes);
		SetFarColor(COL_COMMANDLINEUSERSCREEN, true);

		if (Opt.OnlyEditorViewerUsed != Options::NOT_ONLY_EDITOR_VIEWER)
		{
			Panel *DummyPanel=new Panel;
			_tran(SysLog(L"create dummy panels"));
			CtrlObj.CreateFilePanels();
			CtrlObj.Cp()->LeftPanel = CtrlObj.Cp()->RightPanel = CtrlObj.Cp()->ActivePanel = DummyPanel;
			CtrlObj.Plugins.LoadPlugins();
			CtrlObj.Macro.LoadMacros(true,false);

			if (Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT
				|| Opt.OnlyEditorViewerUsed == Options::ONLY_VIEWER_ON_CMDOUT)
			{
				strEditViewArg = ExecuteCommandAndGrabItsOutput(strEditViewArg);
			}

			if (Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR
				|| Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT)
			{
				FileEditor *ShellEditor=new FileEditor(strEditViewArg,CP_AUTODETECT,FFILEEDIT_CANNEWFILE|FFILEEDIT_ENABLEF6,StartLine,StartChar);
				_tran(SysLog(L"make shelleditor %p",ShellEditor));

				if (!ShellEditor->GetExitCode())  // ????????????
				{
					FrameManager->ExitMainLoop(false);
				}
			}
			else
			{
				FileViewer *ShellViewer=new FileViewer(strEditViewArg,FALSE);

				if (!ShellViewer->GetExitCode())
				{
					FrameManager->ExitMainLoop(false);
				}

				_tran(SysLog(L"make shellviewer, %p",ShellViewer));
			}

			fprintf(stderr, "STARTUP(E/V): %llu\n", (unsigned long long)(clock() - cl_start) );
			FrameManager->EnterMainLoop();

			if (Opt.OnlyEditorViewerUsed == Options::ONLY_VIEWER_ON_CMDOUT
				|| Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR_ON_CMDOUT)
			{
				unlink(strEditViewArg.GetMB().c_str());
			}

			CtrlObj.Cp()->LeftPanel = CtrlObj.Cp()->RightPanel = CtrlObj.Cp()->ActivePanel = nullptr;
			delete DummyPanel;
			_tran(SysLog(L"editor/viewer closed, delete dummy panels"));
		}
		else
		{
			// воспользуемся тем, что ControlObject::Init() создает панели
			// юзая Opt.*
			if (!strDestName1.IsEmpty())  // активная панель
			{
				UpdatePathOptions(strDestName1, true);

				if (!strDestName2.IsEmpty())  // пассивная панель
					UpdatePathOptions(strDestName2, false);
			}

			// теперь все готово - создаем панели!
			CtrlObj.Init();

			// а теперь "провалимся" в каталог или хост-файл (если получится ;-)
			if (!strDestName1.IsEmpty())  // активная панель
			{
				// Always update pointers as prefixed plugin calls could recreate one or both panels
				auto ActivePanel = [&]() { return CtrlObject->Cp()->ActivePanel; };
				auto AnotherPanel = [&]() { return CtrlObject->Cp()->GetAnotherPanel(ActivePanel()); };

				FARString strCurDir;

				if (!strDestName2.IsEmpty())  // пассивная панель
				{
					AnotherPanel()->GetCurDir(strCurDir);
					FarChDir(strCurDir);

					if (IsPluginPrefixPath(strDestName2))
					{
						AnotherPanel()->SetFocus();
						CtrlObject->CmdLine->ExecString(strDestName2,0);
						AnotherPanel()->SetFocus();
					}
				}

				ActivePanel()->GetCurDir(strCurDir);
				FarChDir(strCurDir);

				if (IsPluginPrefixPath(strDestName1))
				{
					CtrlObject->CmdLine->ExecString(strDestName1,0);
				}

				// !!! ВНИМАНИЕ !!!
				// Сначала редравим пассивную панель, а потом активную!
				AnotherPanel()->Redraw();
				ActivePanel()->Redraw();
			}

			fprintf(stderr, "STARTUP: %llu\n", (unsigned long long)(clock() - cl_start));

			if( Opt.IsFirstStart ) {
				DWORD tweaks = WINPORT(SetConsoleTweaks)(TWEAKS_ONLY_QUERY_SUPPORTED);
				if (tweaks & TWEAK_STATUS_SUPPORT_OSC52CLIP_SET) {
					if (Message(0, 2, // at 1st start always only English and we not need use Msg here
						L"Use OSC52 to set clipboard data (question at first start)",
						L"You can toggle use of OSC52 on/off at any time",
						L"in Menu(F9)->\'Options\"->\"Interface settings\"",
						L"",
						L"Use OSC52 to set clipboard data",
						Msg::Yes,
						Msg::No))
					{
						Opt.OSC52ClipSet = 0;
					} else {
						Opt.OSC52ClipSet = 1;
					}
					ConfigOptSave(false);
				}
			}

			FrameManager->EnterMainLoop();
		}

		// очистим за собой!
		SetScreen(0, 0, ScrX, ScrY, L' ', FarColorToReal(COL_COMMANDLINEUSERSCREEN));
		Console.SetTextAttributes(InitAttributes);
		ScrBuf.ResetShadow();
		ScrBuf.Flush();
		MoveRealCursor(0,0);
	}
	CloseConsole();
	return FarExitCode;
}

static void SetupFarPath(const char *Arg0)
{
	InMyTemp(); // pre-cache in env temp pathes
	InitCurrentDirectory();

	char buf[PATH_MAX + 1];
	ssize_t buf_sz = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (buf_sz > 0 && buf[0] == GOOD_SLASH) {
		buf[buf_sz] = 0;
		g_strFarModuleName = buf;

	} else {
		g_strFarModuleName = LookupExecutable(Arg0);
	}

	FARString dir = g_strFarModuleName; // e.g. /usr/local/bin/far2m
	CutToSlash(dir, true);
	const wchar_t *last_element = PointToName(dir);
	if (last_element && wcscmp(last_element, L"bin") == 0) {
		CutToSlash(dir);
		SetPathTranslationPrefix(dir);
	}
}

static unsigned int gMainThreadID;

int FarAppMain(int argc, char **argv)
{
	exit(10);

	// avoid killing process due to broker terminated unexpectedly
	signal(SIGPIPE, SIG_IGN);

	fprintf(stderr, "argv[0]='%s' g_strFarModuleName='%ls' translation_prefix='%ls' temp='%s' config='%s'\n",
		argv[0], g_strFarModuleName.CPtr(), GetPathTranslationPrefix(), InMyTemp().c_str(), InMyConfig().c_str());

	// make current thread to be same as main one to avoid FARString reference-counter
	// from cloning main strings from current one
	OverrideInterThreadID(gMainThreadID);

	Opt.IsUserAdmin = (geteuid()==0);

	_OT(SysLog(L"[[[[[[[[New Session of FAR]]]]]]]]]"));
	Opt.OnlyEditorViewerUsed = Options::NOT_ONLY_EDITOR_VIEWER;
	FARString strEditViewArg;
	FARString DestNames[2];
	int StartLine=-1,StartChar=-1;
	int CntDestName=0; // количество параметров-имен каталогов

	Opt.strRegRoot = L"Software/Far2";
	CheckForConfigUpgrade();

	// По умолчанию - брать плагины из основного каталога
	Opt.LoadPlug.MainPluginDir=TRUE;
	Opt.LoadPlug.PluginsPersonal=TRUE;
	Opt.LoadPlug.PluginsCacheOnly=FALSE;

	setenv("FARPID", ToDec(getpid()).c_str(), 1);

	g_strFarPath = g_strFarModuleName;

	bool translated = TranslateFarString<TranslateInstallPath_Bin2Share>(g_strFarPath);
	CutToSlash(g_strFarPath, true);
	if (translated) {
		// /usr/bin/something -> /usr/share/far2m
		g_strFarPath.Append("/" APP_BASENAME);
	}

	setenv("FARHOME", g_strFarPath.GetMB().c_str(), 1);

	AddEndSlash(g_strFarPath);

	if (Opt.IsUserAdmin) {
		setenv("FARADMINMODE", "1", 1);
	} else {
		unsetenv("FARADMINMODE");
	}

	// run by symlink in editor mode
	if (strcmp(argv[0], "far2medit") == 0) {
		Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
		if (argc > 1) {
			strEditViewArg = argv[argc - 1];	// use last argument
		}
	}

	// макросы не дисаблим
	Opt.Macro.DisableMacro=0;
	bool bCustomPlugins = false;

	for (int I=1; I<argc; I++)
	{
		if (strncmp(argv[I], "--", 2) == 0) {
			continue; // 2024-Jul-06: --primary-selection, --maximize and --nomaximize may appear here
		}
		std::wstring arg_w = MB2Wide(argv[I]);
		bool switchHandled = false;
		if (arg_w[0]==L'-' && arg_w[1])
		{
			switchHandled = true;
			if (!StrCmpNI(arg_w.c_str() + 1, L"SET:", 4))
			{
				Opt.CmdLineStrings.emplace_back(arg_w.c_str() + 5);
				continue;
			}
			switch (Upper(arg_w[1]))
			{
				case L'A':
					switch (Upper(arg_w[2]))
					{
						case 0:
							Opt.CleanAscii=TRUE;
							break;

						case L'G':
							if (!arg_w[3])
								Opt.NoGraphics=TRUE;
							break;

						case L'N':
							if (!arg_w[3])
								Opt.NoBoxes=TRUE;
							break;
					}
					break;

				case L'E':
					if (Opt.OnlyEditorViewerUsed != Options::NOT_ONLY_EDITOR_VIEWER) //skip as already handled
					{
						I = (strcmp(argv[I+1], "-") == 0) ? argc : I + 1;
						break;
					}

					if (iswdigit(arg_w[2]))
					{
						auto ptr = arg_w.data() + 2;
						StartLine = _wtoi(ptr);
						auto ChPtr = wcschr(ptr, L':');
						if (ChPtr)
							StartChar = _wtoi(ChPtr+1);
					}

					if (I+1 < argc)
					{
						if (strcmp(argv[I+1], "-") == 0)
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR_ON_CMDOUT;
							strEditViewArg = ReconstructCommandLine(argc - I - 2, &argv[I+2]);
							I = argc;
						}
						else
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
							strEditViewArg = argv[I+1];
							I++;
						}
					}
					else { // -e without filename => new file to editor
						Opt.OnlyEditorViewerUsed = Options::ONLY_EDITOR;
						strEditViewArg.Clear();
					}
					break;

				case L'V':
					if (Opt.OnlyEditorViewerUsed != Options::NOT_ONLY_EDITOR_VIEWER) //skip as already handled
					{
						I = (strcmp(argv[I+1], "-") == 0) ? argc : I + 1;
						break;
					}

					if (I+1 < argc)
					{
						if (strcmp(argv[I+1], "-") == 0)
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_VIEWER_ON_CMDOUT;
							strEditViewArg = ReconstructCommandLine(argc - I - 2, &argv[I+2]);
							I = argc;
						}
						else
						{
							Opt.OnlyEditorViewerUsed = Options::ONLY_VIEWER;
							strEditViewArg = argv[I+1];
							I++;
						}
					}
					break;

				case L'M':
					switch (Upper(arg_w[2]))
					{
						case 0:
							Opt.Macro.DisableMacro|=MDOL_ALL;
							break;

						case L'A':
							if (!arg_w[3])
								Opt.Macro.DisableMacro|=MDOL_AUTOSTART;
							break;
					}
					break;

				case L'I':
					Opt.SmallIcon=TRUE;
					break;

				case L'P':
					bCustomPlugins = true;
					if (arg_w[2])
					{
						UserDefinedList Udl(ULF_UNIQUE | ULF_CASESENSITIVE, L':', 0);
						if (Udl.Set(arg_w.data() + 2))
						{
							for (size_t i=0; i < Udl.Size(); i++)
							{
								FARString path = Udl.Get(i);
								apiExpandEnvironmentStrings(path, path);
								// Unquote(path);
								ConvertNameToFull(path, path);
								if (!Opt.LoadPlug.strCustomPluginsPath.IsEmpty()) {
									Opt.LoadPlug.strCustomPluginsPath += L':';
								}
								Opt.LoadPlug.strCustomPluginsPath += path;
							}
						}
					}
					break;

				case L'C':
					if (Upper(arg_w[2])==L'O' && !arg_w[3])
					{
						Opt.LoadPlug.PluginsCacheOnly=TRUE;
					}
					else if (Upper(arg_w[2]) == L'D' && !arg_w[3]) {
						if (I + 1 < argc) {
							I++;
							arg_w = MB2Wide(argv[I]);
							switchHandled = false;
						}
					}
					break;

				case L'W':
					Opt.WindowMode=TRUE;
					break;

				case L'U':
					if (!arg_w[2])
						I++; // skip 2 args as this case is processed in SetCustomSettings()
					break;
			}
		}
		if (!switchHandled) // простые параметры. Их может быть max две штукА.
		{
			if (CntDestName < 2)
			{
				if (IsPluginPrefixPath(arg_w.c_str()))
				{
					DestNames[CntDestName++] = arg_w.c_str();
				}
				else
				{
					FARString tmpStr = arg_w;
					ConvertNameToFull(tmpStr, tmpStr);
					if (apiGetFileAttributes(tmpStr) != INVALID_FILE_ATTRIBUTES)
						DestNames[CntDestName++] = tmpStr;
				}
			}
		}
	}

	//Инициализация массива клавиш. Должна быть после CopyGlobalSettings!
	InitKeysArray();
	//WaitForInputIdle(GetCurrentProcess(),0);
	std::set_new_handler(nullptr);

	if (bCustomPlugins) { //если есть ключ /p то он отменяет /co
		Opt.LoadPlug.MainPluginDir = FALSE;
		Opt.LoadPlug.PluginsPersonal = FALSE;
		Opt.LoadPlug.PluginsCacheOnly = FALSE;
	}
	else if (Opt.LoadPlug.PluginsCacheOnly) {
		Opt.LoadPlug.MainPluginDir = FALSE;
		Opt.LoadPlug.PluginsPersonal = FALSE;
	}

	ZeroFarPalette();
	ConfigOptLoad();
	InitFarPalette();

	InitConsole();
	WINPORT(SetConsoleCursorBlinkTime)(NULL, Opt.CursorBlinkTime);

	static_assert(!IsPtr(Msg::NewFileName._id),
		"Too many language messages. Need to refactor code to eliminate use of IsPtr.");

	if (!Lang.Init(g_strFarPath,true,Msg::NewFileName.ID()))
	{
		LPCWSTR LngMsg;
		switch (Lang.LastError())
		{
		case LERROR_BAD_FILE:
			LngMsg = L"\nError: language data is incorrect or damaged. Press any key to exit...";
			break;
		case LERROR_FILE_NOT_FOUND:
			LngMsg = L"\nError: cannot find language data. Press any key to exit...";
			break;
		default:
			LngMsg = L"\nError: cannot load language data. Press any key to exit...";
		}
		ControlObject::ShowStartupBanner(LngMsg);
		WaitKey(); // А стоит ли ожидать клавишу??? Стоит
		return 1;
	}
	setenv("FARLANG", Opt.strLanguage.GetMB().c_str(), 1);

	CheckForImportLegacyShortcuts();

	// (!!!) temporary STUB because now Editor can not input filename "", see: fileedit.cpp -> FileEditor::Init()
	// default Editor file name for new empty file
	if ( Opt.OnlyEditorViewerUsed == Options::ONLY_EDITOR && strEditViewArg.IsEmpty() )
		strEditViewArg = Msg::NewFileName;

	int Result = MainProcess(strEditViewArg,DestNames[0],DestNames[1],StartLine,StartChar);

	EmptyInternalClipboard();
	VTShell_Shutdown();//ensure VTShell deinitialized before statics destructors called
	_OT(SysLog(L"[[[[[Exit of FAR]]]]]]]]]"));
	return Result;
}


/*void EncodingTest()
{
	std::wstring v = MB2Wide("\x80hello\x80""aaaaaaaaaaaa\x80""zzzzzzzzzzz\x80");
	printf("%u: '%ls'\n", (unsigned int)v.size(), v.c_str());
	for (size_t i = 0; i<v.size(); ++i)
		printf("%02x ", (unsigned int)v[i]);

	printf("\n");
	std::string a = StrWide2MB(v);
	for (size_t i = 0; i<a.size(); ++i)
		printf("%02x ", (unsigned char)a[i]);
	printf("\n");
}

void SudoTest()
{
	SudoClientRegion sdc_rgn;
	int fd = sdc_open("/root/foo", O_CREAT | O_RDWR, 0666);
	if (fd!=-1) {
		sdc_write(fd, "bar", 3);
		sdc_close(fd);
	} else
		perror("sdc_open");
	exit(0);
}
*/

static int libexec(const char *lib, const char *cd, const char *symbol, int argc, char *argv[])
{
	void *dl = dlopen(lib, RTLD_LOCAL|RTLD_LAZY);
	if (!dl) {
		fprintf(stderr, "libexec('%s', '%s', %u) - dlopen error %u\n", lib, symbol, argc, errno);
		return -1;
	}

	typedef int (*libexec_main_t)(int argc, char *argv[]);
	libexec_main_t libexec_main = (libexec_main_t)dlsym(dl, symbol);
	if (!libexec_main) {
		fprintf(stderr, "libexec('%s', '%s', %u) - dlsym error %u\n", lib, symbol, argc, errno);
		return -1;
	}

	if (cd && *cd && chdir(cd) == -1) {
		fprintf(stderr, "libexec('%s', '%s', %u) - chdir('%s') error %u\n", lib, symbol, argc, cd, errno);
	}

	return libexec_main(argc, argv);
}

static void SetCustomSettings(const char *arg)
{
	std::string refined;
	if (arg[0] == '.' && arg[1] == GOOD_SLASH) {
		char cwdbuf[MAX_PATH + 1];
		const char *cwd = getcwd(cwdbuf, MAX_PATH);
		if (cwd) {
			refined = cwd;
			refined+= &arg[1];
		}
	}
	else {
		refined = arg;
	}

	while (!refined.empty() && refined.back() == GOOD_SLASH) {
		refined.pop_back();
	}

	fprintf(stderr, "%s: '%s'\n", __FUNCTION__, refined.c_str());

	if (!refined.empty()) {
		// could use FARPROFILE/FARLOCALPROFILE for that but it would be abusing
		setenv("FARSETTINGS", refined.c_str(), 1);
	}
}

int _cdecl main(int argc, char *argv[])
{
	char *name = strrchr(argv[0], GOOD_SLASH);
	if (name) ++name; else name = argv[0];
	if (argc > 0) {
		if (strcmp(name, "far2m_askpass")==0)
			return sudo_main_askpass();
		if (strcmp(name, "far2m_sudoapp")==0)
			return sudo_main_dispatcher(argc - 1, argv + 1);
		if (argc >= 5) {
			if (strcmp(argv[1], "--libexec") == 0) {
				return libexec(argv[2], argv[3], argv[4], argc - 5, argv + 5);
			}
		}
		if (argc > 1 &&
		(strncasecmp(argv[1], "--h", 3) == 0
		 || strncasecmp(argv[1], "-h", 2) == 0
		 || strcmp(argv[1], "-?") == 0)) {

			print_help(name);
			return 0;
		}
	}

	unsetenv("FARSETTINGS"); // don't inherit from parent process in any case
	unsetenv("SUDO_ASKPASS"); // +
	for (int i = 1; i + 1 < argc; ++i) {
		if (!strcasecmp(argv[i],"-u")) {
			++i;
			if (*argv[i]) {
				SetCustomSettings(argv[i]);
			}
		}
	}

	setlocale(LC_ALL, "");//otherwise non-latin keys missing with XIM input method

	const char *lcc = getenv("LC_COLLATE");
	if (lcc && *lcc) {
		setlocale(LC_COLLATE, lcc);
	}

	SetupFarPath(argv[0]);

	{	// if CONFIG_INI is not present => first start & opt for show Help "FAR2L features - Getting Started"
		struct stat stat_buf;
		Opt.IsFirstStart = stat( InMyConfig(CONFIG_INI).c_str(), &stat_buf ) == -1;
	}

	SafeMMap::SignalHandlerRegistrar smm_shr;

	gMainThreadID = GetInterThreadID();

	return WinPortMain(g_strFarModuleName.GetMB().c_str(), argc, argv, FarAppMain);
}
