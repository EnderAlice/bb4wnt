/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "../BB.h"
#include "../Settings.h"
#include "../Workspaces.h"
#include "../Pidl.h"
#include "../Desk.h"
#include "MenuMaker.h"
#include "Menu.h"
#include "RecentItem.h"

//===========================================================================
//
// CommandItem
//
//===========================================================================

CommandItem::CommandItem(const TCHAR* pszCommand, const TCHAR* pszTitle, bool isChecked, const TCHAR* pszIcon)
    : MenuItem(pszTitle)
{
	m_pszCommand = new_str(pszCommand);
	m_isChecked = isChecked;
	m_ItemID = MENUITEM_ID_CI;
	m_pszIcon = new_str(pszIcon);
	m_phIcon = GetIcon(pszIcon);
}

//====================
CommandItem::~CommandItem(){
	free_str(&m_pszIcon);
}
//====================
void CommandItem::Invoke(int button)
{
	if (INVOKE_LEFT & button)
	{
		TCHAR szPath[MAX_PATH]; szPath[0] = _T('\0');
		m_pMenu->hide_on_click();
		if (m_pszCommand){
			if (const TCHAR *p = stristr(m_pszCommand, _T("@BBCore.exec ")))
				_tcsncpy_s(szPath, MAX_PATH, p+13, _TRUNCATE);
			post_command(m_pszCommand);
		}
		else{
			if (m_pidl){
				TCHAR buf[MAX_PATH];
				if (SHGetPathFromIDList(m_pidl, buf))
					_sntprintf_s(szPath, MAX_PATH, _TRUNCATE, _T("\"%s\""), buf);

				exec_pidl(m_pidl, NULL, NULL);
			}
		}

		TCHAR szMenuPath[MAX_PATH]; szMenuPath[0] = _T('\0');
		unquote(szMenuPath, MAX_PATH, Settings_recentMenu);
		int nKeep = Settings_recentItemKeepSize;
		int nSort = Settings_recentItemSortSize;
		bool bBeginEnd = Settings_recentBeginEnd;

		if (szPath[0] && szMenuPath[0] && (nKeep || nSort))
			CreateRecentItemMenu(szMenuPath, szPath, m_pszTitle, m_pszIcon, nKeep, nSort, bBeginEnd);

		return;
	}

	if (INVOKE_RIGHT & button)
	{
		ShowContextMenu(m_pszCommand, m_pidl);
		return;
	}

	if (INVOKE_DRAG & button)
	{
		m_pMenu->start_drag(m_pszCommand, m_pidl);
		return;
	}
}

//===========================================================================
// base class for IntegerItem / StringItem

CommandItemEx::CommandItemEx(const TCHAR *pszCommand, const TCHAR *fmt)
    : CommandItem(NULL, _T(""), false, NULL)
{
	TCHAR buffer[1000];
	if (NULL == _tcschr(pszCommand, _T('%')))
		_tcsncpy_s(buffer, 1000, pszCommand, _TRUNCATE); _tcsncat_s(buffer, 1000, fmt, _TRUNCATE); pszCommand = buffer;
	m_pszCommand = new_str(pszCommand);
}

void CommandItemEx::next_item (WPARAM wParam)
{
	Menu *pm = m_pMenu->m_pParent;
	if (pm)
	{
		pm->set_focus();
		if (wParam) PostMessage(pm->m_hwnd, WM_KEYDOWN, wParam, 0);
		Active(0);
	}
}

//===========================================================================
// VolumeItem

VolumeItem::VolumeItem(const TCHAR *pszTitle, const TCHAR *pszDllName): CommandItem(NULL, pszTitle, false, NULL), VolumeControl(pszDllName)
{
	m_isNOP = Settings_menuVolumeHilite ? 0 : MI_NOP_DISABLED;
	m_nVol = BBGetVolume();
	m_bMute = BBGetMute();
}

void VolumeItem::Measure(HDC hDC, SIZE *size)
{
	size->cx = Settings_menuVolumeWidth;
	size->cy = Settings_menuVolumeHeight;
}

void VolumeItem::Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int xmouse = (short)LOWORD(lParam);
	RECT r; GetItemRect(&r);
	_InflateRect(&r, -2, -2);
	switch(uMsg)
	{
	case WM_MOUSEMOVE:
		MenuItem::Mouse(hwnd, uMsg, wParam, lParam);
		if(!(wParam&MK_LBUTTON)) break;
	case WM_LBUTTONDOWN:
		BBSetVolume((int)iminmax(100*(xmouse-r.left)/GetRectWidth(&r), 0, 100));
		break;
	case WM_MBUTTONDOWN:
		PostMessage(BBhwnd, BB_EXECUTE, 0, (LPARAM) _T("control.exe mmsys.cpl"));
		break;
	case WM_RBUTTONDOWN:
		BBToggleMute();
		break;
	}
	Active(2);
	int nNewVol = BBGetVolume();
	bool bNewMute = BBGetMute();
	if ((nNewVol != m_nVol) || (bNewMute != m_bMute)){
		m_nVol = nNewVol;
		m_bMute = bNewMute;
		m_pMenu->redraw();
	}
	return;
}

void VolumeItem::Paint(HDC hDC)
{
	RECT r; GetItemRect(&r);
	StyleItem *pSI = &mStyle.MenuVolume;
	if (m_bActive && Settings_menuVolumeHilite){
		// draw hilite bar
		pSI = &mStyle.MenuHilite;
		MakeStyleGradient(hDC, &r, pSI, pSI->bordered);
		pSI = &mStyle.MenuFrame;
	}
	// draw volume bar
	RECT rcBar = {r.left+2, r.top+2, r.left+2 + (GetRectWidth(&r)-4)*m_nVol/100, r.bottom-2};
	if(GetRectWidth(&rcBar) > 0)
		MakeStyleGradient(hDC, &rcBar, pSI, pSI->bordered);

	// set volume text
	TCHAR title[128];
	if (m_bMute)
		_tcsncpy_s(title, 128, _T("mute"), _TRUNCATE);
	else if (0 <= m_nVol && m_nVol <= 100)
		_sntprintf_s(title, 128, _TRUNCATE, _T("%s %d%%"), m_pszTitle, m_nVol);
	else
		_tcsncpy_s(title, 128, _T("invalid"), _TRUNCATE);

	// draw menu item text
	BBDrawText(hDC, title, -1, &r, DT_CENTER | DT_MENU_STANDARD, pSI);
}

//===========================================================================
//
// IntegerItem
//
//===========================================================================

IntegerItem::IntegerItem(const TCHAR* pszCommand, intptr_t value, intptr_t minval, intptr_t maxval)
    : CommandItemEx(pszCommand, _T(" %d"))
{
	m_ItemID = MENUITEM_ID_CIInt;
	m_value = value;
	m_min = minval;
	m_max = maxval;
	oldsize = direction = 0;
	offvalue = 0x7FFFFFFF;
	offstring = NULL;
}

void IntegerItem::Measure(HDC hDC, SIZE *size)
{
	TCHAR buf[128], val[128];
	_sntprintf_s(val, 128, _TRUNCATE, offstring && offvalue == m_value ? NLS1(offstring) : _T("%Id"), m_value);
	_sntprintf_s(buf, 128, _TRUNCATE, _T("%c %s %c"),
			  m_value == m_min ? _T(' '): _T('-'),
			  val,
			  m_value == m_max ? _T(' '): _T('+')
			  );
	replace_str(&m_pszTitle, buf);

	MenuItem::Measure(hDC, size);
	if (size->cx > oldsize+5 || size->cx < oldsize-5)
		oldsize = size->cx;
	size->cx = oldsize + 5;
}

//====================

void IntegerItem::Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (false == Settings_menusBroamMode)
	{
		switch(uMsg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			{
				Active(2);
				int xmouse = (short)LOWORD(lParam);
				int xwidth = m_pMenu->m_width;
				direction = (xmouse < xwidth / 2) ? -1 : 1;
				m_pMenu->m_captureflg |= MENU_CAPT_TWEAKINT;
				SetCapture(hwnd);
				m_count = 0;
				ItemTimer(MENU_INTITEM_TIMER);
			}
			return;

		case WM_LBUTTONUP:
			if (direction)
			{
				direction = 0;
				Invoke(INVOKE_LEFT);
			}
			return;
		}
	}
	MenuItem::Mouse(hwnd, uMsg, wParam, lParam);
}

void IntegerItem::set_next_value(void)
{
	intptr_t value = m_value;
	int dir = direction;
	if (//m_count >= 12 ||
		GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		dir *= 10;
		intptr_t mod = value % dir;
		if (mod>0 && dir<0) mod+=dir;
		value -= mod;
	}
	m_value = iminmax(value + dir, m_min, m_max);
	m_pMenu->redraw();
}

void IntegerItem::ItemTimer(WPARAM nTimer)
{
	if (MENU_INTITEM_TIMER != nTimer)
	{
		MenuItem::ItemTimer(nTimer);
		return;
	}
	HWND hwnd = m_pMenu->m_hwnd;
	if (0 == direction || GetCapture() != hwnd)
	{
		KillTimer(hwnd, nTimer);
		return;
	}
	set_next_value();
	SetTimer(hwnd, MENU_INTITEM_TIMER, 0 == m_count ? 320 : 80, NULL);
	++m_count;
}

void IntegerItem::Key(UINT nMsg, WPARAM wParam)
{
	if (WM_KEYDOWN == nMsg)
	{
		if (VK_LEFT == wParam)
		{
			direction = -1;
			set_next_value();
		}
		else
			if (VK_RIGHT == wParam)
			{
				direction = 1;
				set_next_value();
			}
			else
				if (VK_UP == wParam || VK_DOWN==wParam)
				{
					next_item(wParam);
				}
	}
	else
		if (WM_KEYUP == nMsg)
		{
			if (VK_LEFT == wParam || VK_RIGHT == wParam)
			{
				if (direction)
				{
					Invoke(INVOKE_LEFT);
					direction = 0;
				}
			}
		}
		else
			if (WM_CHAR == nMsg && (VK_ESCAPE == wParam || VK_RETURN==wParam))
			{
				next_item(0);
			}

}

void IntegerItem::Invoke(int button)
{
	if (INVOKE_LEFT & button)
	{
		if (Settings_menusBroamMode) return;
		TCHAR result[1024];
		_sntprintf_s(result, 1024, _TRUNCATE, m_pszCommand, m_value);
		post_command(result);
	}
}

//===========================================================================
//
// StringItem
//
//===========================================================================

StringItem::StringItem(const TCHAR* pszCommand, const TCHAR *init_string)
    : CommandItemEx(pszCommand, _T(" %s"))
{
	if (init_string) replace_str(&m_pszTitle, init_string);
	m_ItemID = MENUITEM_ID_CIStr;
	hText = NULL;
}

StringItem::~StringItem()
{
	if (hText) DestroyWindow(hText);
}

void StringItem::Paint(HDC hDC)
{
	if (false == Settings_menusBroamMode)
		m_bActive = m_pMenu->m_bHasFocus && 0 == m_isNOP;
	MenuItem::Paint(hDC);
}

void StringItem::Measure(HDC hDC, SIZE *size)
{
	if (Settings_menusBroamMode)
	{
		MenuItem::Measure(hDC, size);
		return;
	}
	MenuItem::Measure(hDC, size);
	size->cx = (int)imax(size->cx + 20, 120);
	size->cy += 6;
}

void StringItem::Key(UINT nMsg, WPARAM wParam)
{
	if (WM_KEYDOWN == nMsg && (VK_UP == wParam || VK_DOWN == wParam))
	{
		next_item(wParam);
	}
	else
		if (WM_CHAR == nMsg && wParam==VK_ESCAPE)
		{
			SetWindowText(hText, m_pszTitle);
			next_item(0);
		}
}

void StringItem::Invoke(int button)
{
	if (INVOKE_LEFT & button)
	{
		if (Settings_menusBroamMode) return;

		TCHAR buff[1024];
		GetWindowText(hText, buff, 1024);
		replace_str(&m_pszTitle, buff);
		SendMessage(hText, EM_SETMODIFY, FALSE, 0);
		if (NULL == m_pMenu->m_IDString)
			SetWindowText(hText, m_pszTitle);

		TCHAR result[1024];
		_sntprintf_s(result, 1024, _TRUNCATE, m_pszCommand, buff);
		post_command(result);
	}
}

//===========================================================================
void StringItem::SetPosition(void)
{
	if (Settings_menusBroamMode)
	{
		if (hText) DestroyWindow(hText);
		return;
	}

	if (NULL == hText)
	{
		hText = CreateWindow(
			_T("EDIT"),
			NULL,
			WS_CHILD
			| ES_AUTOHSCROLL
			| ES_MULTILINE,
			0, 0, 0, 0,
			m_pMenu->m_hwnd,
			(HMENU)1234,
			hMainInstance,
			NULL
			);

		SetWindowLongPtr(hText, GWLP_USERDATA, (LONG_PTR)this);
		wpEditProc = (WNDPROC)SetWindowLongPtr(hText, GWLP_WNDPROC, (LONG_PTR)EditProc);
		SetWindowText(hText, m_pszTitle);
		int n = GetWindowTextLength(hText);
		PostMessage(hText, EM_SETSEL, n, n);
		PostMessage(hText, EM_SCROLLCARET, 0, 0);
	}

	HFONT hFont = MenuInfo.hFrameFont;
	SendMessage(hText, WM_SETFONT, (WPARAM)hFont, 0);

	RECT r; GetTextRect(&r);
	int h = r.bottom - r.top - 4;
	int w = r.right - r.left + 2;
	SetWindowPos(hText, NULL, r.left-1, r.top+2, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

	int padd = (int)imax(0, (h - get_fontheight(hFont)) / 2);
	r.left  = padd+2;
	r.right = w - (padd+2);
	r.top   = padd;
	r.bottom = h - padd;
	SendMessage(hText, EM_SETRECT, 0, (LPARAM)&r);
}

//===========================================================================

LRESULT CALLBACK StringItem::EditProc(HWND hText, UINT msg, WPARAM wParam, LPARAM lParam)
{
	StringItem *pStringItem = (StringItem*)GetWindowLongPtr(hText, GWLP_USERDATA);
	switch(msg)
	{
		// --------------------------------------------------------
		// Send Result

	case WM_MOUSEMOVE:
		PostMessage(GetParent(hText), WM_MOUSEMOVE, wParam, MAKELPARAM(10, pStringItem->m_nTop+2));
		break;

		// --------------------------------------------------------
		// Key Intercept

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_DOWN:
		case VK_UP:
			pStringItem->Key(msg, wParam);
			return 0;
		}
		break;

	case WM_CHAR:
		switch (wParam)
		{
		case VK_RETURN:
			pStringItem->Invoke(INVOKE_LEFT);
			pStringItem->next_item(0);
			return 0;

		case VK_ESCAPE:
			pStringItem->Key(msg, wParam);

		case VK_TAB:
			return 0;

		case _T('A') - 0x40: // ctrl-A: select all
			SendMessage(hText, EM_SETSEL, 0, GetWindowTextLength(hText));
			return 0;
		}
		break;

		// --------------------------------------------------------
		// Paint

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hText, &ps);
			RECT r; GetClientRect(hText, &r);
			HDC buf = CreateCompatibleDC(hdc);
			HGDIOBJ oldbuf = SelectObject(buf, CreateCompatibleBitmap(hdc, r.right, r.bottom));

			StyleItem *pSI = &mStyle.MenuFrame;
			MakeGradient(buf, r, pSI->type, pSI->Color, pSI->ColorTo, pSI->interlaced, BEVEL_SUNKEN, BEVEL1, 0, 0, 0);
			CallWindowProc(pStringItem->wpEditProc, hText, msg, (WPARAM)buf, lParam);

			BitBltRect(hdc, buf, &ps.rcPaint);
			DeleteObject(SelectObject(buf, oldbuf));
			DeleteDC(buf);
			EndPaint(hText, &ps);
			return 0;
		}

	case WM_ERASEBKGND:
		return TRUE;

	case WM_DESTROY:
		pStringItem->hText = NULL;
		break;

	case WM_SETFOCUS:
		pStringItem->m_pMenu->m_bHasFocus = true;
#ifdef CHECKFOCUS
		dbg_printf(_T("WM_SETFOCUS %s"), _T("edit"));
		InvalidateRect(pStringItem->m_pMenu->m_hwnd, NULL, FALSE);
#endif
		break;

	case WM_KILLFOCUS:
		if (SendMessage(hText, EM_GETMODIFY, 0, 0))
			pStringItem->Invoke(INVOKE_LEFT);

		pStringItem->Active(0);

		pStringItem->m_pMenu->m_bHasFocus = false;
#ifdef CHECKFOCUS
		dbg_printf(_T("WM_KILLFOCUS %s"), _T("edit"));
		InvalidateRect(pStringItem->m_pMenu->m_hwnd, NULL, FALSE);
#endif
		break;
	}
	return CallWindowProc(pStringItem->wpEditProc, hText, msg, wParam, lParam);
}

//===========================================================================

// SeparatorItem
SeparatorItem::SeparatorItem(): CommandItem(NULL, _T(""), false, NULL)
{
	m_ItemID = MENUITEM_ID_CIInt;
	m_isNOP = MI_NOP_SEP;
}

void SeparatorItem::Measure(HDC hDC, SIZE *size)
{
	StyleItem *pSI = &mStyle.MenuSeparator;
	size->cx = 0;
	size->cy = pSI->borderWidth + 4;
}

void SeparatorItem::Paint(HDC hDC)
{
	StyleItem *pSI = &mStyle.MenuSeparator;
	int margin = (int)imax(MenuInfo.nItemIndent + Settings_separatorMargin, 0);

	RECT r; GetItemRect(&r);
	_InflateRect(&r, -margin, -2);

	BYTE SepType = !_tcsicmp(Settings_separatorStyle, _T("toRight"))      |
		!_tcsicmp(Settings_separatorStyle, _T("toLeft"))  << 1 |
			!_tcsicmp(Settings_separatorStyle, _T("Outward")) << 2;
	int hue;
	int nWidth = GetRectWidth(&r);
	// draw separator
	if (nWidth > 0){
		for (int i = r.left; i < r.right; i++){
			for (int j = r.top; j < r.top + pSI->borderWidth; j++){
				switch (SepType){
				case 0x1 : // toRight
					hue = r.right - i;
					break;
				case 0x2 : // toLeft
					hue = i - r.left;
					break;
				case 0x4 : // Outward
					hue = (i <= r.left + nWidth/2 ? i - r.left : r.right - i) * 2;
					break;
				default :
					hue = nWidth;
					break;
				}
				hue = 255 * hue / nWidth;

				if ((pSI->validated & VALID_SHADOWCOLOR) && (i + 1 < r.right))
					SetPixel(hDC, i + 1, j + 1, mixcolors(pSI->ShadowColor, GetPixel(hDC, i + 1, j + 1), hue));
				SetPixel(hDC, i, j, mixcolors(pSI->borderColor, GetPixel(hDC, i, j), hue));
			}
		}
	}
}

