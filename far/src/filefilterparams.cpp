/*
filefilterparams.cpp

Параметры Файлового фильтра
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

#include "colors.hpp"
#include "CFileMask.hpp"
#include "lang.hpp"
#include "keys.hpp"
#include "ctrlobj.hpp"
#include "dialog.hpp"
#include "filelist.hpp"
#include "filefilterparams.hpp"
#include "udlist.hpp"
#include "farcolors.hpp"
#include "message.hpp"
#include "interf.hpp"
#include "setcolor.hpp"
#include "pick_color.hpp"
#include "datetime.hpp"
#include "strmix.hpp"
#include "config.hpp"
#include "DlgGuid.hpp"

FileFilterParams::FileFilterParams()
{
	SetMask(1, L"*");
	SetSize(0, L"", L"");
	memset(&FDate, 0, sizeof(FDate));
	memset(&FAttr, 0, sizeof(FAttr));
	memset(&FHighlight.Colors, 0, sizeof(FHighlight.Colors));
	FHighlight.SortGroup = DEFAULT_SORT_GROUP;
	FHighlight.bContinueProcessing = false;
	ClearAllFlags();
}

const FileFilterParams &FileFilterParams::operator=(const FileFilterParams &FF)
{
	if (this != &FF) {
		SetTitle(FF.GetTitle());
		const wchar_t *Mask;
		FF.GetMask(&Mask);
		SetMask(FF.GetMask(nullptr), Mask);
		FSize = FF.FSize;
		FDate = FF.FDate;
		FAttr = FF.FAttr;
		FF.GetColors(&FHighlight.Colors);
		FHighlight.SortGroup = FF.GetSortGroup();
		FHighlight.bContinueProcessing = FF.GetContinueProcessing();
		memcpy(FFlags, FF.FFlags, sizeof(FFlags));
	}

	return *this;
}

void FileFilterParams::SetTitle(const wchar_t *Title)
{
	m_strTitle = Title;
}

void FileFilterParams::SetMask(bool Used, const wchar_t *Mask, bool IgnoreCase)
{
	FMask.Used = Used;
	FMask.strMask = Mask;
	FMask.IgnoreCase = IgnoreCase;
	FARString strMask = FMask.strMask;

	// Проверка на валидность текущих настроек фильтра
	if (!FMask.FilterMask.Set(strMask, FMF_SILENT)) {
		FMask.strMask = L"*";
		FMask.FilterMask.Set(FMask.strMask, FMF_SILENT);
	}
}

void FileFilterParams::SetDate(bool Used, DWORD DateType, FILETIME DateAfter, FILETIME DateBefore,
		bool bRelative)
{
	FDate.Used = Used;
	FDate.DateType = (enumFDateType)DateType;

	if (DateType >= FDATE_COUNT)
		FDate.DateType = FDATE_MODIFIED;

	FDate.DateAfter.u.LowPart = DateAfter.dwLowDateTime;
	FDate.DateAfter.u.HighPart = DateAfter.dwHighDateTime;
	FDate.DateBefore.u.LowPart = DateBefore.dwLowDateTime;
	FDate.DateBefore.u.HighPart = DateBefore.dwHighDateTime;
	FDate.bRelative = bRelative;
}

void FileFilterParams::SetSize(bool Used, const wchar_t *SizeAbove, const wchar_t *SizeBelow)
{
	FSize.Used = Used;
	far_wcsncpy(FSize.SizeAbove, SizeAbove, ARRAYSIZE(FSize.SizeAbove));
	far_wcsncpy(FSize.SizeBelow, SizeBelow, ARRAYSIZE(FSize.SizeBelow));
	FSize.SizeAboveReal = ConvertFileSizeString(FSize.SizeAbove);
	FSize.SizeBelowReal = ConvertFileSizeString(FSize.SizeBelow);
}

void FileFilterParams::SetAttr(bool Used, DWORD AttrSet, DWORD AttrClear)
{
	FAttr.Used = Used;
	FAttr.AttrSet = AttrSet;
	FAttr.AttrClear = AttrClear;
}

void FileFilterParams::SetColors(HighlightDataColor *Colors)
{
	FHighlight.Colors = *Colors;
}

const wchar_t *FileFilterParams::GetTitle() const
{
	return m_strTitle;
}

const size_t FileFilterParams::GetTitleLen() const
{
	return m_strTitle.GetLength();
}

bool FileFilterParams::GetMask(const wchar_t **Mask) const
{
	if (Mask)
		*Mask = FMask.strMask;

	return FMask.Used;
}

bool FileFilterParams::GetMaskIgnoreCase() const
{
	return FMask.IgnoreCase;
}

bool FileFilterParams::GetDate(DWORD *DateType, FILETIME *DateAfter, FILETIME *DateBefore,
		bool *bRelative) const
{
	if (DateType)
		*DateType = FDate.DateType;

	if (DateAfter) {
		DateAfter->dwLowDateTime = FDate.DateAfter.u.LowPart;
		DateAfter->dwHighDateTime = FDate.DateAfter.u.HighPart;
	}

	if (DateBefore) {
		DateBefore->dwLowDateTime = FDate.DateBefore.u.LowPart;
		DateBefore->dwHighDateTime = FDate.DateBefore.u.HighPart;
	}

	if (bRelative)
		*bRelative = FDate.bRelative;

	return FDate.Used;
}

bool FileFilterParams::GetSize(const wchar_t **SizeAbove, const wchar_t **SizeBelow) const
{
	if (SizeAbove)
		*SizeAbove = FSize.SizeAbove;

	if (SizeBelow)
		*SizeBelow = FSize.SizeBelow;

	return FSize.Used;
}

bool FileFilterParams::GetAttr(DWORD *AttrSet, DWORD *AttrClear) const
{
	if (AttrSet)
		*AttrSet = FAttr.AttrSet;

	if (AttrClear)
		*AttrClear = FAttr.AttrClear;

	return FAttr.Used;
}

void FileFilterParams::GetColors(HighlightDataColor *Colors) const
{
	*Colors = FHighlight.Colors;
}

bool FileFilterParams::FileInFilter(const FileListItem &fli, uint64_t CurrentTime) const
{
	return FileInFilterImpl(fli.strName, fli.FileAttr, fli.FileSize, fli.CreationTime, fli.AccessTime,
			fli.WriteTime, fli.ChangeTime, CurrentTime);
}

bool FileFilterParams::FileInFilter(const FAR_FIND_DATA_EX &fde, uint64_t CurrentTime) const
{
	return FileInFilterImpl(fde.strFileName, fde.dwFileAttributes, fde.nFileSize, fde.ftCreationTime,
			fde.ftLastAccessTime, fde.ftLastWriteTime, fde.ftChangeTime, CurrentTime);
}

bool FileFilterParams::FileInFilter(const FAR_FIND_DATA &fd, uint64_t CurrentTime) const
{
	return FileInFilterImpl(FARString(fd.lpwszFileName), fd.dwFileAttributes, fd.nFileSize, fd.ftCreationTime,
			fd.ftLastAccessTime, fd.ftLastWriteTime, fd.ftLastWriteTime, CurrentTime);
}

bool FileFilterParams::FileInFilterImpl(const FARString &strFileName, DWORD dwFileAttributes,
		uint64_t nFileSize, const FILETIME &CreationTime, const FILETIME &AccessTime,
		const FILETIME &WriteTime, const FILETIME &ChangeTime, uint64_t CurrentTime) const
{
	// Режим проверки атрибутов файла включен?
	if (FAttr.Used) {
		// Проверка попадания файла по установленным атрибутам
		if ((dwFileAttributes & FAttr.AttrSet) != FAttr.AttrSet)
			return false;

		// Проверка попадания файла по отсутствующим атрибутам
		if (dwFileAttributes & FAttr.AttrClear)
			return false;
	}

	// Режим проверки размера файла включен?
	if (FSize.Used) {
		if (*FSize.SizeAbove && nFileSize < FSize.SizeAboveReal) {	// Размер файла меньше минимального разрешённого по фильтру?
			return false;											// Не пропускаем этот файл
		}

		if (*FSize.SizeBelow && nFileSize > FSize.SizeBelowReal) {	// Размер файла больше максимального разрешённого по фильтру?
			return false;											// Не пропускаем этот файл
		}
	}

	// Режим проверки времени файла включен?
	if (FDate.Used) {
		// Преобразуем FILETIME в беззнаковый int64_t
		uint64_t after = FDate.DateAfter.QuadPart;
		uint64_t before = FDate.DateBefore.QuadPart;

		if (after || before) {
			const FILETIME *ft;

			switch (FDate.DateType) {
				case FDATE_CREATED:
					ft = &CreationTime;
					break;
				case FDATE_OPENED:
					ft = &AccessTime;
					break;
				case FDATE_CHANGED:
					ft = &ChangeTime;
					break;
				default:	// case FDATE_MODIFIED:
					ft = &WriteTime;
			}

			ULARGE_INTEGER ftime;
			ftime.u.LowPart = ft->dwLowDateTime;
			ftime.u.HighPart = ft->dwHighDateTime;

			if (FDate.bRelative) {
				if (after)
					after = CurrentTime - after;

				if (before)
					before = CurrentTime - before;
			}

			// Есть введённая пользователем начальная дата?
			if (after && ftime.QuadPart < after) {	// Дата файла меньше начальной даты по фильтру?
				return false;						// Не пропускаем этот файл
			}

			// Есть введённая пользователем конечная дата?
			if (before && ftime.QuadPart > before) {	// Дата файла больше конечной даты по фильтру?
				return false;
			}
		}
	}

	// Режим проверки маски файла включен?
	if (FMask.Used && !FMask.FilterMask.Compare(strFileName, !FMask.IgnoreCase)) {	// Файл не попадает под маску введённую в фильтре?
		return false;																// Не пропускаем этот файл
	}

	// Да! Файл выдержал все испытания и будет допущен к использованию
	// в вызвавшей эту функцию операции.
	return true;
}

void FillPreviewStr(wchar_t *dstStr, size_t dstsize, const wchar_t *srcStr, const size_t srcsize, bool b_trim_dots = false)
{
	size_t	ng = 0, mcl = 0;

	if (!dstStr || !dstsize)
		return;

	if (srcStr && srcsize) {
		ng = dstsize;
		if (b_trim_dots) { // need truncate by dots
			if (StrCellsCount(srcStr, srcsize) > dstsize)
				ng -= 3;
			else
				b_trim_dots = false;
		}
		mcl = StrSizeOfCells(srcStr, srcsize, ng, false);
		memcpy(dstStr, srcStr, mcl * sizeof(wchar_t));
		dstStr += mcl;
		if (b_trim_dots) {
			*dstStr=L'.'; dstStr++;
			*dstStr=L'.'; dstStr++;
			*dstStr=L'.'; dstStr++;
			ng += 3;
		}
	}

	while(ng < dstsize) {
		*dstStr++ = 32;
		ng++;
	}

	*dstStr = 0;
}

#if 0
	wchar_t Attr[ARRAYSIZE(AttrC) * 2] = {0};

	for (size_t i = 0; i < ARRAYSIZE(AttrF); i++) {
		wchar_t *Ptr = Attr + i * 2;
		*Ptr = AttrC[i];

		if ((IncludeAttr & AttrF[i]) == AttrF[i])
			*(Ptr + 1) = L'+';
		else if ((ExcludeAttr & AttrF[i]) == AttrF[i])
			*(Ptr + 1) = L'-';
		else
			*Ptr = *(Ptr + 1) = L'.';
	}
#endif

void FillAttrStr(wchar_t *AttrStr, uint32_t IncludeAttr, uint32_t ExcludeAttr, uint32_t style)
{
	static const wchar_t AttrC[] = L"RAHSD<CEI$TLOVXBYKFN";
	static const DWORD AttrF[ARRAYSIZE(AttrC) - 1] = {FILE_ATTRIBUTE_READONLY, FILE_ATTRIBUTE_ARCHIVE,
			FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_SYSTEM, FILE_ATTRIBUTE_DIRECTORY, FILE_ATTRIBUTE_HARDLINKS, FILE_ATTRIBUTE_COMPRESSED,
			FILE_ATTRIBUTE_ENCRYPTED, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, FILE_ATTRIBUTE_SPARSE_FILE,
			FILE_ATTRIBUTE_TEMPORARY, FILE_ATTRIBUTE_REPARSE_POINT, FILE_ATTRIBUTE_OFFLINE,
			FILE_ATTRIBUTE_VIRTUAL, FILE_ATTRIBUTE_EXECUTABLE, FILE_ATTRIBUTE_BROKEN,
			FILE_ATTRIBUTE_DEVICE_CHAR, FILE_ATTRIBUTE_DEVICE_BLOCK, FILE_ATTRIBUTE_DEVICE_FIFO, FILE_ATTRIBUTE_DEVICE_SOCK};
	wchar_t *Ptr = AttrStr;

	if (!AttrStr)
		return;

	if (style == 1) {

		if (IncludeAttr) {
			*Ptr++ = L'+';
			for (size_t i = 0; i < ARRAYSIZE(AttrF); i++) {
				if ((IncludeAttr & AttrF[i]))
					*Ptr++ = AttrC[i];
			}
			*Ptr++ = L' ';
		}
		if (ExcludeAttr) {
			*Ptr++ = L'-';
			for (size_t i = 0; i < ARRAYSIZE(AttrF); i++) {
				if ((ExcludeAttr & AttrF[i]))
					*Ptr++ = AttrC[i];
			}
		}

		while(Ptr < (AttrStr + 23)) {
			*Ptr++ = 32;
		}
		*Ptr = 0;
	}
	else {
		for (size_t i = 0; i < ARRAYSIZE(AttrF); i++) {
			wchar_t *Ptr = AttrStr + i * 2;
			*Ptr = AttrC[i];

			if ((IncludeAttr & AttrF[i]) == AttrF[i])
				*(Ptr + 1) = L'+';
			else if ((ExcludeAttr & AttrF[i]) == AttrF[i])
				*(Ptr + 1) = L'-';
			else
				*Ptr = *(Ptr + 1) = L'.';
		}
		Ptr += ARRAYSIZE(AttrF) * 2;
		*Ptr = 0;
	}
}

// Централизованная функция для создания строк меню различных фильтров.
void MenuString(FARString &strDest, FileFilterParams *FF, uint32_t attrstyle, bool bHighlightType, int Hotkey, bool bPanelType,
		const wchar_t *FMask, const wchar_t *Title)
{
	wchar_t MarkStrPrw[8];
	wchar_t NameStrPrw[32];

//	const wchar_t Format1A[] = L"%ls %lc %-16.16ls %-2.2ls %lc %ls";
	const wchar_t Format1A[] = L"%ls %lc %ls %-2.2ls %lc %ls";

	const wchar_t Format1c[] = L"&%lc. %-18.18ls %lc %ls %-2.2ls %lc %ls";
	const wchar_t Format1d[] = L"   %-18.18ls %lc %ls %-2.2ls %lc %ls";

//	const wchar_t Format1c[] = L"&%lc. %-18.18ls %lc %-16.16ls %-2.2ls %lc %ls";
//	const wchar_t Format1d[] = L"   %-18.18ls %lc %-16.16ls %-2.2ls %lc %ls";

	const wchar_t Format2[] = L"%ls %lc %ls %lc %ls %-3.3ls %lc %ls";
	const wchar_t DownArrow = 0x2193;
	const wchar_t *Name, *Mask;

	size_t NameLen;
	DWORD IncludeAttr, ExcludeAttr;
	bool UseMask, UseSize, UseDate, RelativeDate;
	HighlightDataColor hl;

	if (bPanelType) {
		Name = Title;
		NameLen = wcslen(Title);
		UseMask = true;
		Mask = FMask;
		IncludeAttr = 0;
		ExcludeAttr = FILE_ATTRIBUTE_DIRECTORY;
		RelativeDate = UseDate = UseSize = false;
	} else {
		FF->GetColors(&hl);
		Name = FF->GetTitle();
		NameLen = FF->GetTitleLen();
		FillPreviewStr(MarkStrPrw, 5, hl.Mark, hl.MarkLen);

		UseMask = FF->GetMask(&Mask);

		if (!FF->GetAttr(&IncludeAttr, &ExcludeAttr))
			IncludeAttr = ExcludeAttr = 0;

		UseSize = FF->GetSize(nullptr, nullptr);
		UseDate = FF->GetDate(nullptr, nullptr, nullptr, &RelativeDate);
	}

	wchar_t Attr[64];
	switch(attrstyle) {
		case 0: FillAttrStr(Attr, IncludeAttr, ExcludeAttr, attrstyle); break;
		case 1: FillAttrStr(Attr, IncludeAttr, ExcludeAttr, attrstyle); break;
	}

	wchar_t SizeDate[4] = L"...";

	if (UseSize) {
		SizeDate[0] = L'S';
	}

	if (UseDate) {
		if (RelativeDate)
			SizeDate[1] = L'R';
		else
			SizeDate[1] = L'D';
	}

	if (bHighlightType) {
		if (FF->GetContinueProcessing())
			SizeDate[2] = DownArrow;

		FillPreviewStr(NameStrPrw, 18, Name, NameLen, true);
		strDest.Format(Format2, MarkStrPrw, BoxSymbols[BS_V1], NameStrPrw, BoxSymbols[BS_V1], Attr, SizeDate, BoxSymbols[BS_V1],
				UseMask ? Mask : L"");
	} else {
		SizeDate[2] = 0;

		if (!Hotkey && !bPanelType) {
			FillPreviewStr(NameStrPrw, wcschr(Name, L'&') ? 22 : 21, Name, NameLen);
			strDest.Format(Format1A, NameStrPrw, BoxSymbols[BS_V1], Attr, SizeDate,
					BoxSymbols[BS_V1], UseMask ? Mask : L"");
		} else if (Hotkey) {
			strDest.Format(Format1c,
				Hotkey, Name, BoxSymbols[BS_V1], Attr, SizeDate, BoxSymbols[BS_V1], UseMask ? Mask : L"");
		} else {
			strDest.Format(Format1d,
				Name, BoxSymbols[BS_V1], Attr, SizeDate, BoxSymbols[BS_V1], UseMask ? Mask : L"");
		}
	}

	RemoveTrailingSpaces(strDest);
}

struct filterpar_highlight_state_s
{
	HighlightDataColor hl;
	wchar_t wsIndent[16];
	CHAR_INFO vbuff[64];
};

enum enumFileFilterConfig
{
	ID_FF_TITLE,

	ID_FF_NAME,
	ID_FF_NAMEEDIT,

	ID_FF_SEPARATOR1,

	ID_FF_MATCHMASK,
	ID_FF_MASKEDIT,
	ID_FF_MATCHCASE,

	ID_FF_SEPARATOR2,

	ID_FF_MATCHSIZE,
	ID_FF_SIZEFROMSIGN,
	ID_FF_SIZEFROMEDIT,
	ID_FF_SIZETOSIGN,
	ID_FF_SIZETOEDIT,

	ID_FF_MATCHDATE,
	ID_FF_DATETYPE,
	ID_FF_DATERELATIVE,
	ID_FF_DATEBEFORESIGN,
	ID_FF_DATEBEFOREEDIT,
	ID_FF_DAYSBEFOREEDIT,
	ID_FF_TIMEBEFOREEDIT,
	ID_FF_DATEAFTERSIGN,
	ID_FF_DATEAFTEREDIT,
	ID_FF_DAYSAFTEREDIT,
	ID_FF_TIMEAFTEREDIT,
	ID_FF_CURRENT,
	ID_FF_BLANK,

	ID_FF_SEPARATOR3,
	ID_FF_SEPARATOR4,

	ID_FF_MATCHATTRIBUTES,


	ID_FF_READONLY,
	ID_FF_FIRST_ATTR = ID_FF_READONLY,

	ID_FF_ARCHIVE,
	ID_FF_HIDDEN,
	ID_FF_SYSTEM,
	ID_FF_DIRECTORY,
	ID_FF_HARDLINKS,
	ID_FF_COMPRESSED,
	ID_FF_ENCRYPTED,
	ID_FF_NOTINDEXED,
	ID_FF_REPARSEPOINT,
	ID_FF_SPARSE,
	ID_FF_TEMP,
	ID_FF_OFFLINE,
	ID_FF_VIRTUAL,
	ID_FF_EXECUTABLE,
	ID_FF_BROKEN,

	ID_FF_DEVCHAR,
	ID_FF_DEVBLOCK,
	ID_FF_DEVFIFO,
	ID_FF_DEVSOCK,
	ID_FF_LAST_ATTR = ID_FF_DEVSOCK,

	ID_HER_SEPARATOR1,
	ID_HER_MARK_TITLE,
	ID_HER_MARKEDIT,
	ID_HER_MARKINHERIT,
	ID_HER_MARKADDINHERIT,
	ID_HER_CUSTOMINDENT,
	ID_HER_INDENTEDIT,

	ID_HER_NORMALFILE,
	ID_HER_NORMALMARKING,
	ID_HER_SELECTEDFILE,
	ID_HER_SELECTEDMARKING,
	ID_HER_CURSORFILE,
	ID_HER_CURSORMARKING,
	ID_HER_SELECTEDCURSORFILE,
	ID_HER_SELECTEDCURSORMARKING,

	ID_HER_COLOREXAMPLE,
	ID_HER_CONTINUEPROCESSING,

	ID_FF_SEPARATOR5,

	ID_FF_OK,
	ID_FF_RESET,
	ID_FF_CANCEL,
	ID_FF_MAKETRANSPARENT,
};

void FilterDlgRelativeDateItemsUpdate(HANDLE hDlg, bool bClear)
{
	SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);

	if (SendDlgMessage(hDlg, DM_GETCHECK, ID_FF_DATERELATIVE, 0)) {
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DATEBEFOREEDIT, 0);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DATEAFTEREDIT, 0);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_CURRENT, 0);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DAYSBEFOREEDIT, 1);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DAYSAFTEREDIT, 1);
	} else {
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DAYSBEFOREEDIT, 0);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DAYSAFTEREDIT, 0);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DATEBEFOREEDIT, 1);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_DATEAFTEREDIT, 1);
		SendDlgMessage(hDlg, DM_SHOWITEM, ID_FF_CURRENT, 1);
	}

	if (bClear) {
		SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_DATEAFTEREDIT, (LONG_PTR)L"");
		SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_DAYSAFTEREDIT, (LONG_PTR)L"");
		SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_TIMEAFTEREDIT, (LONG_PTR)L"");
		SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_DATEBEFOREEDIT, (LONG_PTR)L"");
		SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_TIMEBEFOREEDIT, (LONG_PTR)L"");
		SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_DAYSBEFOREEDIT, (LONG_PTR)L"");
	}

	SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
}

LONG_PTR WINAPI FileFilterConfigDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	filterpar_highlight_state_s *fphlstate = (filterpar_highlight_state_s *)SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
	bool bColorConfig = bool(fphlstate);

	switch (Msg) {
		case DN_INITDIALOG: {
			FilterDlgRelativeDateItemsUpdate(hDlg, false);
			SendDlgMessage(hDlg, DM_SETTEXTLENGTH, ID_HER_MARKEDIT, HIGHLIGHT_MAX_MARK_LENGTH + 8);
			return TRUE;
		}

		case DN_DRAWDLGITEM: {
			if (Param1 != ID_HER_COLOREXAMPLE)
				break;

			static const DWORD FarColor[] = {COL_PANELTEXT, COL_PANELSELECTEDTEXT, COL_PANELCURSOR, COL_PANELSELECTEDCURSOR};
			wchar_t VerticalLine0[] = {BoxSymbols[BS_V2], 0};
			wchar_t VerticalLine1[] = {BoxSymbols[BS_V1], 0};
			SMALL_RECT drect;
			SMALL_RECT irect;

			SendDlgMessage(hDlg, DM_GETDLGRECT, 0, (intptr_t)&drect);
			SendDlgMessage(hDlg, DM_GETITEMPOSITION, ID_HER_COLOREXAMPLE, (intptr_t)&irect);

			irect.Top += drect.Top;
			irect.Bottom += drect.Top;
			irect.Right += drect.Left;
			irect.Left += drect.Left;

			HighlightDataColor *hl = &fphlstate->hl;
			size_t	filenameexamplelen = wcslen(Msg::HighlightExample1);
			size_t	freespace = irect.Right - irect.Left - filenameexamplelen - 2;
			size_t	ng = freespace;
			size_t	mcl = StrSizeOfCells(hl->Mark, hl->MarkLen, ng, false);

			size_t	prews = std::min(Opt.MinFilenameIndentation, Opt.MaxFilenameIndentation);
			if (ng < prews)
				prews -= ng;
			else
				prews = 0;

			uint64_t ColorB = FarColorToReal(COL_PANELBOX);

			for (int i = 0; i < 4; i++) {
				int x = irect.Left;
				int y = irect.Top + i;

				uint64_t ColorF = FarColorToReal(FarColor[i]);
				uint64_t ColorM = FarColorToReal(FarColor[i]);
				uint64_t ColorHF = hl->Color[HIGHLIGHTCOLORTYPE_FILE][i];
				uint64_t ColorHM = hl->Color[HIGHLIGHTCOLORTYPE_MARKSTR][i];
				uint64_t MaskHM = hl->Mask[HIGHLIGHTCOLORTYPE_MARKSTR][i];

				if (!(ColorHF & 0xFF)) {
					ColorHF = FarColorToReal(FarColor[i]);
				}
				if (!(ColorHM & 0xFF)) {
					ColorHM = ColorHF;
					MaskHM = hl->Mask[HIGHLIGHTCOLORTYPE_FILE][i];
				}

				if (hl->Mask[HIGHLIGHTCOLORTYPE_FILE][i])
					ColorF = (ColorF & (~hl->Mask[HIGHLIGHTCOLORTYPE_FILE][i])) | (ColorHF & hl->Mask[HIGHLIGHTCOLORTYPE_FILE][i]);
				if (MaskHM)
					ColorM = (ColorM & (~MaskHM)) | (ColorHM & MaskHM);

				Text(x, y, ColorB, VerticalLine0, 1);
				x++;
				Text(x, y, ColorM, hl->Mark, mcl);
				x += ng;

				if (prews) {
					Text(L' ', prews);
					x += prews;
				}

				Text(x, y, ColorF, Msg::HighlightExample1, filenameexamplelen);
				x += filenameexamplelen;
				ColorF &= (0xFFFFFFFFFFFFFFFF ^ (COMMON_LVB_STRIKEOUT | COMMON_LVB_UNDERSCORE));
				Text(L' ', ColorF, freespace-(ng + prews));

				x += (freespace - (ng + prews));
				Text(x, y, ColorB, VerticalLine1, 1);
			}

			return 0;
		}
		break;

		case DN_BTNCLICK: {
			if (Param1 == ID_FF_CURRENT || Param1 == ID_FF_BLANK)		// Current и Blank
			{
				FILETIME ft;
				FARString strDate, strTime;

				if (Param1 == ID_FF_CURRENT) {
					WINPORT(GetSystemTimeAsFileTime)(&ft);
					ConvertDate(ft, strDate, strTime, 12, FALSE, FALSE, 2);
				} else {
					strDate.Clear();
					strTime.Clear();
				}

				SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
				int relative = (int)SendDlgMessage(hDlg, DM_GETCHECK, ID_FF_DATERELATIVE, 0);
				int db = relative ? ID_FF_DAYSBEFOREEDIT : ID_FF_DATEBEFOREEDIT;
				int da = relative ? ID_FF_DAYSAFTEREDIT : ID_FF_DATEAFTEREDIT;
				SendDlgMessage(hDlg, DM_SETTEXTPTR, da, (LONG_PTR)strDate.CPtr());
				SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_TIMEAFTEREDIT, (LONG_PTR)strTime.CPtr());
				SendDlgMessage(hDlg, DM_SETTEXTPTR, db, (LONG_PTR)strDate.CPtr());
				SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_TIMEBEFOREEDIT, (LONG_PTR)strTime.CPtr());
				SendDlgMessage(hDlg, DM_SETFOCUS, da, 0);
				COORD r;
				r.X = r.Y = 0;
				SendDlgMessage(hDlg, DM_SETCURSORPOS, da, (LONG_PTR)&r);
				SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
				break;
			} else if (Param1 == ID_FF_RESET)		// Reset
			{
				SendDlgMessage(hDlg, DM_ENABLEREDRAW, FALSE, 0);
				SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_MASKEDIT, (LONG_PTR)L"*");
				SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_SIZEFROMEDIT, (LONG_PTR)L"");
				SendDlgMessage(hDlg, DM_SETTEXTPTR, ID_FF_SIZETOEDIT, (LONG_PTR)L"");

				for (int I = ID_FF_FIRST_ATTR; I <= ID_FF_LAST_ATTR; ++I) {
					SendDlgMessage(hDlg, DM_SETCHECK, I, BSTATE_3STATE);
				}

				if (!bColorConfig)
					SendDlgMessage(hDlg, DM_SETCHECK, ID_FF_DIRECTORY, BSTATE_UNCHECKED);

				FarListPos LPos = {0, 0};
				SendDlgMessage(hDlg, DM_LISTSETCURPOS, ID_FF_DATETYPE, (LONG_PTR)&LPos);
				SendDlgMessage(hDlg, DM_SETCHECK, ID_FF_MATCHMASK, BSTATE_CHECKED);
				SendDlgMessage(hDlg, DM_SETCHECK, ID_FF_MATCHCASE, BSTATE_UNCHECKED);
				SendDlgMessage(hDlg, DM_SETCHECK, ID_FF_MATCHSIZE, BSTATE_UNCHECKED);
				SendDlgMessage(hDlg, DM_SETCHECK, ID_FF_MATCHDATE, BSTATE_UNCHECKED);
				SendDlgMessage(hDlg, DM_SETCHECK, ID_FF_DATERELATIVE, BSTATE_UNCHECKED);
				FilterDlgRelativeDateItemsUpdate(hDlg, true);
				SendDlgMessage(hDlg, DM_SETCHECK, ID_FF_MATCHATTRIBUTES,
						bColorConfig ? BSTATE_UNCHECKED : BSTATE_CHECKED);
				SendDlgMessage(hDlg, DM_ENABLEREDRAW, TRUE, 0);
				break;
			} else if (Param1 == ID_FF_MAKETRANSPARENT) {
				HighlightDataColor *hl = &fphlstate->hl;

				for (int i = 0; i < 2; i++)
					for (int j = 0; j < 4; j++)
						hl->Mask[i][j] = (0x000000000000FF00 ^ (BACKGROUND_TRUECOLOR | FOREGROUND_TRUECOLOR |
												COMMON_LVB_STRIKEOUT | COMMON_LVB_UNDERSCORE | COMMON_LVB_REVERSE_VIDEO));

				SendDlgMessage(hDlg, DM_SETCHECK, ID_HER_MARKINHERIT, BSTATE_CHECKED);
				SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
				break;
			} else if (Param1 == ID_FF_DATERELATIVE) {
				FilterDlgRelativeDateItemsUpdate(hDlg, true);
				break;
			}
		}
		case DN_MOUSECLICK:

			if ((Msg == DN_BTNCLICK && Param1 >= ID_HER_NORMALFILE && Param1 <= ID_HER_SELECTEDCURSORMARKING)
					|| (Msg == DN_MOUSECLICK && Param1 == ID_HER_COLOREXAMPLE &&
						((MOUSE_EVENT_RECORD *)Param2)->dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)) {

				if (Msg == DN_MOUSECLICK) {
					Param1 = ID_HER_NORMALFILE + ((MOUSE_EVENT_RECORD *)Param2)->dwMousePosition.Y * 2;

					if (((MOUSE_EVENT_RECORD *)Param2)->dwMousePosition.X == 1
							&& (fphlstate->hl.MarkLen))
						Param1 = ID_HER_NORMALMARKING + ((MOUSE_EVENT_RECORD *)Param2)->dwMousePosition.Y * 2;
				}

				// Color[0=file, 1=mark][0=normal,1=selected,2=undercursor,3=selectedundercursor]
				uint64_t *color = &fphlstate->hl.Color[(Param1 - ID_HER_NORMALFILE) & 1][(Param1 - ID_HER_NORMALFILE) / 2];
				uint64_t *mask  = &fphlstate->hl.Mask[(Param1 - ID_HER_NORMALFILE) & 1][(Param1 - ID_HER_NORMALFILE) / 2];
				GetColorDialogForFileFilter(color, mask);

				int nLength = (int)SendDlgMessage(hDlg, DM_GETTEXTLENGTH, ID_HER_MARKEDIT, 0);
				if (nLength > HIGHLIGHT_MAX_MARK_LENGTH ) {
					SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_HER_MARKEDIT, (LONG_PTR)&fphlstate->hl.Mark[0]);
				}
				else {
					SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_HER_MARKEDIT, (LONG_PTR)&fphlstate->hl.Mark[0]);
					fphlstate->hl.MarkLen = nLength;
				}

				SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
				return TRUE;
			}

			break;
		case DN_EDITCHANGE:

			if (Param1 == ID_HER_MARKEDIT) {

				int nLength = (int)SendDlgMessage(hDlg, DM_GETTEXTLENGTH, ID_HER_MARKEDIT, 0);
				if (nLength > HIGHLIGHT_MAX_MARK_LENGTH ) {
					SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_HER_MARKEDIT, (LONG_PTR)&fphlstate->hl.Mark[0]);
				}
				else {
					SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_HER_MARKEDIT, (LONG_PTR)&fphlstate->hl.Mark[0]);
					fphlstate->hl.MarkLen = nLength;
				}

				SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
				return TRUE;
			}
			if (Param1 == ID_HER_INDENTEDIT) {

				int nLength = (int)SendDlgMessage(hDlg, DM_GETTEXTLENGTH, ID_HER_INDENTEDIT, 0);
				if (nLength > 2 || nLength < 0) {
					SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_HER_INDENTEDIT, (LONG_PTR)&fphlstate->wsIndent[0]);
					SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
					return TRUE;
				}
				wchar_t wsTemp[16];
				SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_HER_INDENTEDIT, (LONG_PTR)&wsTemp[0]);
				for (size_t i = 0; i < (size_t)nLength; i++) {
					if (wsTemp[i] < L'0' || wsTemp[i] > L'9') {
						SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_HER_INDENTEDIT, (LONG_PTR)&fphlstate->wsIndent[0]);
						SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
						return TRUE;
					}
				}
				size_t v = wcstoul(wsTemp, nullptr, 10);
				if (v > HIGHLIGHT_MAX_MARK_LENGTH) {
					SendDlgMessage(hDlg, DM_SETTEXTPTRSILENT, ID_HER_INDENTEDIT, (LONG_PTR)&fphlstate->wsIndent[0]);
					SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
					return TRUE;
				}
				fphlstate->hl.Indent = v;
				SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_HER_INDENTEDIT, (LONG_PTR)&fphlstate->wsIndent[0]);

				SendDlgMessage(hDlg, DM_REDRAW, 0, 0);
				return TRUE;
			}

			break;
		case DN_CLOSE:

			if (Param1 == ID_FF_OK && SendDlgMessage(hDlg, DM_GETCHECK, ID_FF_MATCHSIZE, 0)) {
				FARString strTemp;
				wchar_t *temp;
				temp = strTemp.GetBuffer(SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_FF_SIZEFROMEDIT, 0) + 1);
				SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_FF_SIZEFROMEDIT, (LONG_PTR)temp);
				bool bTemp = !*temp || CheckFileSizeStringFormat(temp);
				temp = strTemp.GetBuffer(SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_FF_SIZETOEDIT, 0) + 1);
				SendDlgMessage(hDlg, DM_GETTEXTPTR, ID_FF_SIZETOEDIT, (LONG_PTR)temp);
				bTemp = bTemp && (!*temp || CheckFileSizeStringFormat(temp));

				if (!bTemp) {
					Message(MSG_WARNING, 1, bColorConfig ? Msg::FileHilightTitle : Msg::FileFilterTitle,
							Msg::BadFileSizeFormat, Msg::Ok);
					return FALSE;
				}
			}

			break;
	}

	return DefDlgProc(hDlg, Msg, Param1, Param2);
}

bool FileFilterConfig(FileFilterParams *FF, bool ColorConfig)
{
	static const wchar_t VerticalLine[] = {BoxSymbols[BS_T_H1V1], BoxSymbols[BS_V1], BoxSymbols[BS_V1],
			BoxSymbols[BS_V1], BoxSymbols[BS_B_H1V1], 0};
	// Временная маска.
	CFileMask FileMask;
	// История для маски файлов
	const wchar_t FilterMasksHistoryName[] = L"FilterMasks";
	// История для имени фильтра
	const wchar_t FilterNameHistoryName[] = L"FilterName";
	// Маски для диалога настройки
	// Маска для ввода дней для относительной даты
	const wchar_t DaysMask[] = L"9999";
	FARString strDateMask, strTimeMask;
	// Определение параметров даты и времени в системе.
	int DateSeparator = GetDateSeparator();
	int TimeSeparator = GetTimeSeparator();
	wchar_t DecimalSeparator = GetDecimalSeparator();
	int DateFormat = GetDateFormat();

	switch (DateFormat) {
		case 0:
			// Маска даты для форматов DD.MM.YYYYY и MM.DD.YYYYY
			strDateMask.Format(L"99%c99%c9999N", DateSeparator, DateSeparator);
			break;
		case 1:
			// Маска даты для форматов DD.MM.YYYYY и MM.DD.YYYYY
			strDateMask.Format(L"99%c99%c9999N", DateSeparator, DateSeparator);
			break;
		default:
			// Маска даты для формата YYYYY.MM.DD
			strDateMask.Format(L"N9999%c99%c99", DateSeparator, DateSeparator);
			break;
	}

	// Маска времени
	strTimeMask.Format(L"99%c99%c99%c99N", TimeSeparator, TimeSeparator, DecimalSeparator);
	DialogDataEx FilterDlgData[] = {
		{DI_DOUBLEBOX,   3,           1,  84,     20, {},                                  DIF_SHOWAMPERSAND,                      Msg::FileFilterTitle             },

		{DI_TEXT,        5,           2,  0,      2,  {},                                  DIF_FOCUS,                              Msg::FileFilterName              },
		{DI_EDIT,        5,           2,  82,     2,  {(DWORD_PTR)FilterNameHistoryName},  DIF_HISTORY,                            L""                              },

		{DI_TEXT,        0,           3,  0,      3,  {},                                  DIF_SEPARATOR,                          L""                              },

		{DI_CHECKBOX,    5,           4,  0,      4,  {},                                  DIF_AUTOMATION,                         Msg::FileFilterMatchMask         },
		{DI_EDIT,        5,           4,  82,     4,  {(DWORD_PTR)FilterMasksHistoryName}, DIF_HISTORY,                            L""                              },

		{DI_CHECKBOX,    5,           5,  0,      5,  {},                                  0,				                       Msg::FileFilterMatchMaskCase		},

		{DI_TEXT,        0,           6,  0,      6,  {},                                  DIF_SEPARATOR,                          L""                              },

		{DI_CHECKBOX,    5,           7,  0,      7,  {},                                  DIF_AUTOMATION,                         Msg::FileFilterSize              },
		{DI_TEXT,        7,           8,  8,      8,  {},                                  0,                                      Msg::FileFilterSizeFromSign      },
		{DI_EDIT,        10,          8,  20,     8,  {},                                  0,                                      L""                              },
		{DI_TEXT,        7,           9,  8,      9,  {},                                  0,                                      Msg::FileFilterSizeToSign        },
		{DI_EDIT,        10,          9,  20,     9,  {},                                  0,                                      L""                              },

		{DI_CHECKBOX,    24,          7,  0,      7,  {},                                  DIF_AUTOMATION,                         Msg::FileFilterDate              },
		{DI_COMBOBOX,    26,          8,  41,     8,  {},                                  DIF_DROPDOWNLIST | DIF_LISTNOAMPERSAND, L""                              },
		{DI_CHECKBOX,    26,          9,  0,      9,  {},                                  0,                                      Msg::FileFilterDateRelative      },
		{DI_TEXT,        48,          8,  50,     8,  {},                                  0,                                      Msg::FileFilterDateBeforeSign    },
		{DI_FIXEDIT,     51,          8,  61,     8,  {(DWORD_PTR)strDateMask.CPtr()},     DIF_MASKEDIT,                           L""                              },
		{DI_FIXEDIT,     51,          8,  61,     8,  {(DWORD_PTR)DaysMask},               DIF_MASKEDIT,                           L""                              },
		{DI_FIXEDIT,     63,          8,  74,     8,  {(DWORD_PTR)strTimeMask.CPtr()},     DIF_MASKEDIT,                           L""                              },
		{DI_TEXT,        48,          9,  50,     9,  {},                                  0,                                      Msg::FileFilterDateAfterSign     },
		{DI_FIXEDIT,     51,          9,  61,     9,  {(DWORD_PTR)strDateMask.CPtr()},     DIF_MASKEDIT,                           L""                              },
		{DI_FIXEDIT,     51,          9,  61,     9,  {(DWORD_PTR)DaysMask},               DIF_MASKEDIT,                           L""                              },
		{DI_FIXEDIT,     63,          9,  74,     9,  {(DWORD_PTR)strTimeMask.CPtr()},     DIF_MASKEDIT,                           L""                              },
		{DI_BUTTON,      0,           7,  0,      7,  {},                                  DIF_BTNNOCLOSE,                         Msg::FileFilterCurrent           },
		{DI_BUTTON,      0,           7,  74,     7,  {},                                  DIF_BTNNOCLOSE,                         Msg::FileFilterBlank             },

		{DI_TEXT,        0,           10,  0,      10,  {},                                  DIF_SEPARATOR,                          L""                              },
		{DI_VTEXT,       22,          6,  22,     10,  {},                                  DIF_BOXCOLOR,                           VerticalLine                     },

		{DI_CHECKBOX,    5,           11, 0,      11, {},                                  DIF_AUTOMATION,                         Msg::FileFilterAttr              },
		{DI_CHECKBOX,    7,           12, 0,      12, {},                                  DIF_3STATE,                             Msg::FileFilterAttrR             },
		{DI_CHECKBOX,    7,           13, 0,      13, {},                                  DIF_3STATE,                             Msg::FileFilterAttrA             },
		{DI_CHECKBOX,    7,           14, 0,      14, {},                                  DIF_3STATE,                             Msg::FileFilterAttrH             },
		{DI_CHECKBOX,    7,           15, 0,      15, {},                                  DIF_3STATE,                             Msg::FileFilterAttrS             },
		{DI_CHECKBOX,    7,           16, 0,      16, {},                                  DIF_3STATE,                             Msg::FileFilterAttrD             },

		{DI_CHECKBOX,    26,          11, 0,      11, {},                                  DIF_3STATE,                             Msg::FileFilterAttrHardLinks     },
		{DI_CHECKBOX,    26,          12, 0,      12, {},                                  DIF_3STATE,                             Msg::FileFilterAttrC             },
		{DI_CHECKBOX,    26,          13, 0,      13, {},                                  DIF_3STATE,                             Msg::FileFilterAttrE             },
		{DI_CHECKBOX,    26,          14, 0,      14, {},                                  DIF_3STATE,                             Msg::FileFilterAttrNI            },
		{DI_CHECKBOX,    26,          15, 0,      15, {},                                  DIF_3STATE,                             Msg::FileFilterAttrReparse       },
		{DI_CHECKBOX,    26,          16, 0,      16, {},                                  DIF_3STATE,                             Msg::FileFilterAttrSparse        },

		{DI_CHECKBOX,    45,          12, 0,      12, {},                                  DIF_3STATE,                             Msg::FileFilterAttrT             },
		{DI_CHECKBOX,    45,          13, 0,      13, {},                                  DIF_3STATE,                             Msg::FileFilterAttrOffline       },
		{DI_CHECKBOX,    45,          14, 0,      14, {},                                  DIF_3STATE,                             Msg::FileFilterAttrVirtual       },
		{DI_CHECKBOX,    45,          15, 0,      15, {},                                  DIF_3STATE,                             Msg::FileFilterAttrExecutable    },
		{DI_CHECKBOX,    45,          16, 0,      16, {},                                  DIF_3STATE,                             Msg::FileFilterAttrBroken        },

		{DI_CHECKBOX,    64,          12, 0,      12, {},                                  DIF_3STATE,                             Msg::FileFilterAttrDevChar       },
		{DI_CHECKBOX,    64,          13, 0,      13, {},                                  DIF_3STATE,                             Msg::FileFilterAttrDevBlock      },
		{DI_CHECKBOX,    64,          14, 0,      14, {},                                  DIF_3STATE,                             Msg::FileFilterAttrDevFIFO       },
		{DI_CHECKBOX,    64,          15, 0,      15, {},                                  DIF_3STATE,                             Msg::FileFilterAttrDevSock       },

		{DI_TEXT,        -1,          17, 0,      17, {},                                  DIF_SEPARATOR,                          Msg::HighlightColors             },
		{DI_TEXT,        16,          18, 0,      18, {},                                  0,                                      Msg::HighlightMarking            },
		{DI_EDIT,        5,           18, 14,     18, {},                                  0,                                      L""                              },
		{DI_CHECKBOX,    0,           18, 0,      18, {},                                  0,                                      Msg::HighlightMarkStrInherit     },
		{DI_CHECKBOX,    0,           18, 0,      18, {},                                  0,                                      Msg::HighlightMarkAddInherit     },
		{DI_CHECKBOX,    0,           18, 0,      18, {},                                  DIF_HIDDEN,                             Msg::HighlightCustomIdent        },
		{DI_EDIT,        0,           18, 4,      18, {},                                  DIF_HIDDEN,                             L""                              },

		{DI_BUTTON,      5,           19, 0,      19, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightFileName1          },
		{DI_BUTTON,      0,           19, 0,      19, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightMarking1           },
		{DI_BUTTON,      5,           20, 0,      20, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightFileName2          },
		{DI_BUTTON,      0,           20, 0,      20, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightMarking2           },
		{DI_BUTTON,      5,           21, 0,      21, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightFileName3          },
		{DI_BUTTON,      0,           21, 0,      21, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightMarking3           },
		{DI_BUTTON,      5,           22, 0,      22, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightFileName4          },
		{DI_BUTTON,      0,           22, 0,      22, {},                                  DIF_BTNNOCLOSE | DIF_NOBRACKETS,        Msg::HighlightMarking4           },
		{DI_USERCONTROL, 54,          19, 79,     22, {},                                  DIF_NOFOCUS,                            L""                              },

		{DI_CHECKBOX,    5,           23, 0,      23, {},                                  0,                                      Msg::HighlightContinueProcessing },
		{DI_TEXT,        0,           18, 0,      18, {},                                  DIF_SEPARATOR,                          L""                              },
		{DI_BUTTON,      0,           19, 0,      19, {},                                  DIF_DEFAULT | DIF_CENTERGROUP,          Msg::Ok                          },
		{DI_BUTTON,      0,           19, 0,      19, {},                                  DIF_CENTERGROUP | DIF_BTNNOCLOSE,       Msg::FileFilterReset             },
		{DI_BUTTON,      0,           19, 0,      19, {},                                  DIF_CENTERGROUP,                        Msg::FileFilterCancel            },
		{DI_BUTTON,      0,           19, 0,      19, {},                                  DIF_CENTERGROUP | DIF_BTNNOCLOSE,       Msg::FileFilterMakeTransparent   }
	};
	FilterDlgData[0].Data = (ColorConfig ? Msg::FileHilightTitle : Msg::FileFilterTitle);
	MakeDialogItemsEx(FilterDlgData, FilterDlg);

	if (ColorConfig) {

		FilterDlg[ID_FF_TITLE].Y2 += 6;

		for (int i = ID_FF_SEPARATOR5; i <= ID_FF_MAKETRANSPARENT; i++) {
			FilterDlg[i].Y1 += 6;
			FilterDlg[i].Y2 += 6;
		}

	} else {
		for (int i = ID_HER_SEPARATOR1; i <= ID_HER_CONTINUEPROCESSING; i++)
			FilterDlg[i].Flags|= DIF_HIDDEN;

		FilterDlg[ID_FF_MAKETRANSPARENT].Flags = DIF_HIDDEN;
	}

	FilterDlg[ID_FF_NAMEEDIT].X1 = FilterDlg[ID_FF_NAME].X1 + (int)FilterDlg[ID_FF_NAME].strData.GetLength()
			- (FilterDlg[ID_FF_NAME].strData.Contains(L'&') ? 1 : 0) + 1;
	FilterDlg[ID_FF_MASKEDIT].X1 = FilterDlg[ID_FF_MATCHMASK].X1
			+ (int)FilterDlg[ID_FF_MATCHMASK].strData.GetLength()
			- (FilterDlg[ID_FF_MATCHMASK].strData.Contains(L'&') ? 1 : 0) + 5;
	FilterDlg[ID_FF_BLANK].X1 = FilterDlg[ID_FF_BLANK].X2 - (int)FilterDlg[ID_FF_BLANK].strData.GetLength()
			+ (FilterDlg[ID_FF_BLANK].strData.Contains(L'&') ? 1 : 0) - 3;
	FilterDlg[ID_FF_CURRENT].X2 = FilterDlg[ID_FF_BLANK].X1 - 2;
	FilterDlg[ID_FF_CURRENT].X1 = FilterDlg[ID_FF_CURRENT].X2
			- (int)FilterDlg[ID_FF_CURRENT].strData.GetLength()
			+ (FilterDlg[ID_FF_CURRENT].strData.Contains(L'&') ? 1 : 0) - 3;
	FilterDlg[ID_HER_MARKINHERIT].X1 = FilterDlg[ID_HER_MARK_TITLE].X1
			+ (int)FilterDlg[ID_HER_MARK_TITLE].strData.GetLength()
			- (FilterDlg[ID_HER_MARK_TITLE].strData.Contains(L'&') ? 1 : 0) + 1;
	FilterDlg[ID_HER_MARKADDINHERIT].X1 = FilterDlg[ID_HER_MARKINHERIT].X1
			+ (int)FilterDlg[ID_HER_MARKINHERIT].strData.GetLength()
			- (FilterDlg[ID_HER_MARKINHERIT].strData.Contains(L'&') ? 1 : 0) + 5;
	FilterDlg[ID_HER_CUSTOMINDENT].X1 = FilterDlg[ID_HER_MARKADDINHERIT].X1
			+ (int)FilterDlg[ID_HER_MARKADDINHERIT].strData.GetLength()
			- (FilterDlg[ID_HER_MARKADDINHERIT].strData.Contains(L'&') ? 1 : 0) + 5;
	FilterDlg[ID_HER_INDENTEDIT].X1 = FilterDlg[ID_HER_CUSTOMINDENT].X1
			+ (int)FilterDlg[ID_HER_CUSTOMINDENT].strData.GetLength()
			- (FilterDlg[ID_HER_CUSTOMINDENT].strData.Contains(L'&') ? 1 : 0) + 5;
	FilterDlg[ID_HER_INDENTEDIT].X2 = FilterDlg[ID_HER_INDENTEDIT].X1 + 4;


	for (int i = ID_HER_NORMALMARKING; i <= ID_HER_SELECTEDCURSORMARKING; i+= 2)
		FilterDlg[i].X1 = FilterDlg[ID_HER_NORMALFILE].X1
				+ (int)FilterDlg[ID_HER_NORMALFILE].strData.GetLength()
				- (FilterDlg[ID_HER_NORMALFILE].strData.Contains(L'&') ? 1 : 0) + 1;

	filterpar_highlight_state_s fphlstate;

	FF->GetColors(&fphlstate.hl);
	FilterDlg[ID_HER_COLOREXAMPLE].VBuf = fphlstate.vbuff;

	swprintf(fphlstate.wsIndent, 10, L"%u", fphlstate.hl.Indent);
	FilterDlg[ID_HER_INDENTEDIT].strData = fphlstate.wsIndent;

	FilterDlg[ID_HER_MARKEDIT].strData = fphlstate.hl.Mark;
	FilterDlg[ID_HER_MARKINHERIT].Selected = ((fphlstate.hl.Flags & HL_FLAGS_MARK_INHERIT) ? 1 : 0);
	FilterDlg[ID_HER_MARKADDINHERIT].Selected = ((fphlstate.hl.Flags & HL_FLAGS_MARK_ADD) ? 1 : 0);
	FilterDlg[ID_HER_CUSTOMINDENT].Selected = ((fphlstate.hl.Flags & HL_FLAGS_INDENT) ? 1 : 0);

	FilterDlg[ID_HER_CONTINUEPROCESSING].Selected = (FF->GetContinueProcessing() ? 1 : 0);
	FilterDlg[ID_FF_NAMEEDIT].strData = FF->GetTitle();
	FilterDlg[ID_FF_MATCHCASE].Selected = (FF->GetMaskIgnoreCase() ? 0 : 1);

	const wchar_t *FMask;
	FilterDlg[ID_FF_MATCHMASK].Selected = FF->GetMask(&FMask) ? 1 : 0;
	FilterDlg[ID_FF_MASKEDIT].strData = FMask;

	if (!FilterDlg[ID_FF_MATCHMASK].Selected)
	{
		FilterDlg[ID_FF_MATCHCASE].Flags|= DIF_DISABLE;
		FilterDlg[ID_FF_MASKEDIT].Flags|= DIF_DISABLE;
	}

	const wchar_t *SizeAbove, *SizeBelow;
	FilterDlg[ID_FF_MATCHSIZE].Selected = FF->GetSize(&SizeAbove, &SizeBelow) ? 1 : 0;
	FilterDlg[ID_FF_SIZEFROMEDIT].strData = SizeAbove;
	FilterDlg[ID_FF_SIZETOEDIT].strData = SizeBelow;

	if (!FilterDlg[ID_FF_MATCHSIZE].Selected)
		for (int i = ID_FF_SIZEFROMSIGN; i <= ID_FF_SIZETOEDIT; i++)
			FilterDlg[i].Flags|= DIF_DISABLE;

	// Лист для комбобокса времени файла
	FarList DateList;
	FarListItem TableItemDate[FDATE_COUNT] = {};
	// Настройка списка типов дат файла
	DateList.Items = TableItemDate;
	DateList.ItemsNumber = FDATE_COUNT;

	for (int i = 0; i < FDATE_COUNT; ++i)
		TableItemDate[i].Text = (Msg::FileFilterWrited + i);

	DWORD DateType;
	FILETIME DateAfter, DateBefore;
	bool bRelative;
	FilterDlg[ID_FF_MATCHDATE].Selected = FF->GetDate(&DateType, &DateAfter, &DateBefore, &bRelative) ? 1 : 0;
	FilterDlg[ID_FF_DATERELATIVE].Selected = bRelative ? 1 : 0;
	FilterDlg[ID_FF_DATETYPE].ListItems = &DateList;
	TableItemDate[DateType].Flags = LIF_SELECTED;

	if (bRelative) {
		ConvertRelativeDate(DateAfter, FilterDlg[ID_FF_DAYSAFTEREDIT].strData,
				FilterDlg[ID_FF_TIMEAFTEREDIT].strData);
		ConvertRelativeDate(DateBefore, FilterDlg[ID_FF_DAYSBEFOREEDIT].strData,
				FilterDlg[ID_FF_TIMEBEFOREEDIT].strData);
	} else {
		ConvertDate(DateAfter, FilterDlg[ID_FF_DATEAFTEREDIT].strData, FilterDlg[ID_FF_TIMEAFTEREDIT].strData,
				12, FALSE, FALSE, 2);
		ConvertDate(DateBefore, FilterDlg[ID_FF_DATEBEFOREEDIT].strData,
				FilterDlg[ID_FF_TIMEBEFOREEDIT].strData, 12, FALSE, FALSE, 2);
	}

	if (!FilterDlg[ID_FF_MATCHDATE].Selected)
		for (int i = ID_FF_DATETYPE; i <= ID_FF_BLANK; i++)
			FilterDlg[i].Flags|= DIF_DISABLE;

	DWORD AttrSet, AttrClear;
	FilterDlg[ID_FF_MATCHATTRIBUTES].Selected = FF->GetAttr(&AttrSet, &AttrClear) ? 1 : 0;
	FilterDlg[ID_FF_READONLY].Selected = (AttrSet & FILE_ATTRIBUTE_READONLY ? 1
					: AttrClear & FILE_ATTRIBUTE_READONLY
					? 0
					: 2);
	FilterDlg[ID_FF_ARCHIVE].Selected = (AttrSet & FILE_ATTRIBUTE_ARCHIVE ? 1
					: AttrClear & FILE_ATTRIBUTE_ARCHIVE
					? 0
					: 2);
	FilterDlg[ID_FF_HIDDEN].Selected = (AttrSet & FILE_ATTRIBUTE_HIDDEN ? 1
					: AttrClear & FILE_ATTRIBUTE_HIDDEN
					? 0
					: 2);
	FilterDlg[ID_FF_SYSTEM].Selected = (AttrSet & FILE_ATTRIBUTE_SYSTEM ? 1
					: AttrClear & FILE_ATTRIBUTE_SYSTEM
					? 0
					: 2);
	FilterDlg[ID_FF_COMPRESSED].Selected = (AttrSet & FILE_ATTRIBUTE_COMPRESSED ? 1
					: AttrClear & FILE_ATTRIBUTE_COMPRESSED
					? 0
					: 2);
	FilterDlg[ID_FF_ENCRYPTED].Selected = (AttrSet & FILE_ATTRIBUTE_ENCRYPTED ? 1
					: AttrClear & FILE_ATTRIBUTE_ENCRYPTED
					? 0
					: 2);
	FilterDlg[ID_FF_DIRECTORY].Selected = (AttrSet & FILE_ATTRIBUTE_DIRECTORY ? 1
					: AttrClear & FILE_ATTRIBUTE_DIRECTORY
					? 0
					: 2);
	FilterDlg[ID_FF_HARDLINKS].Selected = (AttrSet & FILE_ATTRIBUTE_HARDLINKS ? 1
					: AttrClear & FILE_ATTRIBUTE_HARDLINKS
					? 0
					: 2);
	FilterDlg[ID_FF_NOTINDEXED].Selected = (AttrSet & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ? 1
					: AttrClear & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
					? 0
					: 2);
	FilterDlg[ID_FF_SPARSE].Selected = (AttrSet & FILE_ATTRIBUTE_SPARSE_FILE ? 1
					: AttrClear & FILE_ATTRIBUTE_SPARSE_FILE
					? 0
					: 2);
	FilterDlg[ID_FF_TEMP].Selected = (AttrSet & FILE_ATTRIBUTE_TEMPORARY ? 1
					: AttrClear & FILE_ATTRIBUTE_TEMPORARY
					? 0
					: 2);
	FilterDlg[ID_FF_REPARSEPOINT].Selected = (AttrSet & FILE_ATTRIBUTE_REPARSE_POINT ? 1
					: AttrClear & FILE_ATTRIBUTE_REPARSE_POINT
					? 0
					: 2);
	FilterDlg[ID_FF_OFFLINE].Selected = (AttrSet & FILE_ATTRIBUTE_OFFLINE ? 1
					: AttrClear & FILE_ATTRIBUTE_OFFLINE
					? 0
					: 2);
	FilterDlg[ID_FF_VIRTUAL].Selected = (AttrSet & FILE_ATTRIBUTE_VIRTUAL ? 1
					: AttrClear & FILE_ATTRIBUTE_VIRTUAL
					? 0
					: 2);
	FilterDlg[ID_FF_EXECUTABLE].Selected = (AttrSet & FILE_ATTRIBUTE_EXECUTABLE ? 1
					: AttrClear & FILE_ATTRIBUTE_EXECUTABLE
					? 0
					: 2);
	FilterDlg[ID_FF_BROKEN].Selected = (AttrSet & FILE_ATTRIBUTE_BROKEN ? 1
					: AttrClear & FILE_ATTRIBUTE_BROKEN
					? 0
					: 2);
	FilterDlg[ID_FF_DEVCHAR].Selected = (AttrSet & FILE_ATTRIBUTE_DEVICE_CHAR ? 1
					: AttrClear & FILE_ATTRIBUTE_DEVICE_CHAR
					? 0
					: 2);
	FilterDlg[ID_FF_DEVBLOCK].Selected = (AttrSet & FILE_ATTRIBUTE_DEVICE_BLOCK ? 1
					: AttrClear & FILE_ATTRIBUTE_DEVICE_BLOCK
					? 0
					: 2);
	FilterDlg[ID_FF_DEVFIFO].Selected = (AttrSet & FILE_ATTRIBUTE_DEVICE_FIFO ? 1
					: AttrClear & FILE_ATTRIBUTE_DEVICE_FIFO
					? 0
					: 2);
	FilterDlg[ID_FF_DEVSOCK].Selected = (AttrSet & FILE_ATTRIBUTE_DEVICE_SOCK ? 1
					: AttrClear & FILE_ATTRIBUTE_DEVICE_SOCK
					? 0
					: 2);

	if (!FilterDlg[ID_FF_MATCHATTRIBUTES].Selected) {
		for (int i = ID_FF_FIRST_ATTR; i <= ID_FF_LAST_ATTR; i++)
			FilterDlg[i].Flags|= DIF_DISABLE;
	}

	Dialog Dlg(FilterDlg, ARRAYSIZE(FilterDlg), FileFilterConfigDlgProc,
			(LONG_PTR)(ColorConfig ? &fphlstate : nullptr));
	Dlg.SetHelp(ColorConfig ? L"HighlightEdit" : L"Filter");
	Dlg.SetId(ColorConfig ? HighlightConfigId : FiltersConfigId);
	Dlg.SetPosition(-1, -1, FilterDlg[ID_FF_TITLE].X2 + 4, FilterDlg[ID_FF_TITLE].Y2 + 2);
	Dlg.SetAutomation(ID_FF_MATCHMASK, ID_FF_MASKEDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHMASK, ID_FF_MATCHCASE, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE, ID_FF_SIZEFROMSIGN, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE, ID_FF_SIZEFROMEDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE, ID_FF_SIZETOSIGN, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHSIZE, ID_FF_SIZETOEDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DATETYPE, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DATERELATIVE, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DATEAFTERSIGN, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DATEAFTEREDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DAYSAFTEREDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_TIMEAFTEREDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DATEBEFORESIGN, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DATEBEFOREEDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_DAYSBEFOREEDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_TIMEBEFOREEDIT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_CURRENT, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	Dlg.SetAutomation(ID_FF_MATCHDATE, ID_FF_BLANK, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	for (unsigned id = ID_FF_FIRST_ATTR; id <= ID_FF_LAST_ATTR; ++id) {
		Dlg.SetAutomation(ID_FF_MATCHATTRIBUTES, id, DIF_DISABLE, DIF_NONE, DIF_NONE, DIF_DISABLE);
	}

	for (;;) {
		Dlg.ClearDone();
		Dlg.Process();
		int ExitCode = Dlg.GetExitCode();

		if (ExitCode == ID_FF_OK)		// Ok
		{
			// Если введённая пользователем маска не корректна, тогда вернёмся в диалог
			if (FilterDlg[ID_FF_MATCHMASK].Selected && !FileMask.Set(FilterDlg[ID_FF_MASKEDIT].strData, 0))
				continue;

			fphlstate.hl.Flags = (HL_FLAGS_MARK_INHERIT * FilterDlg[ID_HER_MARKINHERIT].Selected);
			fphlstate.hl.Flags |= (HL_FLAGS_MARK_ADD * FilterDlg[ID_HER_MARKADDINHERIT].Selected);
			fphlstate.hl.Flags |= (HL_FLAGS_INDENT * FilterDlg[ID_HER_CUSTOMINDENT].Selected);
			fphlstate.hl.Indent = wcstoul(FilterDlg[ID_HER_INDENTEDIT].strData, nullptr, 10);

			FF->SetColors(&fphlstate.hl);
			FF->SetContinueProcessing(FilterDlg[ID_HER_CONTINUEPROCESSING].Selected != 0);
			FF->SetTitle(FilterDlg[ID_FF_NAMEEDIT].strData);
			FF->SetMask(FilterDlg[ID_FF_MATCHMASK].Selected != 0, FilterDlg[ID_FF_MASKEDIT].strData,
						FilterDlg[ID_FF_MATCHCASE].Selected != 1);
			FF->SetSize(FilterDlg[ID_FF_MATCHSIZE].Selected != 0, FilterDlg[ID_FF_SIZEFROMEDIT].strData,
					FilterDlg[ID_FF_SIZETOEDIT].strData);
			bRelative = FilterDlg[ID_FF_DATERELATIVE].Selected != 0;

			FilterDlg[ID_FF_TIMEBEFOREEDIT].strData.ReplaceChar(8, TimeSeparator);
			FilterDlg[ID_FF_TIMEAFTEREDIT].strData.ReplaceChar(8, TimeSeparator);

			StrToDateTime(FilterDlg[bRelative ? ID_FF_DAYSAFTEREDIT : ID_FF_DATEAFTEREDIT].strData,
					FilterDlg[ID_FF_TIMEAFTEREDIT].strData, DateAfter, DateFormat, DateSeparator,
					TimeSeparator, bRelative);
			StrToDateTime(FilterDlg[bRelative ? ID_FF_DAYSBEFOREEDIT : ID_FF_DATEBEFOREEDIT].strData,
					FilterDlg[ID_FF_TIMEBEFOREEDIT].strData, DateBefore, DateFormat, DateSeparator,
					TimeSeparator, bRelative);
			FF->SetDate(FilterDlg[ID_FF_MATCHDATE].Selected != 0, FilterDlg[ID_FF_DATETYPE].ListPos,
					DateAfter, DateBefore, bRelative);
			AttrSet = 0;
			AttrClear = 0;
			AttrSet|= (FilterDlg[ID_FF_READONLY].Selected == 1 ? FILE_ATTRIBUTE_READONLY : 0);
			AttrSet|= (FilterDlg[ID_FF_ARCHIVE].Selected == 1 ? FILE_ATTRIBUTE_ARCHIVE : 0);
			AttrSet|= (FilterDlg[ID_FF_HIDDEN].Selected == 1 ? FILE_ATTRIBUTE_HIDDEN : 0);
			AttrSet|= (FilterDlg[ID_FF_SYSTEM].Selected == 1 ? FILE_ATTRIBUTE_SYSTEM : 0);
			AttrSet|= (FilterDlg[ID_FF_COMPRESSED].Selected == 1 ? FILE_ATTRIBUTE_COMPRESSED : 0);
			AttrSet|= (FilterDlg[ID_FF_ENCRYPTED].Selected == 1 ? FILE_ATTRIBUTE_ENCRYPTED : 0);
			AttrSet|= (FilterDlg[ID_FF_DIRECTORY].Selected == 1 ? FILE_ATTRIBUTE_DIRECTORY : 0);
			AttrSet|= (FilterDlg[ID_FF_HARDLINKS].Selected == 1 ? FILE_ATTRIBUTE_HARDLINKS : 0);
			AttrSet|= (FilterDlg[ID_FF_NOTINDEXED].Selected == 1 ? FILE_ATTRIBUTE_NOT_CONTENT_INDEXED : 0);
			AttrSet|= (FilterDlg[ID_FF_SPARSE].Selected == 1 ? FILE_ATTRIBUTE_SPARSE_FILE : 0);
			AttrSet|= (FilterDlg[ID_FF_TEMP].Selected == 1 ? FILE_ATTRIBUTE_TEMPORARY : 0);
			AttrSet|= (FilterDlg[ID_FF_REPARSEPOINT].Selected == 1 ? FILE_ATTRIBUTE_REPARSE_POINT : 0);
			AttrSet|= (FilterDlg[ID_FF_OFFLINE].Selected == 1 ? FILE_ATTRIBUTE_OFFLINE : 0);
			AttrSet|= (FilterDlg[ID_FF_VIRTUAL].Selected == 1 ? FILE_ATTRIBUTE_VIRTUAL : 0);
			AttrSet|= (FilterDlg[ID_FF_EXECUTABLE].Selected == 1 ? FILE_ATTRIBUTE_EXECUTABLE : 0);
			AttrSet|= (FilterDlg[ID_FF_BROKEN].Selected == 1 ? FILE_ATTRIBUTE_BROKEN : 0);
			AttrSet|= (FilterDlg[ID_FF_DEVCHAR].Selected == 1 ? FILE_ATTRIBUTE_DEVICE_CHAR : 0);
			AttrSet|= (FilterDlg[ID_FF_DEVBLOCK].Selected == 1 ? FILE_ATTRIBUTE_DEVICE_BLOCK : 0);
			AttrSet|= (FilterDlg[ID_FF_DEVFIFO].Selected == 1 ? FILE_ATTRIBUTE_DEVICE_FIFO : 0);
			AttrSet|= (FilterDlg[ID_FF_DEVSOCK].Selected == 1 ? FILE_ATTRIBUTE_DEVICE_SOCK : 0);

			AttrClear|= (FilterDlg[ID_FF_READONLY].Selected == 0 ? FILE_ATTRIBUTE_READONLY : 0);
			AttrClear|= (FilterDlg[ID_FF_ARCHIVE].Selected == 0 ? FILE_ATTRIBUTE_ARCHIVE : 0);
			AttrClear|= (FilterDlg[ID_FF_HIDDEN].Selected == 0 ? FILE_ATTRIBUTE_HIDDEN : 0);
			AttrClear|= (FilterDlg[ID_FF_SYSTEM].Selected == 0 ? FILE_ATTRIBUTE_SYSTEM : 0);
			AttrClear|= (FilterDlg[ID_FF_COMPRESSED].Selected == 0 ? FILE_ATTRIBUTE_COMPRESSED : 0);
			AttrClear|= (FilterDlg[ID_FF_ENCRYPTED].Selected == 0 ? FILE_ATTRIBUTE_ENCRYPTED : 0);
			AttrClear|= (FilterDlg[ID_FF_DIRECTORY].Selected == 0 ? FILE_ATTRIBUTE_DIRECTORY : 0);
			AttrClear|= (FilterDlg[ID_FF_HARDLINKS].Selected == 0 ? FILE_ATTRIBUTE_HARDLINKS : 0);
			AttrClear|= (FilterDlg[ID_FF_NOTINDEXED].Selected == 0 ? FILE_ATTRIBUTE_NOT_CONTENT_INDEXED : 0);
			AttrClear|= (FilterDlg[ID_FF_SPARSE].Selected == 0 ? FILE_ATTRIBUTE_SPARSE_FILE : 0);
			AttrClear|= (FilterDlg[ID_FF_TEMP].Selected == 0 ? FILE_ATTRIBUTE_TEMPORARY : 0);
			AttrClear|= (FilterDlg[ID_FF_REPARSEPOINT].Selected == 0 ? FILE_ATTRIBUTE_REPARSE_POINT : 0);
			AttrClear|= (FilterDlg[ID_FF_OFFLINE].Selected == 0 ? FILE_ATTRIBUTE_OFFLINE : 0);
			AttrClear|= (FilterDlg[ID_FF_VIRTUAL].Selected == 0 ? FILE_ATTRIBUTE_VIRTUAL : 0);
			AttrClear|= (FilterDlg[ID_FF_EXECUTABLE].Selected == 0 ? FILE_ATTRIBUTE_EXECUTABLE : 0);
			AttrClear|= (FilterDlg[ID_FF_BROKEN].Selected == 0 ? FILE_ATTRIBUTE_BROKEN : 0);
			AttrClear|= (FilterDlg[ID_FF_DEVCHAR].Selected == 0 ? FILE_ATTRIBUTE_DEVICE_CHAR : 0);
			AttrClear|= (FilterDlg[ID_FF_DEVBLOCK].Selected == 0 ? FILE_ATTRIBUTE_DEVICE_BLOCK : 0);
			AttrClear|= (FilterDlg[ID_FF_DEVFIFO].Selected == 0 ? FILE_ATTRIBUTE_DEVICE_FIFO : 0);
			AttrClear|= (FilterDlg[ID_FF_DEVSOCK].Selected == 0 ? FILE_ATTRIBUTE_DEVICE_SOCK : 0);

			FF->SetAttr(FilterDlg[ID_FF_MATCHATTRIBUTES].Selected != 0, AttrSet, AttrClear);
			return true;
		} else
			break;
	}

	return false;
}
