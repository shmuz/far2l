#pragma once
#include <string>
#include <memory>
#include "IVTShell.h"

struct VTAnsiContext;

class VTAnsi
{
	struct {
		std::string tail, tmp;
	} _incomplete;

	std::wstring _ws, _saved_title;
	std::unique_ptr<VTAnsiContext> _ctx;

	void RevertConsoleState(HANDLE con_hnd);

	public:
	VTAnsi(IVTShell *vtsh);
	~VTAnsi();

	void DisableOutput();
	void EnableOutput();
	
	void Write(const char *str, size_t len);
	
	struct VTAnsiState *Suspend();
	void Resume(struct VTAnsiState* state);

	void OnStart();
	void OnStop();
	void OnDetached();
	std::string GetTitle();
};

class VTAnsiSuspend
{
	VTAnsi &_vta;
	struct VTAnsiState *_ansi_state;

	public:
	VTAnsiSuspend(VTAnsi &vta) 
		: _vta(vta), _ansi_state(_vta.Suspend())
	{
	}

	~VTAnsiSuspend()
	{
		if (_ansi_state)
			_vta.Resume(_ansi_state);
	}
	
	inline operator bool() const
	{
		return _ansi_state != nullptr;
	}
};
