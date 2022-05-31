#pragma once

/*
macro.hpp

Макросы
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

#include "farplug-wide.h"
#include "tvar.hpp"

template <typename T>
bool CheckStructSize(const T* s)
{
	return s && (s->StructSize >= sizeof(T));
}

#define ALIGNAS(value, alignment) ((value+(alignment-1))&~(alignment-1))
#define ALIGN(value) ALIGNAS(value, sizeof(void*))

// Macro Const
enum
{
	constMsX          = 0,
	constMsY          = 1,
	constMsButton     = 2,
	constMsCtrlState  = 3,
	constMsEventFlags = 4,
	constMsLastCtrlState = 5,
	constMsLAST       = 6,
};

enum MACRODISABLEONLOAD
{
	MDOL_ALL            = 0x80000000, // дисаблим все макросы при загрузке
	MDOL_AUTOSTART      = 0x00000001, // дисаблим автостартующие макросы
};

// области действия макросов (начало исполнения) -  НЕ БОЛЕЕ 0xFF областей!
enum MACROMODEAREA
{
	MACRO_OTHER           = MACROAREA_OTHER,          // Режим копирования текста с экрана, вертикальные меню
	MACRO_SHELL           = MACROAREA_SHELL,          // Файловые панели
	MACRO_VIEWER          = MACROAREA_VIEWER,         // Внутренняя программа просмотра
	MACRO_EDITOR          = MACROAREA_EDITOR,         // Редактор
	MACRO_DIALOG          = MACROAREA_DIALOG,         // Диалоги
	MACRO_SEARCH          = MACROAREA_SEARCH,         // Быстрый поиск в панелях
	MACRO_DISKS           = MACROAREA_DISKS,          // Меню выбора дисков
	MACRO_MAINMENU        = MACROAREA_MAINMENU,       // Основное меню
	MACRO_MENU            = MACROAREA_MENU,           // Прочие меню
	MACRO_HELP            = MACROAREA_HELP,           // Система помощи
	MACRO_INFOPANEL       = MACROAREA_INFOPANEL,      // Информационная панель
	MACRO_QVIEWPANEL      = MACROAREA_QVIEWPANEL,     // Панель быстрого просмотра
	MACRO_TREEPANEL       = MACROAREA_TREEPANEL,      // Панель дерева папок
	MACRO_FINDFOLDER      = MACROAREA_FINDFOLDER,     // Поиск папок
	MACRO_USERMENU        = MACROAREA_USERMENU,       // Меню пользователя
	MACRO_AUTOCOMPLETION  = MACROAREA_AUTOCOMPLETION, // Список автодополнения

	MACRO_COMMON,                     // ВЕЗДЕ! - должен быть предпоследним, т.к. приоритет самый низший !!!
	MACRO_LAST                        // Должен быть всегда последним! Используется в циклах
};

// коды возврата для KeyMacro::GetCurRecord()
enum MACRORECORDANDEXECUTETYPE
{
	MACROMODE_NOMACRO          =0,  // не в режиме макро
	MACROMODE_EXECUTING        =1,  // исполнение: без передачи плагину пимп
	MACROMODE_EXECUTING_COMMON =2,  // исполнение: с передачей плагину пимп
	MACROMODE_RECORDING        =3,  // запись: без передачи плагину пимп
	MACROMODE_RECORDING_COMMON =4,  // запись: с передачей плагину пимп
};

class Panel;

enum INTMF_FLAGS{
	IMFF_UNLOCKSCREEN               =0x00000001,
	IMFF_DISABLEINTINPUT            =0x00000002,
};

struct MacroPanelSelect {
	int     Action;
	DWORD   ActionFlags;
	int     Mode;
	int64_t Index;
	TVar    *Item;
};

class KeyMacro
{
public:
	KeyMacro();

	static bool AddMacro(DWORD PluginId, const MacroAddMacro* Data);
	static bool DelMacro(DWORD PluginId, void* Id);
	static bool ExecuteString(MacroExecuteString *Data);
	static bool GetMacroKeyInfo(const FARString& StrArea, int Pos, FARString &strKeyName, FARString &strDescription);
	static bool IsOutputDisabled();
	static bool IsExecuting() { return GetExecutingState() != MACROSTATE_NOMACRO; }
	static bool IsHistoryDisabled(int TypeHistory);
	static bool MacroExists(int Key, int Area, bool UseCommon);
	static void RunStartMacro();
	static bool SaveMacros(bool always);
	static void SetMacroConst(int ConstIndex, long long Value);
	static bool PostNewMacro(const wchar_t* Sequence, DWORD InputFlags, DWORD AKey = 0);

	intptr_t CallFar(intptr_t CheckCode, FarMacroCall* Data);
	bool CheckWaitKeyFunc() const;
	int  GetState() const;
	int  GetKey();
	static DWORD GetMacroParseError(COORD& ErrPos, FARString& ErrSrc);
	int GetArea() const { return m_Area; }
	const wchar_t* GetStringToPrint() const { return m_StringToPrint.CPtr(); }
	bool IsRecording() const { return m_Recording != MACROSTATE_NOMACRO; }
	bool LoadMacros(bool FromFar, bool InitedRAM=true, const FarMacroLoad *Data=nullptr);
	static bool ParseMacroString(const wchar_t* Sequence,DWORD Flags,bool skipFile);
	int  PeekKey() const;
	void SetArea(int Area) { m_Area=Area; }
	void SuspendMacros(bool Suspend) { Suspend ? ++m_InternalInput : --m_InternalInput; }

private:
	static int GetExecutingState();
	int AssignMacroKey(DWORD& MacroKey, DWORD& Flags);
	static int GetMacroSettings(uint32_t Key,DWORD &Flags, const wchar_t* Src=L"", const wchar_t* Descr=L"");
	void RestoreMacroChar() const;

	int m_Area;
	int m_StartMode;
	int m_Recording;
	static FARString m_RecCode;
	static FARString m_RecDescription;
	int m_InternalInput;
	int m_WaitKey;
	FARString m_StringToPrint;

	private:
		BOOL CheckEditSelected(DWORD CurFlags);
		BOOL CheckInsidePlugin(DWORD CurFlags);
		BOOL CheckPanel(int PanelMode,DWORD CurFlags, BOOL IsPassivePanel);
		BOOL CheckCmdLine(int CmdLength,DWORD Flags);
		BOOL CheckFileFolder(Panel *ActivePanel,DWORD CurFlags, BOOL IsPassivePanel);
		BOOL CheckAll(int CheckMode,DWORD CurFlags);

	private:
		static LONG_PTR WINAPI AssignMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);
		static LONG_PTR WINAPI ParamMacroDlgProc(HANDLE hDlg,int Msg,int Param1,LONG_PTR Param2);

	public:
		bool ProcessKey(DWORD Key);
};

inline bool IsMenuArea(int Area){return Area==MACRO_MAINMENU || Area==MACRO_MENU || Area==MACRO_DISKS || Area==MACRO_USERMENU || Area==MACRO_AUTOCOMPLETION;}
void Log(const char* str, ...);
