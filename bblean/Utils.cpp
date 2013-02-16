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
#include "Settings.h"
#include "resource.h"
#include <time.h>
#include <shlobj.h>
#include <shellapi.h>

//===========================================================================
// API: BBMessageBox
// Purpose:  standard BB-MessageBox
//===========================================================================

int BBMessageBox(int flg, const TCHAR *fmt, ...)
{
	TCHAR buffer[10000], *msg = buffer, *caption = _T("bbLean"), *p;
	va_list arg; va_start(arg, fmt); _vsntprintf_s (buffer, 10000, _TRUNCATE, fmt, arg);
	if (_T('#') == msg[0] && NULL != (p = _tcschr(msg+1, msg[0])))
		caption = msg+1, *p=0, msg = p+1;

	//return MessageBox (BBhwnd, msg, caption, flg | MB_SYSTEMMODAL | MB_SETFOREGROUND);

	MSGBOXPARAMS mp; ZeroMemory(&mp, sizeof mp);
	mp.cbSize = sizeof mp;
	mp.hInstance = hMainInstance;
	//mp.hwndOwner = NULL;
	mp.lpszText = msg;
	mp.lpszCaption = caption;
	mp.dwStyle = flg | MB_SYSTEMMODAL | MB_SETFOREGROUND | MB_USERICON;
	mp.lpszIcon = MAKEINTRESOURCE(IDI_BLACKBOX);
	MessageBeep(0);
	return MessageBoxIndirect(&mp);
}

//===========================================================================
// Function: BBRegisterClass
// Purpose:  Register a window class, display error on failure
//===========================================================================

BOOL BBRegisterClass (WNDCLASSEX *pWC)
{
	pWC->cbSize = sizeof(WNDCLASSEX);
	if(RegisterClassEx(pWC)) return 1;
	BBMessageBox(MB_OK, NLS2(_T("$BBError_RegisterClass$"),
							 _T("Error: Could not register \"%s\" window class. (%lu)")), pWC->lpszClassName, GetLastError());
	return 0;
}

//===========================================================================
// Function: rgb, switch_rgb
// Purpose: macro replacement and rgb adjust
//===========================================================================

COLORREF rgb (unsigned r,unsigned g,unsigned b)
{
	return RGB(r,g,b);
}

COLORREF switch_rgb (COLORREF c)
{
	return (c&0x0000ff)<<16 | (c&0x00ff00) | (c&0xff0000)>>16;
}

COLORREF mixcolors(COLORREF c1, COLORREF c2, int f)
{
	int n = 255 - f;
	return RGB(
		(GetRValue(c1)*f+GetRValue(c2)*n)>>8,
		(GetGValue(c1)*f+GetGValue(c2)*n)>>8,
		(GetBValue(c1)*f+GetBValue(c2)*n)>>8
		);
}

COLORREF shadecolor(COLORREF c, int f)
{
	int r = (int)GetRValue(c) + f;
	int g = (int)GetGValue(c) + f;
	int b = (int)GetBValue(c) + f;
	if (r < 0) r = 0; if (r > 255) r = 255;
	if (g < 0) g = 0; if (g > 255) g = 255;
	if (b < 0) b = 0; if (b > 255) b = 255;
	return RGB(r, g, b);
}

// unsigned greyvalue(COLORREF c)
// {
//     unsigned r = GetRValue(c);
//     unsigned g = GetGValue(c);
//     unsigned b = GetBValue(c);
//     return (r*79 + g*156 + b*21) / 256;
// }

void draw_line_h(HDC hDC, int x1, int x2, int y, int w, COLORREF C)
{
	HGDIOBJ oldPen = SelectObject(hDC, CreatePen(PS_SOLID, 1, C));
	while (w--){
		MoveToEx(hDC, x1, y, NULL);
		LineTo  (hDC, x2, y);
		++y;
	}
	DeleteObject(SelectObject(hDC, oldPen));
}

/*----------------------------------------------------------------------------*/

intptr_t imax(intptr_t a, intptr_t b) {
	return a>b?a:b;
}

intptr_t imin(intptr_t a, intptr_t b) {
	return a<b?a:b;
}

intptr_t iminmax(intptr_t a, intptr_t b, intptr_t c) {
	if (a<b) a=b;
	if (a>c) a=c;
	return a;
}

bool is_alpha(int c)
{
	return c >= _T('A') && c <= _T('Z') || c >= _T('a') && c <= _T('z');
}

bool is_num(int c)
{
	return c >= _T('0') && c <= _T('9');
}

bool is_alnum(int c)
{
	return is_alpha(c) || is_num(c);
}


//===========================================================================
TCHAR *extract_string(TCHAR *dest, const TCHAR *src, size_t n)
{
	memcpy(dest, src, sizeof(TCHAR) * n);
	dest[n] = 0;
	return dest;
}

//===========================================================================
// Function: strcpy_max
// Purpose:  copy a string with maximal length (a safer strncpy)
//===========================================================================

TCHAR *strcpy_max(TCHAR *dest, const TCHAR *src, size_t maxlen)
{
	return extract_string(dest, src, (size_t)imin(_tcslen(src), maxlen-1));
}

//===========================================================================
// Function: stristr
// Purpose:  ignore case strstr
//===========================================================================

const TCHAR* stristr(const TCHAR *aa, const TCHAR *bb)
{
	const TCHAR *a, *b; TCHAR c, d;
	do
	{
		for (a = aa, b = bb;;++a, ++b)
		{
			if (0 == (c = *b)) return aa;
			if (0 != (d = *a^c))
				if (d != 32 || (c |= 32) < _T('a') || c > _T('z'))
					break;
		}
	} while (*aa++);

	return NULL;
}

//===========================================================================
const TCHAR *string_empty_or_null(const TCHAR *s)
{
	return NULL==s ? _T("<null>") : 0==*s ? _T("<empty>") : s;
}

//===========================================================================
// Function: unquote
// Purpose:  remove quotes from a string (if any)
//===========================================================================

TCHAR* unquote(TCHAR *d, size_t d_size, const TCHAR *s)
{
	size_t l = _tcslen(s);
	if (l >= 2)
	{
		if((s[0] == _T('\"')) || (s[0] == _T('\'')))
		{
			if(s[l-1] == s[0])
			{
				s++;
				l-=2;
			}
		}
	}
	return extract_string(d, s, imin(d_size,l));
}

//===========================================================================
// Function: get_ext
// Purpose: get extension part of filename
// In:      filepath, extension to query
// Out:     * to extension or to terminating 0
//===========================================================================

const TCHAR *get_ext(const TCHAR *path)
{
	size_t nLen = _tcslen(path);
	size_t n = nLen;
	while (n) { if (path[--n] == _T('.')) return path + n; }
	return path + nLen;
}

//===========================================================================
// Function: get_file
// Purpose:  get the pointer to the filename
// In:
// Out:
//===========================================================================

const TCHAR *get_file(const TCHAR *path)
{
	size_t nLen = _tcslen(path);
	while (nLen && !IS_SLASH(path[nLen-1])) nLen--;
	return path + nLen;
}

//===========================================================================
// Function: get_file
// Purpose:  get the pointer to the filename
// In:
// Out:
//===========================================================================

TCHAR *get_directory(TCHAR *buffer, size_t buffer_size, const TCHAR *path)
{
	if (is_alpha(path[0]) && _T(':') == path[1] && IS_SLASH(path[2]))
	{
		const TCHAR *f = get_file(path);
		if (f > &path[3]) --f;
		return extract_string(buffer, path, imin(f-path,buffer_size));
	}
	buffer[0] = 0;
	return buffer;
}

//===========================================================================
// Function: get_relative_path
// Purpose:  get the sub-path, if the path is in the blackbox folder,
// In:       path to check
// Out:      pointer to subpath or full path otherwise.
//===========================================================================

const TCHAR *get_relative_path(const TCHAR *p)
{
	TCHAR home[MAX_PATH];
	GetBlackboxPath(home, MAX_PATH);
	size_t n = _tcslen(home);
	if (0 == _tcsnicmp(p, home, n)) return p + n;
	return p;
}

//===========================================================================
// Function: is_relative_path
// Purpose:  check, if the path is relative
// In:
// Out:
//===========================================================================

bool is_relative_path(const TCHAR *path)
{
	if (IS_SLASH(path[0])) return false;
	if (_tcschr(path, _T(':'))) return false;
	return true;
}

//===========================================================================

TCHAR *replace_slashes(TCHAR *buffer, size_t buffer_size, const TCHAR *path)
{
	const TCHAR *p = path; TCHAR *b = buffer; TCHAR c;
	do
	{
		if((size_t)(b - buffer) >= buffer_size)
		{
			break;
		}
		*b++ = _T('/') == (c = *p++) ? _T('\\') : c;
	} while (c);
	return buffer;
}


//===========================================================================
// Function: add_slash
// Purpose:  add \ when not present
//===========================================================================

TCHAR *add_slash(TCHAR *d, size_t d_size, const TCHAR *s)
{
	size_t l; _tcsncpy_s(d, d_size, s, _TRUNCATE); l = _tcslen(s);
	if (l && !IS_SLASH(d[l-1])) d[l++] = _T('\\');
	d[l] = 0;
	return d;
}

//===========================================================================
// Function: make_bb_path
// Purpose:  add the blackbox path as default
// In:
// Out:
//===========================================================================

TCHAR *make_bb_path(TCHAR *dest, size_t dest_size, const TCHAR *src)
{
	dest[0]=0;
	if (is_relative_path(src)) GetBlackboxPath(dest, MAX_PATH);
	_tcscat_s(dest, dest_size, src);
	return dest;
}

//===========================================================================
// Function: substr_icmp
// Purpose:  strcmp for the second string as start of the first
//===========================================================================

int substr_icmp(const TCHAR *a, const TCHAR *b)
{
	return _tcsnicmp(a, b, _tcslen(b));
}

//===========================================================================
// Function: get_substring_index
// Purpose:  search for a start-string match in a string array
// In:       searchstring, array
// Out:      index or -1
//===========================================================================

int get_substring_index (const TCHAR *key, const TCHAR **string_array)
{
	int i;
	for (i=0; *string_array; i++, string_array++)
		if (0==substr_icmp(key, *string_array)) return i;
	return -1;
}

//===========================================================================
// Function: get_string_index
// Purpose:  search for a match in a string array
// In:       searchstring, array
// Out:      index or -1
//===========================================================================

intptr_t get_string_index (const TCHAR *key, const TCHAR **string_array)
{
	const TCHAR **s;
	for (s = string_array; *s; ++s)
		if (0==_tcsicmp(key, *s)) return (intptr_t)(s - string_array);
	return -1;
}

//===========================================================================

int get_false_true(const TCHAR *arg)
{
	if (arg)
	{
		if (0==_tcsicmp(arg, _T("true"))) return true;
		if (0==_tcsicmp(arg, _T("false"))) return false;
	} return -1;
}

const TCHAR *false_true_string(bool f)
{
	return f ? _T("true") : _T("false");
}

void set_bool(bool *v, const TCHAR *arg)
{
	int f = get_false_true(arg);
	*v = -1 == f ? false == *v : (f != 0);
}

//===========================================================================

TCHAR *replace_argument1(TCHAR *d, size_t d_size, const TCHAR *s, const TCHAR *arg)
{
	TCHAR format[256], *p;
	_tcsncpy_s(format, 256, s, _TRUNCATE); p = _tcsstr(format, _T("%1"));
	if (p) p[1] = _T('s');
	_sntprintf_s(d, d_size, _TRUNCATE, format, arg);
	return d;
}

//===========================================================================
// Function: arrow_bullet
// Purpose:  draw the triangle bullet
// In:       HDC, position x,y, direction -1 or 1
// Out:
//===========================================================================

void arrow_bullet (HDC buf, int x, int y, int d)
{
	int s = mStyle.bulletUnix ? 1 : 2; int e = d;

	if (Settings_arrowUnix)
		x-=d*s, d+=d, e = 0;
	else
		s++, x-=d*s/2, d = 0;

	for (int i=-s, j=0; i<=s; i++)
	{
		j+=d;
		MoveToEx(buf, x,   y+i, NULL);
		j+=e;
		LineTo  (buf, x+j, y+i);
		if (0==i) d=-d, e=-e;
	}
}


// //===========================================================================
// // Function: BitBltRect
// //===========================================================================
// void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r)
// {
//     BitBlt(
//         hdc_to,
//         r->left, r->top, r->right-r->left, r->bottom-r->top,
//         hdc_from,
//         r->left, r->top,
//         SRCCOPY
//         );
// }

//===========================================================================
// Function: get_fontheight
//===========================================================================
int get_fontheight(HFONT hFont)
{
	TEXTMETRIC TXM;
	HDC hdc = CreateCompatibleDC(NULL);
	HGDIOBJ other = SelectObject(hdc, hFont);
	GetTextMetrics(hdc, &TXM);
	SelectObject(hdc, other);
	DeleteDC(hdc);
	return TXM.tmHeight;
}

//===========================================================================

//===========================================================================
int get_filetime(const TCHAR *fn, FILETIME *ft)
{
	WIN32_FIND_DATA data_bb;
	HANDLE h = FindFirstFile(fn, &data_bb);
	if (INVALID_HANDLE_VALUE==h) return 0;
	FindClose(h);
	*ft = data_bb.ftLastWriteTime;
	return 1;
}

int check_filetime(const TCHAR *fn, FILETIME *ft0)
{
	FILETIME ft;
	return get_filetime(fn, &ft) == 0 || CompareFileTime(&ft, ft0) != 0;// > 0;
}

//===========================================================================
// logging support
//===========================================================================
HANDLE hlog_file;

void log_printf(int flag, const TCHAR *fmt, ...)
{
	if ((Settings_LogFlag & flag) || 0 == (flag & 0x7FFF))
	{
		if (NULL == hlog_file)
		{
			TCHAR log_path[MAX_PATH];
			hlog_file = CreateFile(
				make_bb_path(log_path, MAX_PATH, _T("blackbox.log")),
				GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);

			SetFilePointer(hlog_file, 0, NULL, FILE_END);
			TCHAR date[32]; _tstrdate_s(date, 32);
			TCHAR time[10]; _tstrtime_s(time, 10);
			log_printf(flag, _T("\nStarting Log %s %s\n"), date, time);
		}

		TCHAR buffer[4096]; buffer[0] = 0;
		if (_T('\n') != *fmt)
		{
			if (0 == (0x8000 & flag)) _tstrtime_s(buffer, 4096);
			_tcsncat_s(buffer, 4096, _T("\t"), _TRUNCATE);
		}
		va_list arg; size_t b_s; va_start(arg, fmt); b_s = _tcsnlen(buffer,4096);
		_vsntprintf_s (buffer+b_s,4096-b_s, _TRUNCATE, fmt, arg);
		_tcsncat_s(buffer, 4096, _T("\n"), _TRUNCATE);
		DWORD written; WriteFile(hlog_file, buffer, (DWORD)_tcsnlen(buffer, 0xFFFFFFFFUL), &written, NULL);
	}
}

void reset_logging(void)
{
	if (hlog_file && 0 == Settings_LogFlag)
		CloseHandle(hlog_file), hlog_file = NULL;
}

//===========================================================================

//===========================================================================
void dbg_printf (const TCHAR *fmt, ...)
{
	TCHAR buffer[4096];
	va_list arg; va_start(arg, fmt);
	_vsntprintf_s (buffer, 4096, _TRUNCATE, fmt, arg);
	_tcsncat_s(buffer, 4096, _T("\n"), _TRUNCATE);
	OutputDebugString(buffer);
}

void dbg_window(HWND window, const TCHAR *fmt, ...)
{
	TCHAR buffer[4096];
	int x = GetClassName(window, buffer, 4096);
	x += _sntprintf_s(buffer+x, 4096-x, _TRUNCATE, _T(" <%lX>: "), (DWORD_PTR)window);
	va_list arg; va_start(arg, fmt);
	_vsntprintf_s (buffer+x, 4096-x, _TRUNCATE, fmt, arg);
	_tcsncat_s(buffer, 4096, _T("\n"), _TRUNCATE);
	OutputDebugString(buffer);
}

//===========================================================================

//===========================================================================
// API: GetAppByWindow
// Purpose:
// In:
// Out:
//===========================================================================
#include <tlhelp32.h>

// ToolHelp Function Pointers.
HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD,DWORD);
BOOL   (WINAPI *pModule32First)(HANDLE, LPMODULEENTRY32);
BOOL   (WINAPI *pModule32Next)(HANDLE, LPMODULEENTRY32);

// PSAPI Function Pointers.
DWORD  (WINAPI *pGetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD);
BOOL   (WINAPI *pEnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD);

API_EXPORT(size_t) GetAppByWindow(HWND Window, LPTSTR processName, size_t processName_size)
{
	processName[0]=0;
	DWORD pid;
	HANDLE hPr;

	GetWindowThreadProcessId(Window, &pid); // determine the process id of the window handle

	if (pCreateToolhelp32Snapshot && pModule32First && pModule32Next)
	{
		// grab all the modules associated with the process
		hPr = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if (hPr != INVALID_HANDLE_VALUE)
		{
			MODULEENTRY32 me;
			HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(Window, GWLP_HINSTANCE);

			me.dwSize = sizeof(me);
			if (pModule32First(hPr, &me))
				do
					if (me.hModule == hi)
					{
						_tcsncpy_s(processName, processName_size, me.szModule, _TRUNCATE);
						break;
					}
				while (pModule32Next(hPr, &me));
			CloseHandle(hPr);
		}
	}
	else
		if (pGetModuleBaseName && pEnumProcessModules)
		{
			HMODULE hMod; DWORD cbNeeded;
			hPr = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
			if (hPr != NULL)
			{
				if(pEnumProcessModules(hPr, &hMod, sizeof(hMod), &cbNeeded))
				{
					pGetModuleBaseName(hPr, hMod, processName, MAX_PATH);
				}
				CloseHandle(hPr);
			}
		}

	// dbg_printf(_T("appname = %s\n"), processName);
	return _tcslen(processName);
}

//===========================================================================

//===========================================================================
// Function: EditBox
// Purpose: Display a single line editcontrol
// In:
// Out:
//===========================================================================
BOOL CALLBACK dlgproc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR *buffer;
	size_t l;
	switch( msg )
	{
	case WM_DESTROY:
		RemoveProp(hDlg,TEXT("BufferSize"));
		RemoveProp(hDlg,TEXT("Buffer"));
		return 1;

	case WM_INITDIALOG:
		SetWindowText (hDlg, ((TCHAR**)lParam)[0]);
		SetDlgItemText(hDlg, 401, ((TCHAR**)lParam)[1]);
		SetDlgItemText(hDlg, 402, ((TCHAR**)lParam)[2]);
		buffer = ((TCHAR**)lParam)[3];
		SetProp(hDlg,TEXT("Buffer"),(HANDLE)buffer);
		SetProp(hDlg,TEXT("BufferSize"),(HANDLE)(UINT_PTR)((TCHAR**)lParam)[4]);
		MakeSticky(hDlg);
		{
			POINT p; GetCursorPos(&p);
			RECT m; GetMonitorRect(&p, &m, GETMON_WORKAREA|GETMON_FROM_POINT);
			RECT r; GetWindowRect(hDlg, &r);
#if 0
			// at cursor
			r.right -= r.left; r.bottom -= r.top;
			p.x = iminmax(p.x - r.right / 2,  m.left, m.right - r.right);
			p.y = iminmax(p.y - 10,  m.top, m.bottom - r.bottom);
#else
			// center screen
			p.x = (m.left + m.right - r.right + r.left) / 2;
			p.y = (m.top + m.bottom - r.bottom + r.top) / 2;
#endif
			SetWindowPos(hDlg, NULL, p.x, p.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		}
		return 1;

	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		SetForegroundWindow(hDlg);
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam ))
		{
		case IDOK:
			buffer = (TCHAR*)GetProp(hDlg,TEXT("Buffer"));
			l = (size_t)GetProp(hDlg,TEXT("BufferSize"));
			GetDlgItemText (hDlg, 402, buffer, (DWORD)l);
		case IDCANCEL:
			RemoveSticky(hDlg);
			EndDialog(hDlg, LOWORD(wParam));
			return 1;
		}

	default:
		break;
	}
	return 0;
}

INT_PTR EditBox(const TCHAR *caption, const TCHAR *message, const TCHAR *initvalue, TCHAR *buffer, size_t buffer_size)
{
	PCTSTR strarray[5];

	strarray[0] = caption;
	strarray[1] = message;
	strarray[2] = initvalue;
	strarray[3] = buffer;
	strarray[4] = (PCTSTR)buffer_size;

	// 1つ目の引数のポインタを使って2つ目の引数をアクセスすることは駄目
	// x86ではうまく動いていたが、x86-64では不可(最初の引数はレジスタを使うため)
	return DialogBoxParam(
		NULL, MAKEINTRESOURCE(400), NULL, (DLGPROC)dlgproc, (LPARAM)strarray);
}

//===========================================================================
// Function:  SetOnTop
// Purpose:   bring a window on top in case it is visible and not on top anyway
//===========================================================================

void SetOnTop (HWND hwnd)
{
	if (hwnd && IsWindowVisible(hwnd) && !(GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
		SetWindowPos(hwnd,
					 HWND_TOP,
					 0, 0, 0, 0,
					 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING
					 );
}

//===========================================================================
// Function: exec_pidl
//===========================================================================

int exec_pidl(const _ITEMIDLIST *pidl, LPCTSTR verb, LPCTSTR arguments)
{
	TCHAR szFullName[MAX_PATH];
	if (NULL == verb && SHGetPathFromIDList(pidl, szFullName))
		return BBExecute_command(szFullName, arguments, false);

	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei,sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.fMask = verb
		? SEE_MASK_IDLIST | SEE_MASK_FLAG_NO_UI
			: SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI
				;
	//sei.hwnd = NULL;
	sei.lpVerb = verb;
	//sei.lpFile = NULL;
	sei.lpParameters = arguments;
	//sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpIDList = (void*)pidl;
	return ShellExecuteEx(&sei);
}

//===========================================================================
// Function:
// Purpose:
// In:
// Out:
//===========================================================================

#ifndef SHCNF_ACCEPT_INTERRUPTS
//struct _SHChangeNotifyEntry
//{
//	const _ITEMIDLIST *pidl;
//	BOOL fRecursive;
//};
#define SHCNF_ACCEPT_INTERRUPTS 0x0001 
#define SHCNF_ACCEPT_NON_INTERRUPTS 0x0002
#define SHCNF_NO_PROXY 0x8000
#endif

#ifndef SHCNE_DISKEVENTS
#define SHCNE_DISKEVENTS    0x0002381FL
#define SHCNE_GLOBALEVENTS  0x0C0581E0L // Events that dont match pidls first
#define SHCNE_ALLEVENTS     0x7FFFFFFFL
#define SHCNE_INTERRUPT     0x80000000L // The presence of this flag indicates
#endif

extern UINT (WINAPI *pSHChangeNotifyRegister)(
    HWND hWnd, 
    DWORD dwFlags, 
    LONG wEventMask, 
    UINT uMsg, 
    DWORD cItems,
    struct _SHChangeNotifyEntry *lpItems
    );

extern BOOL (WINAPI *pSHChangeNotifyDeregister)(UINT ulID);

//===========================================================================
// Function:
// Purpose:
// In:
// Out:
//===========================================================================

UINT add_change_notify_entry(HWND hwnd, const _ITEMIDLIST *pidl)
{
	struct _SHChangeNotifyEntry E;
	E.pidl = pidl;
	E.fRecursive = FALSE;
	return pSHChangeNotifyRegister(
		hwnd,
		SHCNF_ACCEPT_INTERRUPTS|SHCNF_ACCEPT_NON_INTERRUPTS|SHCNF_NO_PROXY,
		SHCNE_ALLEVENTS,
		BB_FOLDERCHANGED,
		1,
		&E
		);
}

void remove_change_notify_entry(UINT id_notify)
{
	pSHChangeNotifyDeregister(id_notify);
}

//===========================================================================
// Function:
// Purpose:
// In:
// Out:
//===========================================================================

#ifdef BBOPT_SUPPORT_NLS

struct nls {
	struct nls *next;
	unsigned hash;
	int k;
	TCHAR *translation;
	TCHAR key[1];
};

struct nls *pNLS;
bool nls_loaded;

void free_nls(void)
{
	struct nls *t;
	dolist (t, pNLS) free_str(&t->translation);
	freeall(&pNLS);
	nls_loaded = false;
}

int decode_escape(TCHAR *d, const TCHAR *s)
{
	TCHAR c, e, *d0 = d; do
	{
		c = *s++;
		if (_T('\\') == c)
		{
			e = *s++;
			if (_T('n') == e) c = _T('\n');
			else
				if (_T('r') == e) c = _T('\r');
				else
					if (_T('t') == e) c = _T('\t');
					else
						if (_T('\"') == e || _T('\'') == e || _T('\\') == e) c = e;
						else --s;
		}
		*d++ = c;
	} while(c);
	return d - d0 - 1;
}

// -------------------------------------------

void load_nls(void)
{
	const TCHAR *lang_file =
		ReadString(extensionsrcPath(), _T("blackbox.options.language:"), NULL);
	if (NULL == lang_file) return;

	TCHAR full_path[MAX_PATH];
	FILE *fp = _tfopen (make_bb_path(full_path, lang_file), _T("rt, ccs=UTF-8"));
	if (NULL == fp) return;

	TCHAR line[4096], key[256], new_text[4096], *np; int nl;
	key[0] = 0;

	new_text[0] = 0; np = new_text; nl = 0;
	for (;;)
	{
		bool eof = false == read_next_line(fp, line, 4096);
		TCHAR *s = line, c = *s;
		if (_T('$') == c || eof)
		{
			if (key[0] && new_text[0])
			{
				struct nls *t = (struct nls *)c_alloc(sizeof *t + _tcslen(key));
				t->hash = calc_hash(t->key, key, &t->k);
				t->translation = new_str(new_text);
				cons_node(&pNLS, t);
			}
			if (eof) break;
			if (_T(' ') == s[1]) s += 2;
			decode_escape(key, s);
			new_text[0] = 0; np = new_text; nl = 0;
			continue;
		}

		if (_T('#') == c || _T('!') == c) continue;

		if (_T('\0') != c)
		{
			while (nl--) *np++ = _T('\n');
			np += decode_escape(np, s);
			nl = 0;
		}

		nl ++;
	}
	fclose(fp);
	reverse_list(&pNLS);
}

// -------------------------------------------
const TCHAR *nls2a(const TCHAR *i, const TCHAR *p)
{
	if (false == nls_loaded)
	{
		load_nls();
		nls_loaded = true;
	}

	if (pNLS)
	{
		TCHAR buffer[256]; int k; unsigned hash;
		hash = calc_hash(buffer, i, &k);
		struct nls *t;
		dolist (t, pNLS)
			if (t->hash==hash && 0==_tmemcmp(buffer, t->key, 1+k))
				return t->translation;
	}
	return p;
}

const TCHAR *nls2b(const TCHAR *p)
{
	const TCHAR *e;
	if (*p != _T('$') || NULL == (e = _tcschr(p+1, *p)))
		return p;

	++e;
	TCHAR buffer[256];
	return nls2a(extract_string(buffer, p, e-p), e);
}

const TCHAR *nls1(const TCHAR *p)
{
	return nls2a(p, p);
}

#endif

//===========================================================================
// Function: BBDrawText
// Purpose: draw text with shadow and/or outline
// In:
// Out:
//===========================================================================
int BBDrawText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, StyleItem* pSI){
	if (pSI->validated & VALID_SHADOWCOLOR){ // draw shadow
		RECT rcShadow;
		SetTextColor(hDC, pSI->ShadowColor);
		if (pSI->validated & VALID_OUTLINECOLOR){ // draw shadow with outline
			_CopyOffsetRect(&rcShadow, lpRect, 2, 0);
			DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow,  0, 1);
			DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow,  0, 1);
			DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow, -1, 0);
			DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow, -1, 0);
			DrawText(hDC, lpString, nCount, &rcShadow, uFormat);
		}
		else{
			_CopyOffsetRect(&rcShadow, lpRect, 1, 1);
			DrawText(hDC, lpString, nCount, &rcShadow, uFormat);
		}
	}
	if (pSI->validated & VALID_OUTLINECOLOR){ // draw outline
		RECT rcOutline;
		SetTextColor(hDC, pSI->OutlineColor);
		_CopyOffsetRect(&rcOutline, lpRect, 1, 0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0,  1);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,  -1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,  -1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0, -1);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0, -1);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   1,  0);
		DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
	}
	// draw text
	SetTextColor(hDC, pSI->TextColor);
	return DrawText(hDC, lpString, nCount, lpRect, uFormat);
}

//===========================================================================
// Function: GetIconFromHWND
// Purpose:
// In: WINDOW HANDLE
// Out: ICON HANDLE
//===========================================================================
HICON GetIconFromHWND(HWND hWnd){
	HICON hIcon;
	SendMessageTimeout(hWnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIcon);
	if(hIcon)
		return hIcon;

	hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
	if(hIcon)
		return hIcon;

	SendMessageTimeout(hWnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIcon);
	if(hIcon)
		return hIcon;

	hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
	if(hIcon)
		return hIcon;

	return NULL;
}

unsigned greyvalue(COLORREF c)
{
	return (GetRValue(c) * 77 + GetGValue(c) * 150 + GetBValue(c) * 29) >> 8;
}

void _CopyRect(RECT * lprcDst, const RECT * lprcSrc)
{
	*lprcDst = *lprcSrc;
}

void _InflateRect(RECT * lprc, int dx, int dy)
{
	lprc->left -= dx;
	lprc->right += dx;
	lprc->top -= dy;
	lprc->bottom += dy;
}

void _OffsetRect(RECT * lprc, int dx, int dy)
{
	lprc->left += dx;
	lprc->right += dx;
	lprc->top += dy;
	lprc->bottom += dy;
}

void _SetRect(RECT * lprc, int xLeft, int yTop, int xRight, int yBottom)
{
	lprc->left = xLeft;
	lprc->right = xRight;
	lprc->top = yTop;
	lprc->bottom = yBottom;
}

void _CopyOffsetRect(RECT * lprcDst, const RECT * lprcSrc, int dx, int dy)
{
	lprcDst->left = lprcSrc->left + dx;
	lprcDst->right = lprcSrc->right + dx;
	lprcDst->top = lprcSrc->top + dy;
	lprcDst->bottom = lprcSrc->bottom + dy;
}

int GetRectWidth(const RECT * lprc)
{
	return lprc->right - lprc->left;
}

int GetRectHeight(const RECT * lprc)
{
	return lprc->bottom - lprc->top;
}

void BitBltRect(HDC hdc_to, HDC hdc_from, const RECT * lprc)
{
	BitBlt(hdc_to, lprc->left, lprc->top, GetRectWidth(lprc), GetRectHeight(lprc), hdc_from, lprc->left, lprc->top, SRCCOPY);
}
