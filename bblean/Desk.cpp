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

#include "BB.h"
#include "Desk.h"
#include "Settings.h"
#include "Workspaces.h"
#include "MessageManager.h"
#include <shlobj.h>
#include <shellapi.h>
#include <process.h>

#define ST static

// changed for compatibility with ZMatrix
// const TCHAR szDesktopName[] = _T("BBLeanDesktop");
const TCHAR szDesktopName[] = _T("DesktopBackgroundClass");

HWND hDesktopWnd;
int focusmodel;

struct {
	HBITMAP bmp;
	TCHAR command[MAX_PATH];
} Root;

LRESULT CALLBACK Desk_WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
class DeskDropTarget *m_DeskDropTarget;
void Desk_Clear(void);

bool dont_hide_explorer = false;
bool dont_hide_tray = false;

// in bbroot.cpp
extern HBITMAP load_desk_bitmap(LPCTSTR command);

void init_DeskDropTarget(HWND hwnd);
void exit_DeskDropTarget(HWND hwnd);
bool get_drop_command(const TCHAR *filename, int flags);

//===========================================================================
struct hwnd_list *basebarlist;

void hidewnd (HWND hwnd, int f)
{
	//dbg_printf(_T("hw = %08x"), hw);
	if (hwnd && (ShowWindow(hwnd, SW_HIDE) || f))
		cons_node(&basebarlist, new_node(hwnd));
}

BOOL CALLBACK EnumExplorerWindowsProc(HWND hwnd, LPARAM lParam)
{
	TCHAR StringTmp[256];
	if (GetClassName(hwnd, StringTmp, 256) && 0==_tcscmp(StringTmp, _T("BaseBar")))
		hidewnd(hwnd, 0);
	return TRUE;
}

void HideExplorer()
{
	HWND hw;
	hw = FindWindow(_T("Progman"), _T("Program Manager"));
	if (hw)
	{
		if (false == dont_hide_explorer)
		{
			//hw = FindWindowEx(hw, NULL, _T("SHELLDLL_DefView"), NULL);
			//hw = FindWindowEx(hw, NULL, _T("SysListView32"), NULL);
			hidewnd(hw, 0);
		}
		else
			MakeSticky(hw);
	}
	hw = FindWindow(ShellTrayClass, NULL);
	if (hw)
	{
		if (false == dont_hide_tray)
		{
			hidewnd(hw, 1);
			EnumWindows((WNDENUMPROC)EnumExplorerWindowsProc, 0);
		}
		else
			MakeSticky(hw);
	}
}

void ShowExplorer()
{
	struct hwnd_list *p;
	dolist (p, basebarlist) ShowWindow(p->hwnd, SW_SHOW);
	freeall(&basebarlist);
}

//===========================================================================
void set_focus_model(const TCHAR *fm_string)
{
	int fm = 0;
	if (0==_tcsicmp(fm_string, _T("SloppyFocus")))
		fm = 1;
	if (0==_tcsicmp(fm_string, _T("AutoRaise")))
		fm = 3;

	if (fm==focusmodel) return;
	focusmodel = fm;

	if (fm)
		SystemParametersInfo(SPI_SETACTIVEWNDTRKTIMEOUT,  0, (PVOID)Settings_autoRaiseDelay, SPIF_SENDCHANGE);
	SystemParametersInfo(SPI_SETACTIVEWINDOWTRACKING, 0, (PVOID)(0!=(fm & 1)), SPIF_SENDCHANGE);
	SystemParametersInfo(SPI_SETACTIVEWNDTRKZORDER,   0, (PVOID)(0!=(fm & 2)), SPIF_SENDCHANGE);
}

//===========================================================================
void (*pSetDesktopMouseHook)(HWND BlackboxWnd, bool withExplorer);
void (*pUnsetDesktopMouseHook)();
HMODULE DestopHookInstance;

// --------------------------------------------
void Desk_Init(void)
{
	set_focus_model(Settings_focusModel);

	if (Settings_desktopHook || dont_hide_explorer)
	{
		DestopHookInstance = LoadLibrary(_T("DesktopHook"));
		*(FARPROC*)&pSetDesktopMouseHook = GetProcAddress(DestopHookInstance, "SetDesktopMouseHook" );
		*(FARPROC*)&pUnsetDesktopMouseHook = GetProcAddress(DestopHookInstance, "UnsetDesktopMouseHook" );
		if (pSetDesktopMouseHook)
			pSetDesktopMouseHook(BBhwnd, underExplorer);
		else
			BBMessageBox(MB_OK, NLS2(_T("$BBError_DesktopHook$"),
									 _T("Error: DesktopHook.dll not found!")));
	}
	else
	{
		WNDCLASSEX wc;
		ZeroMemory(&wc, sizeof(wc));
		wc.hInstance = hMainInstance;
		wc.lpfnWndProc = Desk_WndProc;
		wc.lpszClassName = szDesktopName;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.style = CS_DBLCLKS;

		BBRegisterClass(&wc);
		CreateWindowEx(
			WS_EX_TOOLWINDOW,
			szDesktopName,
			NULL,
			WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
			0,0,0,0,
			GetDesktopWindow(),
			NULL,
			hMainInstance,
			NULL
			);
	}
	Desk_new_background();
}

// --------------------------------------------
void Desk_Exit()
{
	if (pUnsetDesktopMouseHook)
	{
		pUnsetDesktopMouseHook();
		FreeLibrary(DestopHookInstance);
	}
	if (hDesktopWnd)
	{
		DestroyWindow(hDesktopWnd);
		UnregisterClass(szDesktopName, hMainInstance);
		hDesktopWnd = NULL;
	}
	Desk_Clear();
	set_focus_model(_T(""));
}

//===========================================================================
void Desk_SetPosition()
{
	SetWindowPos(
		hDesktopWnd,
		HWND_BOTTOM,
		VScreenX, VScreenY, VScreenWidth, VScreenHeight,
		SWP_NOACTIVATE
		);
}

//===========================================================================
void Desk_Clear(void)
{
	if (Root.bmp)
		DeleteObject(Root.bmp), Root.bmp = NULL;

	if (hDesktopWnd)
		InvalidateRect(hDesktopWnd, NULL, FALSE);
}

void Desk_reset_rootCommand(void)
{
	Root.command [0] = 0;
}

//===========================================================================
const TCHAR * Desk_extended_rootCommand(const TCHAR *p)
{
	const TCHAR rc_key [] = _T("blackbox.background.rootCommand:");
	const TCHAR *extrc = extensionsrcPath();
	if (p) WriteString(extrc, rc_key, p);
	else p = ReadString(extrc, rc_key, NULL);
	return p;
}

//===========================================================================
HANDLE hDTThread;

void __cdecl load_root_thread(void *pv)
{
	HBITMAP bmp = NULL;
	if (Root.command[0]) bmp = load_desk_bitmap(Root.command);
	PostMessage(hDesktopWnd, WM_USER, 0, (LPARAM)bmp);
	//CloseHandle(hDTThread);
	hDTThread = NULL;
}

void Desk_new_background(const TCHAR *p)
{
	p = Desk_extended_rootCommand(p);
	if (p)
	{
		if (0 == _tcsicmp(p, _T("none"))) p = _T("");
		if (0 == _tcsicmp(p, _T("style"))) p = NULL;
	}

	if (false == Settings_background_enabled)
		p = _T("");
	else
		if (NULL == p)
			p = mStyle.rootCommand;

	if (0 == _tcscmp(Root.command, p))
		return;

	_tcsncpy_s(Root.command, MAX_PATH, p, _TRUNCATE);

	if (NULL == hDesktopWnd || false == Settings_smartWallpaper)
	{
		// use Windows Wallpaper?
		Desk_Clear();
		if (Root.command[0]) BBExecute_string(Root.command, true);
	}
	else
	{
		if (hDTThread) WaitForSingleObject(hDTThread, INFINITE);
		hDTThread = (HANDLE)_beginthread(load_root_thread, 0, NULL);
	}
}

//===========================================================================

LRESULT CALLBACK Desk_WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static UINT msgs [] = { BB_DRAGTODESKTOP, 0 };
	static bool button_down;
	int n = 0;
	int nDelta;
	switch (uMsg)
	{
		//====================
	case WM_CREATE:
		hDesktopWnd = hwnd;
		DisableInputMethod(hwnd);
		MakeSticky(hwnd);
		MessageManager_AddMessages(hwnd, msgs);
		Desk_SetPosition();
		init_DeskDropTarget(hwnd);
		break;

		//====================
	case WM_DESTROY:
		exit_DeskDropTarget(hwnd);
		MessageManager_RemoveMessages(hwnd, msgs);
		RemoveSticky(hwnd);
		break;

	case WM_USER:
		Desk_Clear();
		Root.bmp = (HBITMAP)lParam;
	case WM_NCPAINT:
		Desk_SetPosition();
		break;

		//====================
	case WM_CLOSE:
		break;

		//====================
	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		button_down = true;
		break;

		//====================
	case WM_LBUTTONUP: n = 0; goto post_click;
	case WM_RBUTTONUP: n = 1; goto post_click;
	case WM_MBUTTONUP: n = 2; goto post_click;
	case WM_XBUTTONUP:
		switch (HIWORD(wParam)) {
		case XBUTTON1: n = 3; goto post_click;
		case XBUTTON2: n = 4; goto post_click;
		//case XBUTTON3: n = 5; goto post_click;
		} break;

	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK: n = 6;
		button_down = true;
		goto post_click;

	case WM_MOUSEWHEEL:
		nDelta = (short)HIWORD(wParam);
		if (nDelta > 0) n =  7; // Wheel UP
		if (nDelta < 0) n =  8; // Wheel Down
		button_down = true;
		goto post_click;

	post_click:
		if (button_down) PostMessage(BBhwnd, BB_DESKCLICK, 0, n);
		button_down = false;
		//            PostMessage(BBhwnd, BB_DESKCLICK, 0, n);
		break;

		//====================
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc_scrn = BeginPaint(hwnd, &ps);
			if (Root.bmp)
			{
				HDC hdc_bmp = CreateCompatibleDC(hdc_scrn);
				HGDIOBJ other = SelectObject(hdc_bmp, Root.bmp);
				BitBltRect(hdc_scrn, hdc_bmp, &ps.rcPaint);
				SelectObject(hdc_bmp, other);
				DeleteDC(hdc_bmp);
			}
			else
			{
				PaintDesktop(hdc_scrn);
			}
			EndPaint(hwnd, &ps);
			break;
		}

		//====================
	case WM_ERASEBKGND:
		return TRUE;

		//====================
	case BB_DRAGTODESKTOP:
		return get_drop_command((const TCHAR *)lParam, (int)wParam);

		//====================
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);

	}
	return 0;
}

//===========================================================================
const short vk_codes[] = { VK_MENU, VK_SHIFT, VK_CONTROL, VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
const char mk_mods[] = { MK_ALT, MK_SHIFT, MK_CONTROL, MK_LBUTTON, MK_RBUTTON, MK_MBUTTON };
const TCHAR modkey_strings_r[][6] = { _T("Alt"), _T("Shift"), _T("Ctrl"), _T("Left"), _T("Right"), _T("Mid") };
const TCHAR modkey_strings_l[][6] = { _T("Alt"), _T("Shift"), _T("Ctrl"), _T("Right"), _T("Left"), _T("Mid") };
const TCHAR button_strings[][10] = { _T("Left"), _T("Right"), _T("Mid"), _T("X1"), _T("X2"), _T("X3"), _T("Double"), _T("WheelUp"), _T("WheelDown")};
//===========================================================================
unsigned get_modkeys(void)
{
	unsigned modkey = 0;
	for (int i = 0; i < (int)(sizeof(vk_codes)/sizeof(vk_codes[0])); i++){
		if (0x8000 & GetAsyncKeyState(vk_codes[i])) modkey |= mk_mods[i];
	}
	return modkey;
}

bool Desk_mousebutton_event(LPARAM button)
{
	TCHAR rc_key[80] = _T("blackbox.desktop.");
	TCHAR *p;
	const TCHAR (*modkey_strings)[6] = GetSystemMetrics(SM_SWAPBUTTON) ? &modkey_strings_l[0] : &modkey_strings_r[0];

	unsigned modkey = get_modkeys();
	for (int i = 0; i < (int)(sizeof(mk_mods)/sizeof(mk_mods[0])); i++){
		if (mk_mods[i] & modkey) _tcsncat_s(rc_key, 80, modkey_strings[i], _TRUNCATE);
	}

	if (button >= (int)(sizeof(button_strings)/sizeof(button_strings[0]))) return false;
	p = _tcschr(rc_key,0);
	if (button >= 7){
		_sntprintf_s(p, 80-(p-rc_key), _TRUNCATE, _T("%s:"), button_strings[button]); // WheelUp/WheelDown
	}
	else{
		_sntprintf_s(p, 80-(p-rc_key), _TRUNCATE, _T("%sClick:"), button_strings[button]);
	}
	const TCHAR *broam = ReadString(extensionsrcPath(), rc_key, NULL);

	if (broam)
		post_command(broam);
	else
		if (1 == button && 0 == modkey)
			PostMessage(BBhwnd, BB_MENU, BB_MENU_ROOT, 0);
		else
			if ((2 == button && 0 == modkey) || (1 == button && MK_SHIFT == modkey))
				PostMessage(BBhwnd, BB_MENU, BB_MENU_TASKS, 0);
			else
				return false;

	return true;
}

bool get_drop_command(const TCHAR *filename, int flags)
{
	TCHAR buffer[MAX_PATH + 100];

	const TCHAR *e = _tcsrchr(filename, _T('.'));
	if (e && stristr(_T(".bmp.gif.png.jpg.jpeg"), e))
	{
		unsigned modkey = get_modkeys();
		const TCHAR *mode;

		if (MK_SHIFT == modkey) mode = _T("center");
		else
			if (MK_CONTROL == modkey) mode = _T("tile");
			else mode = _T("full");

		if (0 == (flags & 1))
		{
			_sntprintf_s(buffer, MAX_PATH+100, _TRUNCATE, _T("@BBCore.rootCommand bsetroot -%s \"%s\""), mode, filename);
			post_command(buffer);
		}
		return true;
	}

	if (flags & 2)
	{
		if (0 == (flags & 1))
		{
			_sntprintf_s(buffer, MAX_PATH+100, _TRUNCATE, _T("@BBCore.style %s"), filename);
			post_command(buffer);
		}
		return true;
	}

	return false;
}

//===========================================================================

class DeskDropTarget : public IDropTarget
{
public:
	DeskDropTarget()
	{
		m_dwRef = 1;
	};

	virtual ~DeskDropTarget()
	{
	};

	STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject)
	{
		if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDropTarget))
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		return ++m_dwRef;
	}

	STDMETHOD_(ULONG, Release)()
	{
		int tempCount = --m_dwRef;
		if (0 == tempCount) delete this;
		return tempCount;
	}

	STDMETHOD (DragEnter) (LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
	{
		AddRef();
		m_filename[0] = 0;
		if (pDataObject)
		{
			FORMATETC fmte;
			fmte.cfFormat   = CF_HDROP;
			fmte.ptd        = NULL;
			fmte.dwAspect   = DVASPECT_CONTENT;
			fmte.lindex     = -1;
			fmte.tymed      = TYMED_HGLOBAL;

			STGMEDIUM medium;
			if (SUCCEEDED(pDataObject->GetData(&fmte, &medium)))
			{
				HDROP hDrop = (HDROP)medium.hGlobal;
				m_filename[0] = 0;
				DragQueryFile(hDrop, 0, m_filename, sizeof(m_filename)/sizeof(TCHAR));
				DragFinish(hDrop);
				m_flags = is_stylefile(m_filename) ? 2 : 0;
			}
		}
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}

	STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
	{
		*pdwEffect = DROPEFFECT_NONE;
		if (m_filename[0] && SendMessage(BBhwnd, BB_DRAGTODESKTOP, m_flags | 1, (LPARAM)m_filename))
			*pdwEffect = DROPEFFECT_LINK;
		return S_OK;
	}

	STDMETHOD(Drop)(LPDATAOBJECT pDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
	{
		*pdwEffect = DROPEFFECT_NONE;
		if (m_filename[0] && SendMessage(BBhwnd, BB_DRAGTODESKTOP, m_flags, (LPARAM)m_filename))
			*pdwEffect = DROPEFFECT_LINK;
		Release();
		return S_OK;
	}

	STDMETHOD(DragLeave)()
	{
		Release();
		return S_OK;
	}

private:
	DWORD m_dwRef;
	TCHAR m_filename[MAX_PATH];
	int m_flags;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// interface

void init_DeskDropTarget(HWND hwnd)
{
	m_DeskDropTarget = new DeskDropTarget();
	RegisterDragDrop(hwnd, m_DeskDropTarget);
}

void exit_DeskDropTarget (HWND hwnd)
{
	RevokeDragDrop(hwnd);
	m_DeskDropTarget->Release();
}

//===========================================================================
