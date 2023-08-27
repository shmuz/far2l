/*
config.cpp

Конфигурация
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

#include "config.hpp"
#include "lang.hpp"
#include "language.hpp"
#include "keys.hpp"
#include "colors.hpp"
#include "cmdline.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filepanels.hpp"
#include "filelist.hpp"
#include "panel.hpp"
#include "help.hpp"
#include "filefilter.hpp"
#include "poscache.hpp"
#include "findfile.hpp"
#include "hilight.hpp"
#include "interf.hpp"
#include "keyboard.hpp"
#include "palette.hpp"
#include "message.hpp"
#include "stddlg.hpp"
#include "pathmix.hpp"
#include "dirmix.hpp"
#include "panelmix.hpp"
#include "strmix.hpp"
#include "udlist.hpp"
#include "datetime.hpp"
#include "DialogBuilder.hpp"
#include "vtshell.h"
#include "ConfigRW.hpp"

Options Opt={0};

// Стандартный набор разделителей
static const wchar_t *WordDiv0 = L"~!%^&*()+|{}:\"<>?`-=\\[];',./";

// Стандартный набор разделителей для функции Xlat
static const wchar_t *WordDivForXlat0=L" \t!#$%^&*()+|=\\/@?";

FARString strKeyNameConsoleDetachKey;
static const wchar_t szCtrlDot[]=L"Ctrl.";
static const wchar_t szCtrlShiftDot[]=L"CtrlShift.";

// KeyName
static const char NSecColors[]="Colors";
static const char NSecScreen[]="Screen";
static const char NSecCmdline[]="Cmdline";
static const char NSecInterface[]="Interface";
static const char NSecInterfaceCompletion[]="Interface/Completion";
static const char NSecViewer[]="Viewer";
static const char NSecDialog[]="Dialog";
static const char NSecEditor[]="Editor";
static const char NSecNotifications[]="Notifications";
static const char NSecXLat[]="XLat";
static const char NSecSystem[]="System";
static const char NSecSystemExecutor[]="System/Executor";
static const char NSecSystemNowell[]="System/Nowell";
static const char NSecHelp[]="Help";
static const char NSecLanguage[]="Language";
static const char NSecConfirmations[]="Confirmations";
static const char NSecPluginConfirmations[]="PluginConfirmations";
static const char NSecPanel[]="Panel";
static const char NSecPanelLeft[]="Panel/Left";
static const char NSecPanelRight[]="Panel/Right";
static const char NSecPanelLayout[]="Panel/Layout";
static const char NSecPanelTree[]="Panel/Tree";
static const char NSecLayout[]="Layout";
static const char NSecDescriptions[]="Descriptions";
static const char NSecKeyMacros[]="KeyMacros";
static const char NSecPolicies[]="Policies";
static const char NSecSavedHistory[]="SavedHistory";
static const char NSecSavedViewHistory[]="SavedViewHistory";
static const char NSecSavedFolderHistory[]="SavedFolderHistory";
static const char NSecSavedDialogHistory[]="SavedDialogHistory";
static const char NSecCodePages[]="CodePages";
static const char NParamHistoryCount[]="HistoryCount";
static const char NSecVMenu[]="VMenu";

struct AllXlats : std::vector<std::string>
{
	AllXlats()
		: std::vector<std::string>(KeyFileReadHelper(InMyConfig("xlats.ini")).EnumSections())
	{
		const auto &xlats_global = KeyFileReadHelper(GetHelperPathName("xlats.ini")).EnumSections();
		for (const auto &xlat : xlats_global) { // local overrides global
			if (std::find(begin(), end(), xlat) == end()) {
				emplace_back(xlat);
			}
		}
	}
};

static DWORD ApplyConsoleTweaks()
{
	DWORD64 tweaks = 0;
	if (Opt.ExclusiveCtrlLeft)
		tweaks|= EXCLUSIVE_CTRL_LEFT;
	if (Opt.ExclusiveCtrlRight)
		tweaks|= EXCLUSIVE_CTRL_RIGHT;
	if (Opt.ExclusiveAltLeft)
		tweaks|= EXCLUSIVE_ALT_LEFT;
	if (Opt.ExclusiveAltRight)
		tweaks|= EXCLUSIVE_ALT_RIGHT;
	if (Opt.ExclusiveWinLeft)
		tweaks|= EXCLUSIVE_WIN_LEFT;
	if (Opt.ExclusiveWinRight)
		tweaks|= EXCLUSIVE_WIN_RIGHT;
	if (Opt.ConsolePaintSharp)
		tweaks|= CONSOLE_PAINT_SHARP;
	if (Opt.OSC52ClipSet)
		tweaks|= CONSOLE_OSC52CLIP_SET;
	if (Opt.TTYPaletteOverride)
		tweaks|= CONSOLE_TTY_PALETTE_OVERRIDE;
	return WINPORT(SetConsoleTweaks)(tweaks);
}

static void ApplySudoConfiguration()
{
 	const std::string &sudo_app = GetHelperPathName("far2m_sudoapp");
	const std::string &askpass_app = GetHelperPathName("far2m_askpass");

	SudoClientMode mode;
	if (Opt.SudoEnabled) {
		mode = Opt.SudoConfirmModify ? SCM_CONFIRM_MODIFY : SCM_CONFIRM_NONE;
	} else
		mode = SCM_DISABLE;
	sudo_client_configure(mode, Opt.SudoPasswordExpiration, sudo_app.c_str(), askpass_app.c_str(),
		Wide2MB(Msg::SudoTitle).c_str(), Wide2MB(Msg::SudoPrompt).c_str(), Wide2MB(Msg::SudoConfirm).c_str());
}

static void AddHistorySettings(DialogBuilder &Builder, FarLangMsg MTitle, int *OptEnabled, int *OptCount)
{
	DialogItemEx *EnabledCheckBox = Builder.AddCheckbox(MTitle, OptEnabled);
	DialogItemEx *CountEdit = Builder.AddIntEditField(OptCount, 6);
	DialogItemEx *CountText = Builder.AddTextBefore(CountEdit, Msg::ConfigMaxHistoryCount);
	CountEdit->Indent(4);
	CountText->Indent(4);
	Builder.LinkFlags(EnabledCheckBox, CountEdit, DIF_DISABLE);
	Builder.LinkFlags(EnabledCheckBox, CountText, DIF_DISABLE);
}

static void SanitizeHistoryCounts()
{
	Opt.HistoryCount = std::max(Opt.HistoryCount, 16);
	Opt.FoldersHistoryCount = std::max(Opt.FoldersHistoryCount, 16);
	Opt.ViewHistoryCount = std::max(Opt.ViewHistoryCount, 16);
	Opt.DialogsHistoryCount = std::max(Opt.DialogsHistoryCount, 16);
}

void SystemSettings()
{
	DialogBuilder Builder(Msg::ConfigSystemTitle, L"SystemSettings");

	DialogItemEx *SudoEnabledItem = Builder.AddCheckbox(Msg::ConfigSudoEnabled, &Opt.SudoEnabled);
	DialogItemEx *SudoPasswordExpirationEdit = Builder.AddIntEditField(&Opt.SudoPasswordExpiration, 4);
	DialogItemEx *SudoPasswordExpirationText = Builder.AddTextBefore(SudoPasswordExpirationEdit, Msg::ConfigSudoPasswordExpiration);

	SudoPasswordExpirationText->Indent(4);
	SudoPasswordExpirationEdit->Indent(4);

	DialogItemEx *SudoConfirmModifyItem = Builder.AddCheckbox(Msg::ConfigSudoConfirmModify, &Opt.SudoConfirmModify);
	SudoConfirmModifyItem->Indent(4);

	Builder.LinkFlags(SudoEnabledItem, SudoConfirmModifyItem, DIF_DISABLE);
	Builder.LinkFlags(SudoEnabledItem, SudoPasswordExpirationEdit, DIF_DISABLE);

	DialogItemEx *DeleteToRecycleBin = Builder.AddCheckbox(Msg::ConfigRecycleBin, &Opt.DeleteToRecycleBin);
	DialogItemEx *DeleteLinks = Builder.AddCheckbox(Msg::ConfigRecycleBinLink, &Opt.DeleteToRecycleBinKillLink);
	DeleteLinks->Indent(4);
	Builder.LinkFlags(DeleteToRecycleBin, DeleteLinks, DIF_DISABLE);

//	Builder.AddCheckbox(MSudoParanoic, &Opt.SudoParanoic);
//	Builder.AddCheckbox(CopyWriteThrough, &Opt.CMOpt.WriteThrough);
	Builder.AddCheckbox(Msg::ConfigScanJunction, &Opt.ScanJunction);
	Builder.AddCheckbox(Msg::ConfigOnlyFilesSize, &Opt.OnlyFilesSize);

	DialogItemEx *InactivityExit = Builder.AddCheckbox(Msg::ConfigInactivity, &Opt.InactivityExit);
	DialogItemEx *InactivityExitTime = Builder.AddIntEditField(&Opt.InactivityExitTime, 2);
	InactivityExitTime->Indent(4);
	Builder.AddTextAfter(InactivityExitTime, Msg::ConfigInactivityMinutes);
	Builder.LinkFlags(InactivityExit, InactivityExitTime, DIF_DISABLE);

	AddHistorySettings(Builder, Msg::ConfigSaveHistory, &Opt.SaveHistory, &Opt.HistoryCount);
	AddHistorySettings(Builder, Msg::ConfigSaveFoldersHistory, &Opt.SaveFoldersHistory, &Opt.FoldersHistoryCount);
	AddHistorySettings(Builder, Msg::ConfigSaveViewHistory, &Opt.SaveViewHistory, &Opt.ViewHistoryCount);

	Builder.AddCheckbox(Msg::ConfigAutoSave, &Opt.AutoSaveSetup);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		SanitizeHistoryCounts();
		ApplySudoConfiguration();
	}
}


void PanelSettings()
{
	DialogBuilder Builder(Msg::ConfigPanelTitle, L"PanelSettings");
	BOOL AutoUpdate = (Opt.AutoUpdateLimit );

	Builder.AddCheckbox(Msg::ConfigHidden, &Opt.ShowHidden);
	Builder.AddCheckbox(Msg::ConfigHighlight, &Opt.Highlight);
	Builder.AddCheckbox(Msg::ConfigAutoChange, &Opt.Tree.AutoChangeFolder);
	Builder.AddCheckbox(Msg::ConfigSelectFolders, &Opt.SelectFolders);
	Builder.AddCheckbox(Msg::ConfigSortFolderExt, &Opt.SortFolderExt);
	Builder.AddCheckbox(Msg::ConfigReverseSort, &Opt.ReverseSort);

	DialogItemEx *AutoUpdateEnabled = Builder.AddCheckbox(Msg::ConfigAutoUpdateLimit, &AutoUpdate);
	DialogItemEx *AutoUpdateLimit = Builder.AddIntEditField((int *) &Opt.AutoUpdateLimit, 6);
	Builder.LinkFlags(AutoUpdateEnabled, AutoUpdateLimit, DIF_DISABLE, false);
	DialogItemEx *AutoUpdateText = Builder.AddTextBefore(AutoUpdateLimit, Msg::ConfigAutoUpdateLimit2);
	AutoUpdateLimit->Indent(4);
	AutoUpdateText->Indent(4);
	Builder.AddCheckbox(Msg::ConfigAutoUpdateRemoteDrive, &Opt.AutoUpdateRemoteDrive);

	Builder.AddSeparator();
	Builder.AddCheckbox(Msg::ConfigShowColumns, &Opt.ShowColumnTitles);
	Builder.AddCheckbox(Msg::ConfigShowStatus, &Opt.ShowPanelStatus);
	Builder.AddCheckbox(Msg::ConfigShowTotal, &Opt.ShowPanelTotals);
	Builder.AddCheckbox(Msg::ConfigShowFree, &Opt.ShowPanelFree);
	Builder.AddCheckbox(Msg::ConfigShowScrollbar, &Opt.ShowPanelScrollbar);
	Builder.AddCheckbox(Msg::ConfigShowScreensNumber, &Opt.ShowScreensNumber);
	Builder.AddCheckbox(Msg::ConfigShowSortMode, &Opt.ShowSortMode);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		if (!AutoUpdate)
			Opt.AutoUpdateLimit = 0;

	//  FrameManager->RefreshFrame();
		CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
		CtrlObject->Cp()->Redraw();
	}
}


void InputSettings()
{
	const DWORD supported_tweaks = ApplyConsoleTweaks();

	std::vector<DialogBuilderListItem> XLatItems;
	AllXlats xlats;

	int SelectedXLat = -1;
	for (int i = 0; i < (int)xlats.size(); ++i) {
		if (Opt.XLat.XLat == xlats[i]) {
			SelectedXLat = i;
		}
		XLatItems.emplace_back(DialogBuilderListItem{ FarLangMsg{::Lang.InternMsg(xlats[i].c_str())}, i});
	}

	DialogBuilder Builder(Msg::ConfigInputTitle, L"InputSettings");
	Builder.AddCheckbox(Msg::ConfigMouse, &Opt.Mouse);

	Builder.AddText(Msg::ConfigXLats);
	DialogItemEx *Item = Builder.AddComboBox(&SelectedXLat, 40,
		XLatItems.data(), XLatItems.size(), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);
	Item->Indent(4);

	Builder.AddCheckbox(Msg::ConfigXLatFastFileFind, &Opt.XLat.EnableForFastFileFind);
	Builder.AddCheckbox(Msg::ConfigXLatDialogs, &Opt.XLat.EnableForDialogs);

	if (supported_tweaks & TWEAK_STATUS_SUPPORT_EXCLUSIVE_KEYS) {
		Builder.AddText(Msg::ConfigExclusiveKeys);
		Item = Builder.AddCheckbox(Msg::ConfigExclusiveCtrlLeft, &Opt.ExclusiveCtrlLeft);
		Item->Indent(4);
		Builder.AddCheckboxAfter(Item, Msg::ConfigExclusiveCtrlRight, &Opt.ExclusiveCtrlRight);

		Item = Builder.AddCheckbox(Msg::ConfigExclusiveAltLeft, &Opt.ExclusiveAltLeft);
		Item->Indent(4);
		Builder.AddCheckboxAfter(Item, Msg::ConfigExclusiveAltRight, &Opt.ExclusiveAltRight);

		Item = Builder.AddCheckbox(Msg::ConfigExclusiveWinLeft, &Opt.ExclusiveWinLeft);
		Item->Indent(4);
		Builder.AddCheckboxAfter(Item, Msg::ConfigExclusiveWinRight, &Opt.ExclusiveWinRight);
	}

	Builder.AddOKCancel();

	if (Builder.ShowDialog()) {
		if (size_t(SelectedXLat) < xlats.size()) {
			Opt.XLat.XLat = xlats[SelectedXLat];
		}
		ApplyConsoleTweaks();
	}
}

/* $ 17.12.2001 IS
   Настройка средней кнопки мыши для панелей. Воткнем пока сюда, потом надо
   переехать в специальный диалог по программированию мыши.
*/
void InterfaceSettings()
{
	for (;;) {
		DialogBuilder Builder(Msg::ConfigInterfaceTitle, L"InterfSettings");

		Builder.AddCheckbox(Msg::ConfigClock, &Opt.Clock);
		Builder.AddCheckbox(Msg::ConfigViewerEditorClock, &Opt.ViewerEditorClock);
		Builder.AddCheckbox(Msg::ConfigKeyBar, &Opt.ShowKeyBar);
		Builder.AddCheckbox(Msg::ConfigMenuBar, &Opt.ShowMenuBar);
		DialogItemEx *SaverCheckbox = Builder.AddCheckbox(Msg::ConfigSaver, &Opt.ScreenSaver);

		DialogItemEx *SaverEdit = Builder.AddIntEditField(&Opt.ScreenSaverTime, 2);
		SaverEdit->Indent(4);
		Builder.AddTextAfter(SaverEdit, Msg::ConfigSaverMinutes);
		Builder.LinkFlags(SaverCheckbox, SaverEdit, DIF_DISABLE);

		Builder.AddCheckbox(Msg::ConfigCopyTotal, &Opt.CMOpt.CopyShowTotal);
		Builder.AddCheckbox(Msg::ConfigCopyTimeRule, &Opt.CMOpt.CopyTimeRule);
		Builder.AddCheckbox(Msg::ConfigDeleteTotal, &Opt.DelOpt.DelShowTotal);
		Builder.AddCheckbox(Msg::ConfigPgUpChangeDisk, &Opt.PgUpChangeDisk);


		const DWORD supported_tweaks = ApplyConsoleTweaks();
		int ChangeFontID = -1;
		DialogItemEx *ChangeFontItem = nullptr;
		if (supported_tweaks & TWEAK_STATUS_SUPPORT_PAINT_SHARP) {
			ChangeFontItem = Builder.AddButton(Msg::ConfigConsoleChangeFont, ChangeFontID);
		}

		if (supported_tweaks & TWEAK_STATUS_SUPPORT_PAINT_SHARP) {
			if (ChangeFontItem)
				Builder.AddCheckboxAfter(ChangeFontItem, Msg::ConfigConsolePaintSharp, &Opt.ConsolePaintSharp);
			else
				Builder.AddCheckbox(Msg::ConfigConsolePaintSharp, &Opt.ConsolePaintSharp);
		}

		if (supported_tweaks & TWEAK_STATUS_SUPPORT_OSC52CLIP_SET) {
			Builder.AddCheckbox(Msg::ConfigOSC52ClipSet, &Opt.OSC52ClipSet);
		}

		if (supported_tweaks & TWEAK_STATUS_SUPPORT_TTY_PALETTE) {
			Builder.AddCheckbox(Msg::ConfigTTYPaletteOverride, &Opt.TTYPaletteOverride);
		}

		Builder.AddText(Msg::ConfigWindowTitle);
		Builder.AddEditField(&Opt.strWindowTitle, 47);

		//OKButton->Flags = DIF_CENTERGROUP;
		//OKButton->DefaultButton = TRUE;
		//OKButton->Y1 = OKButton->Y2 = NextY++;
		//OKButtonID = DialogItemsCount-1;


		Builder.AddOKCancel();

		int clicked_id = -1;
		if (Builder.ShowDialog(&clicked_id)) {
			if (Opt.CMOpt.CopyTimeRule)
				Opt.CMOpt.CopyTimeRule = 3;

			SetFarConsoleMode();
			CtrlObject->Cp()->LeftPanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->RightPanel->Update(UPDATE_KEEP_SELECTION);
			CtrlObject->Cp()->SetScreenPosition();
			// $ 10.07.2001 SKV ! надо это делать, иначе если кейбар спрятали, будет полный рамс.
			CtrlObject->Cp()->Redraw();
			ApplyConsoleTweaks();
			break;
		}

		if (ChangeFontID == -1 || clicked_id != ChangeFontID)
			break;

		WINPORT(ConsoleChangeFont)();
	}
}

void AutoCompleteSettings()
{
	DialogBuilder Builder(Msg::ConfigAutoCompleteTitle, L"AutoCompleteSettings");
	DialogItemEx *ListCheck=Builder.AddCheckbox(Msg::ConfigAutoCompleteShowList, &Opt.AutoComplete.ShowList);
	DialogItemEx *ModalModeCheck=Builder.AddCheckbox(Msg::ConfigAutoCompleteModalList, &Opt.AutoComplete.ModalList);
	ModalModeCheck->Indent(4);
	Builder.AddCheckbox(Msg::ConfigAutoCompleteAutoAppend, &Opt.AutoComplete.AppendCompletion);
	Builder.LinkFlags(ListCheck, ModalModeCheck, DIF_DISABLE);

	Builder.AddText(Msg::ConfigAutoCompleteExceptions);
	Builder.AddEditField(&Opt.AutoComplete.Exceptions, 47);

	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void InfoPanelSettings()
{

	DialogBuilder Builder(Msg::ConfigInfoPanelTitle, L"InfoPanelSettings");
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void DialogSettings()
{
	DialogBuilder Builder(Msg::ConfigDlgSetsTitle, L"DialogSettings");

	AddHistorySettings(Builder, Msg::ConfigDialogsEditHistory, &Opt.Dialogs.EditHistory, &Opt.DialogsHistoryCount);
	Builder.AddCheckbox(Msg::ConfigDialogsEditBlock, &Opt.Dialogs.EditBlock);
	Builder.AddCheckbox(Msg::ConfigDialogsDelRemovesBlocks, &Opt.Dialogs.DelRemovesBlocks);
	Builder.AddCheckbox(Msg::ConfigDialogsAutoComplete, &Opt.Dialogs.AutoComplete);
	Builder.AddCheckbox(Msg::ConfigDialogsEULBsClear, &Opt.Dialogs.EULBsClear);
	Builder.AddCheckbox(Msg::ConfigDialogsMouseButton, &Opt.Dialogs.MouseButton);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		SanitizeHistoryCounts();
		if (Opt.Dialogs.MouseButton )
			Opt.Dialogs.MouseButton = 0xFFFF;
	}
}

void VMenuSettings()
{
	DialogBuilderListItem CAListItems[]=
	{
		{ Msg::ConfigVMenuClickCancel, VMENUCLICK_CANCEL },  // Cancel menu
		{ Msg::ConfigVMenuClickApply,  VMENUCLICK_APPLY  },  // Execute selected item
		{ Msg::ConfigVMenuClickIgnore, VMENUCLICK_IGNORE },  // Do nothing
	};

	DialogBuilder Builder(Msg::ConfigVMenuTitle, L"VMenuSettings");

	Builder.AddText(Msg::ConfigVMenuLBtnClick);
	Builder.AddComboBox((int *) &Opt.VMenu.LBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);
	Builder.AddText(Msg::ConfigVMenuRBtnClick);
	Builder.AddComboBox((int *) &Opt.VMenu.RBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);
	Builder.AddText(Msg::ConfigVMenuMBtnClick);
	Builder.AddComboBox((int *) &Opt.VMenu.MBtnClick, 40, CAListItems, ARRAYSIZE(CAListItems), DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void CmdlineSettings()
{
	DialogBuilderListItem CMWListItems[] = {
		{ Msg::ConfigCmdlineWaitKeypress_Never,   0 },
		{ Msg::ConfigCmdlineWaitKeypress_OnError, 1 },
		{ Msg::ConfigCmdlineWaitKeypress_Always,  2 },
	};

	DialogBuilder Builder(Msg::ConfigCmdlineTitle, L"CmdlineSettings");
	AddHistorySettings(Builder, Msg::ConfigSaveHistory, &Opt.SaveHistory, &Opt.HistoryCount);
	Builder.AddCheckbox(Msg::ConfigCmdlineEditBlock, &Opt.CmdLine.EditBlock);
	Builder.AddCheckbox(Msg::ConfigCmdlineDelRemovesBlocks, &Opt.CmdLine.DelRemovesBlocks);
	Builder.AddCheckbox(Msg::ConfigCmdlineAutoComplete, &Opt.CmdLine.AutoComplete);

	DialogItemEx *LimitEdit = Builder.AddIntEditField(&Opt.CmdLine.VTLogLimit, 6);
	Builder.AddTextBefore(LimitEdit, Msg::ConfigCmdlineVTLogLimit);

	Builder.AddText(Msg::ConfigCmdlineWaitKeypress);
	Builder.AddComboBox((int *)&Opt.CmdLine.WaitKeypress, 40, CMWListItems, ARRAYSIZE(CMWListItems),
			DIF_DROPDOWNLIST | DIF_LISTAUTOHIGHLIGHT | DIF_LISTWRAPMODE);

	DialogItemEx *UsePromptFormat =
			Builder.AddCheckbox(Msg::ConfigCmdlineUsePromptFormat, &Opt.CmdLine.UsePromptFormat);
	DialogItemEx *PromptFormat = Builder.AddEditField(&Opt.CmdLine.strPromptFormat, 19);
	PromptFormat->Indent(4);
	Builder.LinkFlags(UsePromptFormat, PromptFormat, DIF_DISABLE);
	DialogItemEx *UseShell = Builder.AddCheckbox(Msg::ConfigCmdlineUseShell, &Opt.CmdLine.UseShell);
	DialogItemEx *Shell = Builder.AddEditField(&Opt.CmdLine.strShell, 19);
	Shell->Indent(4);
	Builder.LinkFlags(UseShell, Shell, DIF_DISABLE);
	Builder.AddOKCancel();

	int oldUseShell = Opt.CmdLine.UseShell;
	FARString oldShell = Opt.CmdLine.strShell;

	if (Builder.ShowDialog()) {
		SanitizeHistoryCounts();

		CtrlObject->CmdLine->SetPersistentBlocks(Opt.CmdLine.EditBlock);
		CtrlObject->CmdLine->SetDelRemovesBlocks(Opt.CmdLine.DelRemovesBlocks);
		CtrlObject->CmdLine->SetAutoComplete(Opt.CmdLine.AutoComplete);

		if (Opt.CmdLine.UseShell != oldUseShell || Opt.CmdLine.strShell != oldShell)
			VTShell_Shutdown();
	}
}

void SetConfirmations()
{
	DialogBuilder Builder(Msg::SetConfirmTitle, L"ConfirmDlg");

	Builder.AddCheckbox(Msg::SetConfirmCopy, &Opt.Confirm.Copy);
	Builder.AddCheckbox(Msg::SetConfirmMove, &Opt.Confirm.Move);
	Builder.AddCheckbox(Msg::SetConfirmRO, &Opt.Confirm.RO);
	Builder.AddCheckbox(Msg::SetConfirmDelete, &Opt.Confirm.Delete);
	Builder.AddCheckbox(Msg::SetConfirmDeleteFolders, &Opt.Confirm.DeleteFolder);
	Builder.AddCheckbox(Msg::SetConfirmEsc, &Opt.Confirm.Esc);
	Builder.AddCheckbox(Msg::SetConfirmRemoveConnection, &Opt.Confirm.RemoveConnection);
	Builder.AddCheckbox(Msg::SetConfirmRemoveSUBST, &Opt.Confirm.RemoveSUBST);
	Builder.AddCheckbox(Msg::SetConfirmDetachVHD, &Opt.Confirm.DetachVHD);
	Builder.AddCheckbox(Msg::SetConfirmRemoveHotPlug, &Opt.Confirm.RemoveHotPlug);
	Builder.AddCheckbox(Msg::SetConfirmAllowReedit, &Opt.Confirm.AllowReedit);
	Builder.AddCheckbox(Msg::SetConfirmHistoryClear, &Opt.Confirm.HistoryClear);
	Builder.AddCheckbox(Msg::SetConfirmExit, &Opt.Confirm.Exit);
	Builder.AddOKCancel();

	Builder.ShowDialog();
}

void PluginsManagerSettings()
{
	DialogBuilder Builder(Msg::PluginsManagerSettingsTitle, L"PluginsManagerSettings");

	Builder.AddCheckbox(Msg::PluginsManagerOEMPluginsSupport, &Opt.LoadPlug.OEMPluginsSupport);
	Builder.AddCheckbox(Msg::PluginsManagerScanSymlinks, &Opt.LoadPlug.ScanSymlinks);
	Builder.AddText(Msg::PluginsManagerPersonalPath);
	Builder.AddEditField(&Opt.LoadPlug.strPersonalPluginsPath, 45, L"PersPath", DIF_EDITPATH);

	Builder.AddSeparator(Msg::PluginConfirmationTitle);
	DialogItemEx *ConfirmOFP = Builder.AddCheckbox(Msg::PluginsManagerOFP, &Opt.PluginConfirm.OpenFilePlugin);
	ConfirmOFP->Flags|=DIF_3STATE;
	DialogItemEx *StandardAssoc = Builder.AddCheckbox(Msg::PluginsManagerStdAssoc, &Opt.PluginConfirm.StandardAssociation);
	DialogItemEx *EvenIfOnlyOne = Builder.AddCheckbox(Msg::PluginsManagerEvenOne, &Opt.PluginConfirm.EvenIfOnlyOnePlugin);
	StandardAssoc->Indent(2);
	EvenIfOnlyOne->Indent(4);

	Builder.AddCheckbox(Msg::PluginsManagerSFL, &Opt.PluginConfirm.SetFindList);
	Builder.AddCheckbox(Msg::PluginsManagerPF, &Opt.PluginConfirm.Prefix);
	Builder.AddOKCancel();

	Builder.ShowDialog();
}


void SetDizConfig()
{
	DialogBuilder Builder(Msg::CfgDizTitle, L"FileDiz");

	Builder.AddText(Msg::CfgDizListNames);
	Builder.AddEditField(&Opt.Diz.strListNames, 65);
	Builder.AddSeparator();

	Builder.AddCheckbox(Msg::CfgDizSetHidden, &Opt.Diz.SetHidden);
	Builder.AddCheckbox(Msg::CfgDizROUpdate, &Opt.Diz.ROUpdate);
	DialogItemEx *StartPos = Builder.AddIntEditField(&Opt.Diz.StartPos, 2);
	Builder.AddTextAfter(StartPos, Msg::CfgDizStartPos);
	Builder.AddSeparator();

	static FarLangMsg DizOptions[] = { Msg::CfgDizNotUpdate, Msg::CfgDizUpdateIfDisplayed, Msg::CfgDizAlwaysUpdate };
	Builder.AddRadioButtons(&Opt.Diz.UpdateMode, 3, DizOptions);
	Builder.AddSeparator();

	Builder.AddCheckbox(Msg::CfgDizAnsiByDefault, &Opt.Diz.AnsiByDefault);
	Builder.AddCheckbox(Msg::CfgDizSaveInUTF, &Opt.Diz.SaveInUTF);
	Builder.AddOKCancel();
	Builder.ShowDialog();
}

void ViewerConfig(ViewerOptions &ViOpt,bool Local)
{
	DialogBuilder Builder(Msg::ViewConfigTitle, L"ViewerSettings");

	if (!Local)
	{
		Builder.AddCheckbox(Msg::ViewConfigExternalF3, &Opt.ViOpt.UseExternalViewer);
		Builder.AddText(Msg::ViewConfigExternalCommand);
		Builder.AddEditField(&Opt.strExternalViewer, 64, L"ExternalViewer", DIF_EDITPATH);
		Builder.AddSeparator(Msg::ViewConfigInternal);
	}

	Builder.StartColumns();
	Builder.AddCheckbox(Msg::ViewConfigPersistentSelection, &ViOpt.PersistentBlocks);
	DialogItemEx *SavePos = Builder.AddCheckbox(Msg::ViewConfigSavePos, &Opt.ViOpt.SavePos);
	DialogItemEx *TabSize = Builder.AddIntEditField(&ViOpt.TabSize, 3);
	Builder.AddTextAfter(TabSize, Msg::ViewConfigTabSize);
	if (!Local)
		Builder.AddCheckbox(Msg::ViewShowKeyBar, &ViOpt.ShowKeyBar);
	Builder.ColumnBreak();

	Builder.AddCheckbox(Msg::ViewConfigArrows, &ViOpt.ShowArrows);
	DialogItemEx *SaveShortPos = Builder.AddCheckbox(Msg::ViewConfigSaveShortPos, &Opt.ViOpt.SaveShortPos);
	Builder.LinkFlags(SavePos, SaveShortPos, DIF_DISABLE);
	Builder.AddCheckbox(Msg::ViewConfigScrollbar, &ViOpt.ShowScrollbar);
	if (!Local)
		Builder.AddCheckbox(Msg::ViewShowTitleBar, &ViOpt.ShowTitleBar);
	Builder.EndColumns();

	if (!Local)
	{
		Builder.AddEmptyLine();
		Builder.AddCheckbox(Msg::ViewAutoDetectCodePage, &ViOpt.AutoDetectCodePage);
		Builder.AddText(Msg::ViewConfigDefaultCodePage);
		Builder.AddCodePagesBox(&ViOpt.DefaultCodePage, 40, false, false);
	}
	Builder.AddOKCancel();
	if (Builder.ShowDialog())
	{
		if (ViOpt.TabSize<1 || ViOpt.TabSize>512)
			ViOpt.TabSize=8;
	}
}

void EditorConfig(EditorOptions &EdOpt,bool Local)
{
	DialogBuilder Builder(Msg::EditConfigTitle, L"EditorSettings");
	if (!Local)
	{
		Builder.AddCheckbox(Msg::EditConfigEditorF4, &Opt.EdOpt.UseExternalEditor);
		Builder.AddText(Msg::EditConfigEditorCommand);
		Builder.AddEditField(&Opt.strExternalEditor, 64, L"ExternalEditor", DIF_EDITPATH);
		Builder.AddSeparator(Msg::EditConfigInternal);
	}

	Builder.AddText(Msg::EditConfigExpandTabsTitle);
	DialogBuilderListItem ExpandTabsItems[] = {
		{ Msg::EditConfigDoNotExpandTabs, EXPAND_NOTABS },
		{ Msg::EditConfigExpandTabs, EXPAND_NEWTABS },
		{ Msg::EditConfigConvertAllTabsToSpaces, EXPAND_ALLTABS }
	};
	Builder.AddComboBox(&EdOpt.ExpandTabs, 64, ExpandTabsItems, 3, DIF_DROPDOWNLIST|DIF_LISTAUTOHIGHLIGHT|DIF_LISTWRAPMODE);

	Builder.StartColumns();
	Builder.AddCheckbox(Msg::EditConfigPersistentBlocks, &EdOpt.PersistentBlocks);
	DialogItemEx *SavePos = Builder.AddCheckbox(Msg::EditConfigSavePos, &EdOpt.SavePos);
	Builder.AddCheckbox(Msg::EditConfigAutoIndent, &EdOpt.AutoIndent);
	DialogItemEx *TabSize = Builder.AddIntEditField(&EdOpt.TabSize, 3);
	Builder.AddTextAfter(TabSize, Msg::EditConfigTabSize);
	Builder.AddCheckbox(Msg::EditShowWhiteSpace, &EdOpt.ShowWhiteSpace);
	if (!Local)
		Builder.AddCheckbox(Msg::EditShowKeyBar, &EdOpt.ShowKeyBar);
	Builder.ColumnBreak();

	Builder.AddCheckbox(Msg::EditConfigDelRemovesBlocks, &EdOpt.DelRemovesBlocks);
	DialogItemEx *SaveShortPos = Builder.AddCheckbox(Msg::EditConfigSaveShortPos, &EdOpt.SaveShortPos);
	Builder.LinkFlags(SavePos, SaveShortPos, DIF_DISABLE);
	Builder.AddCheckbox(Msg::EditCursorBeyondEnd, &EdOpt.CursorBeyondEOL);
	Builder.AddCheckbox(Msg::EditConfigScrollbar, &EdOpt.ShowScrollBar);
	Builder.AddCheckbox(Msg::EditConfigPickUpWord, &EdOpt.SearchPickUpWord);
	if (!Local)
		Builder.AddCheckbox(Msg::EditShowTitleBar, &EdOpt.ShowTitleBar);
	Builder.EndColumns();

	if (!Local)
	{
		Builder.AddEmptyLine();
		Builder.AddCheckbox(Msg::EditShareWrite, &EdOpt.EditOpenedForWrite);
		Builder.AddCheckbox(Msg::EditLockROFileModification, &EdOpt.ReadOnlyLock, 1);
		Builder.AddCheckbox(Msg::EditWarningBeforeOpenROFile, &EdOpt.ReadOnlyLock, 2);
		Builder.AddCheckbox(Msg::EditAutoDetectCodePage, &EdOpt.AutoDetectCodePage);
		Builder.AddText(Msg::EditConfigDefaultCodePage);
		Builder.AddCodePagesBox(&EdOpt.DefaultCodePage, 40, false, false);
	}

	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
		if (EdOpt.TabSize<1 || EdOpt.TabSize>512)
			EdOpt.TabSize=8;
	}
}


void NotificationsConfig(NotificationsOptions &NotifOpt)
{
	DialogBuilder Builder(Msg::NotifConfigTitle, L"NotificationsSettings");

	Builder.AddCheckbox(Msg::NotifConfigOnFileOperation, &NotifOpt.OnFileOperation);
	Builder.AddCheckbox(Msg::NotifConfigOnConsole, &NotifOpt.OnConsole);
	Builder.AddEmptyLine();
	Builder.AddCheckbox(Msg::NotifConfigOnlyIfBackground, &NotifOpt.OnlyIfBackground);
	Builder.AddOKCancel();

	if (Builder.ShowDialog())
	{
	}
}


void SetFolderInfoFiles()
{
	FARString strFolderInfoFiles;

	if (GetString(Msg::SetFolderInfoTitle,Msg::SetFolderInfoNames,L"FolderInfoFiles",
	              Opt.InfoPanel.strFolderInfoFiles,strFolderInfoFiles,L"OptMenu",FIB_ENABLEEMPTY|FIB_BUTTONS))
	{
		Opt.InfoPanel.strFolderInfoFiles = strFolderInfoFiles;

		if (CtrlObject->Cp()->LeftPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->LeftPanel->Update(0);

		if (CtrlObject->Cp()->RightPanel->GetType() == INFO_PANEL)
			CtrlObject->Cp()->RightPanel->Update(0);
	}
}


// Структура, описывающая всю конфигурацию(!)
static struct FARConfig
{
	int   IsSave;   // =1 - будет записываться в SaveConfig()
	DWORD ValType;  // REG_DWORD, REG_SZ, REG_BINARY
	const char *KeyName;
	const char *ValName;
	union {
		void      *ValPtr;   // адрес переменной, куда помещаем данные
		FARString *StrPtr;
	};
	DWORD DefDWord; // он же размер данных для REG_BINARY
	union {
	  const wchar_t *DefStr;   // строка по умолчанию
	  const BYTE    *DefArr;   // данные по умолчанию
	};

	constexpr FARConfig(int save, const char *key, const char *val, BYTE *trg, DWORD size, const BYTE *dflt) :
		IsSave(save),ValType(REG_BINARY),KeyName(key),ValName(val),ValPtr(trg),DefDWord(size),DefArr(dflt) {}
	constexpr FARConfig(int save, const char *key, const char *val, void *trg, DWORD dflt) :
		IsSave(save),ValType(REG_DWORD),KeyName(key),ValName(val),ValPtr(trg),DefDWord(dflt),DefStr(nullptr) {}
	constexpr FARConfig(int save, const char *key, const char *val, FARString *trg, const wchar_t *dflt) :
		IsSave(save),ValType(REG_SZ),KeyName(key),ValName(val),StrPtr(trg),DefDWord(0),DefStr(dflt) {}

} CFG[]=
{
	{1, NSecColors, "CurrentPalette",               Palette, SIZE_ARRAY_PALETTE, DefaultPalette},

	{1, NSecScreen, "Clock",                        &Opt.Clock, 1},
	{1, NSecScreen, "ViewerEditorClock",            &Opt.ViewerEditorClock, 0},
	{1, NSecScreen, "KeyBar",                       &Opt.ShowKeyBar, 1},
	{1, NSecScreen, "ScreenSaver",                  &Opt.ScreenSaver, 0},
	{1, NSecScreen, "ScreenSaverTime",              &Opt.ScreenSaverTime, 5},
	{0, NSecScreen, "DeltaXY",                      &Opt.ScrSize.DeltaXY, 0},

	{1, NSecCmdline, "UsePromptFormat",             &Opt.CmdLine.UsePromptFormat, 0},
	{1, NSecCmdline, "PromptFormat",                &Opt.CmdLine.strPromptFormat, L"$p$# "},
	{1, NSecCmdline, "UseShell",                    &Opt.CmdLine.UseShell, 0},
	{1, NSecCmdline, "Shell",                       &Opt.CmdLine.strShell, L"/bin/bash"},
	{1, NSecCmdline, "DelRemovesBlocks",            &Opt.CmdLine.DelRemovesBlocks, 1},
	{1, NSecCmdline, "EditBlock",                   &Opt.CmdLine.EditBlock, 0},
	{1, NSecCmdline, "AutoComplete",                &Opt.CmdLine.AutoComplete, 1},
	{1, NSecCmdline, "WaitKeypress",                &Opt.CmdLine.WaitKeypress, 1},
	{1, NSecCmdline, "VTLogLimit",                  &Opt.CmdLine.VTLogLimit, 5000},

	{1, NSecInterface, "Mouse",                     &Opt.Mouse, 1},
	{0, NSecInterface, "UseVk_oem_x",               &Opt.UseVk_oem_x, 1},
	{1, NSecInterface, "ShowMenuBar",               &Opt.ShowMenuBar, 0},
	{0, NSecInterface, "CursorSize1",               &Opt.CursorSize[0], 15},
	{0, NSecInterface, "CursorSize2",               &Opt.CursorSize[1], 10},
	{0, NSecInterface, "CursorSize3",               &Opt.CursorSize[2], 99},
	{0, NSecInterface, "CursorSize4",               &Opt.CursorSize[3], 99},
	{0, NSecInterface, "ShiftsKeyRules",            &Opt.ShiftsKeyRules, 1},
	{1, NSecInterface, "CtrlPgUp",                  &Opt.PgUpChangeDisk, 1},

	{1, NSecInterface, "ConsolePaintSharp",         &Opt.ConsolePaintSharp, 0},
	{1, NSecInterface, "ExclusiveCtrlLeft",         &Opt.ExclusiveCtrlLeft, 0},
	{1, NSecInterface, "ExclusiveCtrlRight",        &Opt.ExclusiveCtrlRight, 0},
	{1, NSecInterface, "ExclusiveAltLeft",          &Opt.ExclusiveAltLeft, 0},
	{1, NSecInterface, "ExclusiveAltRight",         &Opt.ExclusiveAltRight, 0},
	{1, NSecInterface, "ExclusiveWinLeft",          &Opt.ExclusiveWinLeft, 0},
	{1, NSecInterface, "ExclusiveWinRight",         &Opt.ExclusiveWinRight, 0},

	{1, NSecInterface, "OSC52ClipSet",              &Opt.OSC52ClipSet, 0},
	{1, NSecInterface, "TTYPaletteOverride",        &Opt.TTYPaletteOverride, 1},

	{0, NSecInterface, "ShowTimeoutDelFiles",       &Opt.ShowTimeoutDelFiles, 50},
	{0, NSecInterface, "ShowTimeoutDACLFiles",      &Opt.ShowTimeoutDACLFiles, 50},
	{0, NSecInterface, "FormatNumberSeparators",    &Opt.FormatNumberSeparators, 0},
	{1, NSecInterface, "CopyShowTotal",             &Opt.CMOpt.CopyShowTotal, 1},
	{1, NSecInterface, "DelShowTotal",              &Opt.DelOpt.DelShowTotal, 0},
	{1, NSecInterface, "WindowTitle",               &Opt.strWindowTitle, L"%State - FAR2M %Ver %Backend %User@%Host"}, // %Platform
	{1, NSecInterfaceCompletion, "Exceptions",      &Opt.AutoComplete.Exceptions, L"git*reset*--hard;*://*:*@*"},
	{1, NSecInterfaceCompletion, "ShowList",        &Opt.AutoComplete.ShowList, 1},
	{1, NSecInterfaceCompletion, "ModalList",       &Opt.AutoComplete.ModalList, 0},
	{1, NSecInterfaceCompletion, "Append",          &Opt.AutoComplete.AppendCompletion, 0},

	{1, NSecViewer, "ExternalViewerName",           &Opt.strExternalViewer, L""},
	{1, NSecViewer, "UseExternalViewer",            &Opt.ViOpt.UseExternalViewer, 0},
	{1, NSecViewer, "SaveViewerPos",                &Opt.ViOpt.SavePos, 1},
	{1, NSecViewer, "SaveViewerShortPos",           &Opt.ViOpt.SaveShortPos, 1},
	{1, NSecViewer, "AutoDetectCodePage",           &Opt.ViOpt.AutoDetectCodePage, 0},
	{1, NSecViewer, "SearchRegexp",                 &Opt.ViOpt.SearchRegexp, 0},

	{1, NSecViewer, "TabSize",                      &Opt.ViOpt.TabSize, 8},
	{1, NSecViewer, "ShowKeyBar",                   &Opt.ViOpt.ShowKeyBar, 1},
	{1, NSecViewer, "ShowTitleBar",                 &Opt.ViOpt.ShowTitleBar, 1},
	{1, NSecViewer, "ShowArrows",                   &Opt.ViOpt.ShowArrows, 1},
	{1, NSecViewer, "ShowScrollbar",                &Opt.ViOpt.ShowScrollbar, 0},
	{1, NSecViewer, "IsWrap",                       &Opt.ViOpt.ViewerIsWrap, 1},
	{1, NSecViewer, "Wrap",                         &Opt.ViOpt.ViewerWrap, 0},
	{1, NSecViewer, "PersistentBlocks",             &Opt.ViOpt.PersistentBlocks, 0},
	{1, NSecViewer, "DefaultCodePage",              &Opt.ViOpt.DefaultCodePage, CP_UTF8},

	{1, NSecDialog, "EditHistory",                  &Opt.Dialogs.EditHistory, 1},
	{1, NSecDialog, "EditBlock",                    &Opt.Dialogs.EditBlock, 0},
	{1, NSecDialog, "AutoComplete",                 &Opt.Dialogs.AutoComplete, 1},
	{1, NSecDialog, "EULBsClear",                   &Opt.Dialogs.EULBsClear, 0},
	{0, NSecDialog, "SelectFromHistory",            &Opt.Dialogs.SelectFromHistory, 0},
	{0, NSecDialog, "EditLine",                     &Opt.Dialogs.EditLine, 0},
	{1, NSecDialog, "MouseButton",                  &Opt.Dialogs.MouseButton, 0xFFFF},
	{1, NSecDialog, "DelRemovesBlocks",             &Opt.Dialogs.DelRemovesBlocks, 1},
	{0, NSecDialog, "CBoxMaxHeight",                &Opt.Dialogs.CBoxMaxHeight, 24},

	{1, NSecEditor, "ExternalEditorName",           &Opt.strExternalEditor, L""},
	{1, NSecEditor, "UseExternalEditor",            &Opt.EdOpt.UseExternalEditor, 0},
	{1, NSecEditor, "ExpandTabs",                   &Opt.EdOpt.ExpandTabs, 0},
	{1, NSecEditor, "TabSize",                      &Opt.EdOpt.TabSize, 8},
	{1, NSecEditor, "PersistentBlocks",             &Opt.EdOpt.PersistentBlocks, 0},
	{1, NSecEditor, "DelRemovesBlocks",             &Opt.EdOpt.DelRemovesBlocks, 1},
	{1, NSecEditor, "AutoIndent",                   &Opt.EdOpt.AutoIndent, 0},
	{1, NSecEditor, "SaveEditorPos",                &Opt.EdOpt.SavePos, 1},
	{1, NSecEditor, "SaveEditorShortPos",           &Opt.EdOpt.SaveShortPos, 1},
	{1, NSecEditor, "AutoDetectCodePage",           &Opt.EdOpt.AutoDetectCodePage, 0},
	{1, NSecEditor, "EditorCursorBeyondEOL",        &Opt.EdOpt.CursorBeyondEOL, 1},
	{1, NSecEditor, "ReadOnlyLock",                 &Opt.EdOpt.ReadOnlyLock, 0},
	{0, NSecEditor, "EditorUndoSize",               &Opt.EdOpt.UndoSize, 0},
	{0, NSecEditor, "WordDiv",                      &Opt.strWordDiv, WordDiv0},
	{0, NSecEditor, "BSLikeDel",                    &Opt.EdOpt.BSLikeDel, 1},
	{0, NSecEditor, "EditorF7Rules",                &Opt.EdOpt.F7Rules, 1},
	{0, NSecEditor, "FileSizeLimit",                &Opt.EdOpt.FileSizeLimitLo, 0},
	{0, NSecEditor, "FileSizeLimitHi",              &Opt.EdOpt.FileSizeLimitHi, 0},
	{0, NSecEditor, "CharCodeBase",                 &Opt.EdOpt.CharCodeBase, 1},
	{0, NSecEditor, "AllowEmptySpaceAfterEof",      &Opt.EdOpt.AllowEmptySpaceAfterEof, 0},
	{1, NSecEditor, "DefaultCodePage",              &Opt.EdOpt.DefaultCodePage, CP_UTF8},
	{1, NSecEditor, "ShowKeyBar",                   &Opt.EdOpt.ShowKeyBar, 1},
	{1, NSecEditor, "ShowTitleBar",                 &Opt.EdOpt.ShowTitleBar, 1},
	{1, NSecEditor, "ShowScrollBar",                &Opt.EdOpt.ShowScrollBar, 0},
	{1, NSecEditor, "EditOpenedForWrite",           &Opt.EdOpt.EditOpenedForWrite, 1},
	{1, NSecEditor, "SearchSelFound",               &Opt.EdOpt.SearchSelFound, 0},
	{1, NSecEditor, "SearchRegexp",                 &Opt.EdOpt.SearchRegexp, 0},
	{1, NSecEditor, "SearchPickUpWord",             &Opt.EdOpt.SearchPickUpWord, 0},
	{1, NSecEditor, "ShowWhiteSpace",               &Opt.EdOpt.ShowWhiteSpace, 0},

	{1, NSecNotifications, "OnFileOperation",       &Opt.NotifOpt.OnFileOperation, 1},
	{1, NSecNotifications, "OnConsole",             &Opt.NotifOpt.OnConsole, 1},
	{1, NSecNotifications, "OnlyIfBackground",      &Opt.NotifOpt.OnlyIfBackground, 1},

	{0, NSecXLat, "Flags",                          &Opt.XLat.Flags, XLAT_SWITCHKEYBLAYOUT|XLAT_CONVERTALLCMDLINE},
	{1, NSecXLat, "EnableForFastFileFind",          &Opt.XLat.EnableForFastFileFind, 1},
	{1, NSecXLat, "EnableForDialogs",               &Opt.XLat.EnableForDialogs, 1},
	{1, NSecXLat, "WordDivForXlat",                 &Opt.XLat.strWordDivForXlat, WordDivForXlat0},
	{1, NSecXLat, "XLat",                           &Opt.XLat.XLat, L"ru:qwerty-йцукен"},

	{1, NSecSavedHistory, NParamHistoryCount,       &Opt.HistoryCount, 512},
	{1, NSecSavedFolderHistory, NParamHistoryCount, &Opt.FoldersHistoryCount, 512},
	{1, NSecSavedViewHistory, NParamHistoryCount,   &Opt.ViewHistoryCount, 512},
	{1, NSecSavedDialogHistory, NParamHistoryCount, &Opt.DialogsHistoryCount, 512},

	{1, NSecSystem, "SaveHistory",                  &Opt.SaveHistory, 1},
	{1, NSecSystem, "SaveFoldersHistory",           &Opt.SaveFoldersHistory, 1},
	{0, NSecSystem, "SavePluginFoldersHistory",     &Opt.SavePluginFoldersHistory, 0},
	{1, NSecSystem, "SaveViewHistory",              &Opt.SaveViewHistory, 1},
	{1, NSecSystem, "AutoSaveSetup",                &Opt.AutoSaveSetup, 0},
	{1, NSecSystem, "DeleteToRecycleBin",           &Opt.DeleteToRecycleBin, 0},
	{1, NSecSystem, "DeleteToRecycleBinKillLink",   &Opt.DeleteToRecycleBinKillLink, 1},
	{0, NSecSystem, "WipeSymbol",                   &Opt.WipeSymbol, 0},
	{1, NSecSystem, "SudoEnabled",                  &Opt.SudoEnabled, 1},
	{1, NSecSystem, "SudoConfirmModify",            &Opt.SudoConfirmModify, 1},
	{1, NSecSystem, "SudoPasswordExpiration",       &Opt.SudoPasswordExpiration, 15*60},

	{1, NSecSystem, "UseCOW",                       &Opt.CMOpt.SparseFiles, 0},
	{1, NSecSystem, "SparseFiles",                  &Opt.CMOpt.SparseFiles, 0},
	{1, NSecSystem, "HowCopySymlink",               &Opt.CMOpt.HowCopySymlink, 1},
	{1, NSecSystem, "WriteThrough",                 &Opt.CMOpt.WriteThrough, 0},
	{1, NSecSystem, "CopyXAttr",                    &Opt.CMOpt.CopyXAttr, 0},
	{0, NSecSystem, "CopyAccessMode",               &Opt.CMOpt.CopyAccessMode, 1},
	{1, NSecSystem, "MultiCopy",                    &Opt.CMOpt.MultiCopy, 0},
	{1, NSecSystem, "CopyTimeRule",                 &Opt.CMOpt.CopyTimeRule, 3},

	{1, NSecSystem, "InactivityExit",               &Opt.InactivityExit, 0},
	{1, NSecSystem, "InactivityExitTime",           &Opt.InactivityExitTime, 15},
	{1, NSecSystem, "DriveMenuMode2",               &Opt.ChangeDriveMode, (DWORD)-1},
	{1, NSecSystem, "DriveDisconnetMode",           &Opt.ChangeDriveDisconnetMode, 1},

	{1, NSecSystem, "DriveExceptions",              &Opt.ChangeDriveExceptions,
		L"/System/*;/proc;/proc/*;/sys;/sys/*;/dev;/dev/*;/run;/run/*;/tmp;/snap;/snap/*;"
		"/private;/private/*;/var/lib/lxcfs;/var/snap/*;/var/spool/cron"},
	{1, NSecSystem, "DriveColumn2",                 &Opt.ChangeDriveColumn2, L"$U/$T"},
	{1, NSecSystem, "DriveColumn3",                 &Opt.ChangeDriveColumn3, L"$S$D"},

	{1, NSecSystem, "AutoUpdateRemoteDrive",        &Opt.AutoUpdateRemoteDrive, 1},
	{1, NSecSystem, "FileSearchMode",               &Opt.FindOpt.FileSearchMode, FINDAREA_FROM_CURRENT},
	{0, NSecSystem, "CollectFiles",                 &Opt.FindOpt.CollectFiles, 1},
	{1, NSecSystem, "SearchInFirstSize",            &Opt.FindOpt.strSearchInFirstSize, L""},
	{1, NSecSystem, "FindAlternateStreams",         &Opt.FindOpt.FindAlternateStreams, 0},
	{1, NSecSystem, "SearchOutFormat",              &Opt.FindOpt.strSearchOutFormat, L"D,S,A"},
	{1, NSecSystem, "SearchOutFormatWidth",         &Opt.FindOpt.strSearchOutFormatWidth, L"14,13,0"},
	{1, NSecSystem, "FindFolders",                  &Opt.FindOpt.FindFolders, 1},
	{1, NSecSystem, "FindSymLinks",                 &Opt.FindOpt.FindSymLinks, 1},
	{1, NSecSystem, "UseFilterInSearch",            &Opt.FindOpt.UseFilter, 0},
	{1, NSecSystem, "FindCodePage",                 &Opt.FindCodePage, CP_AUTODETECT},
	{0, NSecSystem, "CmdHistoryRule",               &Opt.CmdHistoryRule, 0},
	{0, NSecSystem, "SetAttrFolderRules",           &Opt.SetAttrFolderRules, 1},
	{0, NSecSystem, "MaxPositionCache",             &Opt.MaxPositionCache, POSCACHE_MAX_ELEMENTS},
	{0, NSecSystem, "ConsoleDetachKey",             &strKeyNameConsoleDetachKey, L"CtrlAltTab"},
	{0, NSecSystem, "SilentLoadPlugin",             &Opt.LoadPlug.SilentLoadPlugin, 0},
	{1, NSecSystem, "OEMPluginsSupport",            &Opt.LoadPlug.OEMPluginsSupport, 1},
	{1, NSecSystem, "ScanSymlinks",                 &Opt.LoadPlug.ScanSymlinks, 1},
	{1, NSecSystem, "MultiMakeDir",                 &Opt.MultiMakeDir, 0},
	{0, NSecSystem, "MsWheelDelta",                 &Opt.MsWheelDelta, 1},
	{0, NSecSystem, "MsWheelDeltaView",             &Opt.MsWheelDeltaView, 1},
	{0, NSecSystem, "MsWheelDeltaEdit",             &Opt.MsWheelDeltaEdit, 1},
	{0, NSecSystem, "MsWheelDeltaHelp",             &Opt.MsWheelDeltaHelp, 1},
	{0, NSecSystem, "MsHWheelDelta",                &Opt.MsHWheelDelta, 1},
	{0, NSecSystem, "MsHWheelDeltaView",            &Opt.MsHWheelDeltaView, 1},
	{0, NSecSystem, "MsHWheelDeltaEdit",            &Opt.MsHWheelDeltaEdit, 1},
	{0, NSecSystem, "SubstNameRule",                &Opt.SubstNameRule, 2},
	{0, NSecSystem, "ShowCheckingFile",             &Opt.ShowCheckingFile, 0},
	{0, NSecSystem, "DelThreadPriority",            &Opt.DelThreadPriority, 0},
	{0, NSecSystem, "QuotedSymbols",                &Opt.strQuotedSymbols, L" $&()[]{};|*?!'`\"\\\xA0"}, //xA0 => 160 =>oem(0xFF)
	{0, NSecSystem, "QuotedName",                   &Opt.QuotedName, QUOTEDNAME_INSERT},
	//{0, NSecSystem, "CPAJHefuayor",               &Opt.strCPAJHefuayor, 0},
	{0, NSecSystem, "PluginMaxReadData",            &Opt.PluginMaxReadData, 0x40000},
	{0, NSecSystem, "CASRule",                      &Opt.CASRule, 0xFFFFFFFFU},
	{0, NSecSystem, "AllCtrlAltShiftRule",          &Opt.AllCtrlAltShiftRule, 0x0000FFFF},
	{1, NSecSystem, "ScanJunction",                 &Opt.ScanJunction, 1},
	{1, NSecSystem, "OnlyFilesSize",                &Opt.OnlyFilesSize, 0},
	{0, NSecSystem, "UsePrintManager",              &Opt.UsePrintManager, 1},
	{0, NSecSystem, "WindowMode",                   &Opt.WindowMode, 0},

	{0, NSecSystemNowell, "MoveRO",                 &Opt.Nowell.MoveRO, 1},

	{0, NSecSystemExecutor, "RestoreCP",            &Opt.RestoreCPAfterExecute, 1},
	{0, NSecSystemExecutor, "UseAppPath",           &Opt.ExecuteUseAppPath, 1},
	{0, NSecSystemExecutor, "ShowErrorMessage",     &Opt.ExecuteShowErrorMessage, 1},
	{0, NSecSystemExecutor, "FullTitle",            &Opt.ExecuteFullTitle, 0},
	{0, NSecSystemExecutor, "SilentExternal",       &Opt.ExecuteSilentExternal, 0},

	{0, NSecPanelTree, "MinTreeCount",              &Opt.Tree.MinTreeCount, 4},
	{0, NSecPanelTree, "TreeFileAttr",              &Opt.Tree.TreeFileAttr, FILE_ATTRIBUTE_HIDDEN},
	{0, NSecPanelTree, "LocalDisk",                 &Opt.Tree.LocalDisk, 2},
	{0, NSecPanelTree, "NetDisk",                   &Opt.Tree.NetDisk, 2},
	{0, NSecPanelTree, "RemovableDisk",             &Opt.Tree.RemovableDisk, 2},
	{0, NSecPanelTree, "NetPath",                   &Opt.Tree.NetPath, 2},
	{1, NSecPanelTree, "AutoChangeFolder",          &Opt.Tree.AutoChangeFolder, 0}, // ???

	{0, NSecHelp, "ActivateURL",                    &Opt.HelpURLRules, 1},

	{1, NSecLanguage, "Help",                       &Opt.strHelpLanguage, L"English"},
	{1, NSecLanguage, "Main",                       &Opt.strLanguage, L"English"},

	{1, NSecConfirmations, "Copy",                  &Opt.Confirm.Copy, 1},
	{1, NSecConfirmations, "Move",                  &Opt.Confirm.Move, 1},
	{1, NSecConfirmations, "RO",                    &Opt.Confirm.RO, 1},
	{1, NSecConfirmations, "Drag",                  &Opt.Confirm.Drag, 1},
	{1, NSecConfirmations, "Delete",                &Opt.Confirm.Delete, 1},
	{1, NSecConfirmations, "DeleteFolder",          &Opt.Confirm.DeleteFolder, 1},
	{1, NSecConfirmations, "Esc",                   &Opt.Confirm.Esc, 1},
	{1, NSecConfirmations, "RemoveConnection",      &Opt.Confirm.RemoveConnection, 1},
	{1, NSecConfirmations, "RemoveSUBST",           &Opt.Confirm.RemoveSUBST, 1},
	{1, NSecConfirmations, "DetachVHD",             &Opt.Confirm.DetachVHD, 1},
	{1, NSecConfirmations, "RemoveHotPlug",         &Opt.Confirm.RemoveHotPlug, 1},
	{1, NSecConfirmations, "AllowReedit",           &Opt.Confirm.AllowReedit, 1},
	{1, NSecConfirmations, "HistoryClear",          &Opt.Confirm.HistoryClear, 1},
	{1, NSecConfirmations, "Exit",                  &Opt.Confirm.Exit, 1},
	{0, NSecConfirmations, "EscTwiceToInterrupt",   &Opt.Confirm.EscTwiceToInterrupt, 0},

	{1, NSecPluginConfirmations, "OpenFilePlugin",  &Opt.PluginConfirm.OpenFilePlugin, 0},
	{1, NSecPluginConfirmations, "StandardAssociation", &Opt.PluginConfirm.StandardAssociation, 0},
	{1, NSecPluginConfirmations, "EvenIfOnlyOnePlugin", &Opt.PluginConfirm.EvenIfOnlyOnePlugin, 0},
	{1, NSecPluginConfirmations, "SetFindList",     &Opt.PluginConfirm.SetFindList, 0},
	{1, NSecPluginConfirmations, "Prefix",          &Opt.PluginConfirm.Prefix, 0},

	{0, NSecPanel, "ShellRightLeftArrowsRule",      &Opt.ShellRightLeftArrowsRule, 0},
	{1, NSecPanel, "ShowHidden",                    &Opt.ShowHidden, 1},
	{1, NSecPanel, "Highlight",                     &Opt.Highlight, 1},
	{1, NSecPanel, "SortFolderExt",                 &Opt.SortFolderExt, 0},
	{1, NSecPanel, "SelectFolders",                 &Opt.SelectFolders, 0},
	{1, NSecPanel, "ReverseSort",                   &Opt.ReverseSort, 1},
	{0, NSecPanel, "RightClickRule",                &Opt.PanelRightClickRule, 2},
	{0, NSecPanel, "CtrlFRule",                     &Opt.PanelCtrlFRule, 1},
	{0, NSecPanel, "CtrlAltShiftRule",              &Opt.PanelCtrlAltShiftRule, 0},
	{0, NSecPanel, "RememberLogicalDrives",         &Opt.RememberLogicalDrives, 0},
	{1, NSecPanel, "AutoUpdateLimit",               &Opt.AutoUpdateLimit, 0},

	{1, NSecPanelLeft, "Type",                      &Opt.LeftPanel.Type, 0},
	{1, NSecPanelLeft, "Visible",                   &Opt.LeftPanel.Visible, 1},
	{1, NSecPanelLeft, "Focus",                     &Opt.LeftPanel.Focus, 1},
	{1, NSecPanelLeft, "ViewMode",                  &Opt.LeftPanel.ViewMode, 2},
	{1, NSecPanelLeft, "SortMode",                  &Opt.LeftPanel.SortMode, 1},
	{1, NSecPanelLeft, "SortOrder",                 &Opt.LeftPanel.SortOrder, 1},
	{1, NSecPanelLeft, "SortGroups",                &Opt.LeftPanel.SortGroups, 0},
	{1, NSecPanelLeft, "NumericSort",               &Opt.LeftPanel.NumericSort, 0},
	{1, NSecPanelLeft, "CaseSensitiveSortNix",      &Opt.LeftPanel.CaseSensitiveSort, 1},
	{1, NSecPanelLeft, "Folder",                    &Opt.strLeftFolder, L""},
	{1, NSecPanelLeft, "CurFile",                   &Opt.strLeftCurFile, L""},
	{1, NSecPanelLeft, "SelectedFirst",             &Opt.LeftSelectedFirst, 0},
	{1, NSecPanelLeft, "DirectoriesFirst",          &Opt.LeftPanel.DirectoriesFirst, 1},

	{1, NSecPanelRight, "Type",                     &Opt.RightPanel.Type, 0},
	{1, NSecPanelRight, "Visible",                  &Opt.RightPanel.Visible, 1},
	{1, NSecPanelRight, "Focus",                    &Opt.RightPanel.Focus, 0},
	{1, NSecPanelRight, "ViewMode",                 &Opt.RightPanel.ViewMode, 2},
	{1, NSecPanelRight, "SortMode",                 &Opt.RightPanel.SortMode, 1},
	{1, NSecPanelRight, "SortOrder",                &Opt.RightPanel.SortOrder, 1},
	{1, NSecPanelRight, "SortGroups",               &Opt.RightPanel.SortGroups, 0},
	{1, NSecPanelRight, "NumericSort",              &Opt.RightPanel.NumericSort, 0},
	{1, NSecPanelRight, "CaseSensitiveSortNix",     &Opt.RightPanel.CaseSensitiveSort, 1},
	{1, NSecPanelRight, "Folder",                   &Opt.strRightFolder, L""},
	{1, NSecPanelRight, "CurFile",                  &Opt.strRightCurFile, L""},
	{1, NSecPanelRight, "SelectedFirst",            &Opt.RightSelectedFirst, 0},
	{1, NSecPanelRight, "DirectoriesFirst",         &Opt.RightPanel.DirectoriesFirst, 1},

	{1, NSecPanelLayout, "ColumnTitles",            &Opt.ShowColumnTitles, 1},
	{1, NSecPanelLayout, "StatusLine",              &Opt.ShowPanelStatus, 1},
	{1, NSecPanelLayout, "TotalInfo",               &Opt.ShowPanelTotals, 1},
	{1, NSecPanelLayout, "FreeInfo",                &Opt.ShowPanelFree, 0},
	{1, NSecPanelLayout, "Scrollbar",               &Opt.ShowPanelScrollbar, 0},
	{0, NSecPanelLayout, "ScrollbarMenu",           &Opt.ShowMenuScrollbar, 1},
	{1, NSecPanelLayout, "ScreensNumber",           &Opt.ShowScreensNumber, 1},
	{1, NSecPanelLayout, "SortMode",                &Opt.ShowSortMode, 1},

	{1, NSecLayout, "LeftHeightDecrement",          &Opt.LeftHeightDecrement, 0},
	{1, NSecLayout, "RightHeightDecrement",         &Opt.RightHeightDecrement, 0},
	{1, NSecLayout, "WidthDecrement",               &Opt.WidthDecrement, 0},
	{1, NSecLayout, "FullscreenHelp",               &Opt.FullScreenHelp, 0},

	{1, NSecDescriptions, "ListNames",              &Opt.Diz.strListNames, L"Descript.ion,Files.bbs"},
	{1, NSecDescriptions, "UpdateMode",             &Opt.Diz.UpdateMode, DIZ_UPDATE_IF_DISPLAYED},
	{1, NSecDescriptions, "ROUpdate",               &Opt.Diz.ROUpdate, 0},
	{1, NSecDescriptions, "SetHidden",              &Opt.Diz.SetHidden, 1},
	{1, NSecDescriptions, "StartPos",               &Opt.Diz.StartPos, 0},
	{1, NSecDescriptions, "AnsiByDefault",          &Opt.Diz.AnsiByDefault, 0},
	{1, NSecDescriptions, "SaveInUTF",              &Opt.Diz.SaveInUTF, 0},

	{0, NSecKeyMacros, "DateFormat",                &Opt.Macro.strDateFormat, L"%a %b %d %H:%M:%S %Z %Y"},
	{0, NSecKeyMacros, "CONVFMT",                   &Opt.Macro.strMacroCONVFMT, L"%.6g"},

	{0, NSecPolicies, "ShowHiddenDrives",           &Opt.Policies.ShowHiddenDrives, 1},
	{0, NSecPolicies, "DisabledOptions",            &Opt.Policies.DisabledOptions, 0},


	{0, NSecSystem, "ExcludeCmdHistory",            &Opt.ExcludeCmdHistory, 0}, //AN

	{1, NSecCodePages, "CPMenuMode2",               &Opt.CPMenuMode, 1},

	{1, NSecSystem, "FolderInfo",                   &Opt.InfoPanel.strFolderInfoFiles, L"DirInfo,File_Id.diz,Descript.ion,ReadMe.*,Read.Me"},

	{1, NSecVMenu, "LBtnClick",                     &Opt.VMenu.LBtnClick, VMENUCLICK_CANCEL},
	{1, NSecVMenu, "RBtnClick",                     &Opt.VMenu.RBtnClick, VMENUCLICK_CANCEL},
	{1, NSecVMenu, "MBtnClick",                     &Opt.VMenu.MBtnClick, VMENUCLICK_APPLY},
};

static bool g_config_ready = false;

void ReadConfig()
{
	FARString strKeyNameFromReg;
	FARString strPersonalPluginsPath;
	size_t I;

	ConfigReader cfg_reader;

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	cfg_reader.SelectSection(NSecSystem);
	Opt.LoadPlug.strPersonalPluginsPath = cfg_reader.GetString("PersonalPluginsPath", L"");
	bool ExplicitWindowMode=Opt.WindowMode!=FALSE;
	//Opt.LCIDSort=LOCALE_USER_DEFAULT; // проинициализируем на всякий случай
	/* *************************************************** </ПРЕПРОЦЕССЫ> */

	for (I=0; I < ARRAYSIZE(CFG); ++I)
	{
		cfg_reader.SelectSection(CFG[I].KeyName);
		switch (CFG[I].ValType)
		{
			case REG_DWORD:
				if ((int *)CFG[I].ValPtr == &Opt.Confirm.Exit) {
					// when background mode available then exit dialog allows also switch to background
					// so saved settings must differ for that two modes
					CFG[I].ValName = WINPORT(ConsoleBackgroundMode)(FALSE) ? "ExitOrBknd" : "Exit";
				}
				*(unsigned int *)CFG[I].ValPtr = cfg_reader.GetUInt(CFG[I].ValName, (unsigned int)CFG[I].DefDWord);
				break;
			case REG_SZ:
				*CFG[I].StrPtr = cfg_reader.GetString(CFG[I].ValName, CFG[I].DefStr);
				break;
			case REG_BINARY:
				int Size = cfg_reader.GetBytes((BYTE*)CFG[I].ValPtr, CFG[I].DefDWord, CFG[I].ValName, CFG[I].DefArr);
				if (Size > 0 && Size < (int)CFG[I].DefDWord)
					memset(((BYTE*)CFG[I].ValPtr)+Size,0,CFG[I].DefDWord-Size);

				break;
		}
	}

	/* <ПОСТПРОЦЕССЫ> *************************************************** */

	SanitizeHistoryCounts();

	if (Opt.ShowMenuBar)
		Opt.ShowMenuBar=1;

	if (Opt.PluginMaxReadData < 0x1000) // || Opt.PluginMaxReadData > 0x80000)
		Opt.PluginMaxReadData=0x20000;

	if(ExplicitWindowMode)
	{
		Opt.WindowMode=TRUE;
	}

	Opt.HelpTabSize=8; // пока жестко пропишем...
	//   Уточняем алгоритм "взятия" палитры.
	for (I=COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR+1;
	        I < (COL_LASTPALETTECOLOR-COL_FIRSTPALETTECOLOR);
	        ++I)
	{
		if (!Palette[I])
		{
			if (!Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR])
				Palette[I]=DefaultPalette[I];
			else if (Palette[COL_PRIVATEPOSITION_FOR_DIF165ABOVE-COL_FIRSTPALETTECOLOR] == 1)
				Palette[I]=BlackPalette[I];

			/*
			else
			  в других случаях нифига ничего не делаем, т.к.
			  есть другие палитры...
			*/
		}
	}

	Opt.ViOpt.ViewerIsWrap&=1;
	Opt.ViOpt.ViewerWrap&=1;

	// Исключаем случайное стирание разделителей ;-)
	if (Opt.strWordDiv.IsEmpty())
		Opt.strWordDiv = WordDiv0;

	// Исключаем случайное стирание разделителей
	if (Opt.XLat.strWordDivForXlat.IsEmpty())
		Opt.XLat.strWordDivForXlat = WordDivForXlat0;

	Opt.PanelRightClickRule%=3;
	Opt.PanelCtrlAltShiftRule%=3;
	Opt.ConsoleDetachKey=KeyNameToKey(strKeyNameConsoleDetachKey);

	if (Opt.EdOpt.TabSize<1 || Opt.EdOpt.TabSize>512)
		Opt.EdOpt.TabSize=8;

	if (Opt.ViOpt.TabSize<1 || Opt.ViOpt.TabSize>512)
		Opt.ViOpt.TabSize=8;

	cfg_reader.SelectSection(NSecKeyMacros);

	strKeyNameFromReg = cfg_reader.GetString("KeyRecordCtrlDot", szCtrlDot);

	if ((Opt.Macro.KeyMacroCtrlDot=KeyNameToKey(strKeyNameFromReg)) == KEY_INVALID)
		Opt.Macro.KeyMacroCtrlDot=KEY_CTRLDOT;

	strKeyNameFromReg = cfg_reader.GetString("KeyRecordCtrlShiftDot", szCtrlShiftDot);

	if ((Opt.Macro.KeyMacroCtrlShiftDot=KeyNameToKey(strKeyNameFromReg)) == KEY_INVALID)
		Opt.Macro.KeyMacroCtrlShiftDot=KEY_CTRLSHIFTDOT;

	Opt.EdOpt.strWordDiv = Opt.strWordDiv;
	FileList::ReadPanelModes(cfg_reader);

	{
		//cfg_reader.SelectSection(NSecXLat);
		AllXlats xlats;
		std::string SetXLat;
		for (const auto &xlat : xlats) {
			if (Opt.XLat.XLat == xlat) {
				SetXLat.clear();
				break;
			}
			if (SetXLat.empty()) {
				SetXLat = xlat;
			}
		}
		if (!SetXLat.empty()) {
			Opt.XLat.XLat = SetXLat;
		}
	}

	Opt.FindOpt.OutColumns.clear();

	if (!Opt.FindOpt.strSearchOutFormat.IsEmpty())
	{
		if (Opt.FindOpt.strSearchOutFormatWidth.IsEmpty())
			Opt.FindOpt.strSearchOutFormatWidth=L"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";
		TextToViewSettings(Opt.FindOpt.strSearchOutFormat.CPtr(),Opt.FindOpt.strSearchOutFormatWidth.CPtr(),
		                   Opt.FindOpt.OutColumns);
	}

	FileFilter::InitFilter(cfg_reader);

	g_config_ready = true;
	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

void ApplyConfig()
{
	ApplySudoConfiguration();
	ApplyConsoleTweaks();
}

void AssertConfigLoaded()
{
	if (!g_config_ready)
	{
		fprintf(stderr, "%s: oops\n", __FUNCTION__);
		abort();
	}
}

void SaveConfig(int Ask)
{
	if (Opt.Policies.DisabledOptions&0x20000) // Bit 17 - Сохранить параметры
		return;

	if (Ask && Message(0,2,Msg::SaveSetupTitle,Msg::SaveSetupAsk1,Msg::SaveSetupAsk2,Msg::SaveSetup,Msg::Cancel))
		return;

	WINPORT(SaveConsoleWindowState)();

	/* <ПРЕПРОЦЕССЫ> *************************************************** */
	Panel *LeftPanel=CtrlObject->Cp()->LeftPanel;
	Panel *RightPanel=CtrlObject->Cp()->RightPanel;
	Opt.LeftPanel.Focus=LeftPanel->GetFocus();
	Opt.LeftPanel.Visible=LeftPanel->IsVisible();
	Opt.RightPanel.Focus=RightPanel->GetFocus();
	Opt.RightPanel.Visible=RightPanel->IsVisible();

	if (LeftPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.LeftPanel.Type=LeftPanel->GetType();
		Opt.LeftPanel.ViewMode=LeftPanel->GetViewMode();
		Opt.LeftPanel.SortMode=LeftPanel->GetSortMode();
		Opt.LeftPanel.SortOrder=LeftPanel->GetSortOrder();
		Opt.LeftPanel.SortGroups=LeftPanel->GetSortGroups();
		Opt.LeftPanel.NumericSort=LeftPanel->GetNumericSort();
		Opt.LeftPanel.CaseSensitiveSort=LeftPanel->GetCaseSensitiveSort();
		Opt.LeftSelectedFirst=LeftPanel->GetSelectedFirstMode();
		Opt.LeftPanel.DirectoriesFirst=LeftPanel->GetDirectoriesFirst();
	}

	LeftPanel->GetCurDir(Opt.strLeftFolder);
	LeftPanel->GetCurBaseName(Opt.strLeftCurFile);

	if (RightPanel->GetMode()==NORMAL_PANEL)
	{
		Opt.RightPanel.Type=RightPanel->GetType();
		Opt.RightPanel.ViewMode=RightPanel->GetViewMode();
		Opt.RightPanel.SortMode=RightPanel->GetSortMode();
		Opt.RightPanel.SortOrder=RightPanel->GetSortOrder();
		Opt.RightPanel.SortGroups=RightPanel->GetSortGroups();
		Opt.RightPanel.NumericSort=RightPanel->GetNumericSort();
		Opt.RightPanel.CaseSensitiveSort=RightPanel->GetCaseSensitiveSort();
		Opt.RightSelectedFirst=RightPanel->GetSelectedFirstMode();
		Opt.RightPanel.DirectoriesFirst=RightPanel->GetDirectoriesFirst();
	}

	RightPanel->GetCurDir(Opt.strRightFolder);
	RightPanel->GetCurBaseName(Opt.strRightCurFile);
	CtrlObject->HiFiles->SaveHiData();

	ConfigWriter cfg_writer;

	/* *************************************************** </ПРЕПРОЦЕССЫ> */
	cfg_writer.SelectSection(NSecSystem);
	cfg_writer.SetString("PersonalPluginsPath", Opt.LoadPlug.strPersonalPluginsPath);
//	cfg_writer.SetString(NSecLanguage, "Main", Opt.strLanguage);

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		if (CFG[I].IsSave)
		{
			cfg_writer.SelectSection(CFG[I].KeyName);
			switch (CFG[I].ValType)
			{
				case REG_DWORD:
					cfg_writer.SetUInt(CFG[I].ValName, *(unsigned int *)CFG[I].ValPtr);
					break;
				case REG_SZ:
					cfg_writer.SetString(CFG[I].ValName, CFG[I].StrPtr->CPtr());
					break;
				case REG_BINARY:
					cfg_writer.SetBytes(CFG[I].ValName, (const BYTE*)CFG[I].ValPtr, CFG[I].DefDWord);
					break;
			}
		}
	}

	/* <ПОСТПРОЦЕССЫ> *************************************************** */
	FileFilter::SaveFilters(cfg_writer);
	FileList::SavePanelModes(cfg_writer);

	if (Ask)
		CtrlObject->Macro.SaveMacros(false);

	/* *************************************************** </ПОСТПРОЦЕССЫ> */
}

void LanguageSettings()
{
	VMenu *LangMenu, *HelpMenu;

	if (Select(FALSE, &LangMenu))
	{
		Lang.Close();

		if (!Lang.Init(g_strFarPath, true, Msg::NewFileName.ID()))
		{
			Message(MSG_WARNING, 1, L"Error", L"Cannot load language data", L"Ok");
			exit(0);
		}

		Select(TRUE,&HelpMenu);
		delete HelpMenu;
		LangMenu->Hide();
		CtrlObject->Plugins.ReloadLanguage();
		setenv("FARLANG", Opt.strLanguage.GetMB().c_str(), 1);
		PrepareStrFTime();
		PrepareUnitStr();
		FrameManager->InitKeyBar();
		CtrlObject->Cp()->RedrawKeyBar();
		CtrlObject->Cp()->SetScreenPosition();
		ApplySudoConfiguration();
	}
	delete LangMenu; //???? BUGBUG
}

int GetConfigValue(const wchar_t *wKey, const wchar_t *wName, DWORD &dwValue, FARString &strValue, const void **binData)
{
	std::string sKey = FARString(wKey).GetMB();
	std::string sName = FARString(wName).GetMB();
	const char *Key=sKey.c_str(), *Name=sName.c_str();

	for (size_t I=0; I < ARRAYSIZE(CFG); ++I)
	{
		if (!strcasecmp(CFG[I].KeyName,Key) && !strcasecmp(CFG[I].ValName,Name))
		{
			switch (CFG[I].ValType)
			{
				case REG_DWORD:
					dwValue = *(unsigned int *)CFG[I].ValPtr;
					return REG_DWORD;
				case REG_SZ:
					strValue = *CFG[I].StrPtr;
					return REG_SZ;
				case REG_BINARY:
					*binData = CFG[I].ValPtr;
					dwValue = CFG[I].DefDWord;
					return REG_BINARY;
			}
		}
	}
	return REG_NONE;
}
