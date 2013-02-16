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

/* BB.h - global defines aside of what is needed for the sdk-api */

#ifndef _BB_H
#define _BB_H

// ==============================================================
/* optional defines */

/* experimental nationalized language support */
//#define BBOPT_SUPPORT_NLS

/* misc. developement options */
//#define BBOPT_DEVEL

/* memory allocation tracking */
//#define BBOPT_MEMCHECK

/* core dump to file */
//#define BBOPT_STACKDUMP

//#define NDEBUG

// ==============================================================
/* compiler specifics */

//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500
//#define _WIN32_IE 0x0500
#define NO_INTSHCUT_GUIDS
#define NO_SHDOCVW_GUIDS

#ifdef __BORLANDC__
  #ifdef CRTSTRING
    #include "crt-string.h"
    #define TRY if (1)
    #define EXCEPT if(0)
  #else
    #define TRY __try
    #define EXCEPT __except(1)
  #endif
#endif

#ifdef __GNUC__
  #define TRY if (1)
  #define EXCEPT if(0)
#endif

#ifdef _MSC_VER
  #ifdef BBOPT_STACKDUMP
    #define TRY if (1)
    #define EXCEPT if(0)
  #else
    #define TRY _try
    #define EXCEPT _except(1)
  #endif
  #define stricmp _stricmp
  #define strnicmp _strnicmp
  #define memicmp _memicmp
  #define strlwr _strlwr
  #define strupr _strupr
#else
  #undef BBOPT_STACKDUMP
#endif

// ==============================================================

// ==============================================================
/* Always includes */

#ifndef __BBCORE__
#define __BBCORE__ // enable exports in BBApi.h
#endif
#include "BBApi.h"
//#include "win0x500.h"
#include "m_alloc.h"
#include "Tinylist.h"
#include <assert.h>

// ==============================================================
/* Blackbox icon and menu drag-cursor */

#define IDI_BLACKBOX 101
#define IDC_MOVEMENU 102

/* Convenience defs */
#define IS_SPC(c) ((TCHAR)(c) <= 32)
#define IS_SLASH(c) ((c) == '\\' || (c) == '/')
// ==============================================================
/* global variables */

extern OSVERSIONINFO osInfo;
extern bool         usingNT;
extern bool         usingWin2kXP;

extern HINSTANCE    hMainInstance;
extern HWND         BBhwnd;
extern int          VScreenX, VScreenY;
extern int          VScreenWidth, VScreenHeight;
extern int          ScreenWidth, ScreenHeight;
extern bool         underExplorer;
extern bool         multimon;
extern bool         dont_hide_explorer;
extern bool         dont_hide_tray;

extern const TCHAR   bb_exename[];

extern const TCHAR   ShellTrayClass[];
extern bool         bbactive;

extern void (WINAPI* pSwitchToThisWindow)(HWND, int);
extern BOOL (WINAPI* pTrackMouseEvent)(LPTRACKMOUSEEVENT lpEventTrack);

// ==============================================================
/* Blackbox window timers */
#define BB_CHECKWINDOWS_TIMER   2 /* refresh the VWM window list */
#define BB_WRITERC_TIMER        3
#define BB_RUNSTARTUP_TIMER     4
#define BB_COMDLL_WATCHER       5

/* SetDesktopMargin internal flags */
#define BB_DM_REFRESH -1
#define BB_DM_RESET -2

// ==============================================================
/* utils.cpp */

int BBMessageBox(int flg, const TCHAR *fmt, ...);
BOOL BBRegisterClass (WNDCLASSEX *wcp);

TCHAR *strcpy_max(TCHAR *dest, const TCHAR *src, size_t maxlen);
const TCHAR* stristr(const TCHAR *a, const TCHAR *b);
intptr_t get_string_index (const TCHAR *key, const TCHAR **string_list);
int get_substring_index (const TCHAR *key, const TCHAR **string_list);
int substr_icmp(const TCHAR *a, const TCHAR *b);
int get_false_true(const TCHAR *arg);
void set_bool(bool *v, const TCHAR *arg);
const TCHAR *false_true_string(bool f);
const TCHAR *string_empty_or_null(const TCHAR *s);
TCHAR *extract_string(TCHAR *dest, const TCHAR *src, size_t length);
int bb_sscanf(const TCHAR **src, const TCHAR *fmt, ...);

intptr_t imax(intptr_t a, intptr_t b);
intptr_t imin(intptr_t a, intptr_t b);
intptr_t iminmax(intptr_t i, intptr_t minval, intptr_t maxval);
bool is_alpha(int c);
bool is_num(int c);
bool is_alnum(int c);

TCHAR *replace_argument1(TCHAR *out, size_t out_size, const TCHAR *format, const TCHAR *arg);

/* color utilities */
COLORREF rgb (unsigned r,unsigned g,unsigned b);
COLORREF switch_rgb (COLORREF c);
COLORREF mixcolors(COLORREF c1, COLORREF c2, int mixfac);
unsigned greyvalue(COLORREF c);
void draw_line_h(HDC hDC, int x1, int x2, int y, int w, COLORREF C);

/* filenames */
const TCHAR *get_ext(const TCHAR *e);
const TCHAR *get_file(const TCHAR *path);
TCHAR *get_directory(TCHAR *buffer, size_t d_size, const TCHAR *path);
TCHAR* unquote(TCHAR *d, size_t d_size, const TCHAR *s);
TCHAR *add_slash(TCHAR *d, size_t d_size, const TCHAR *s);
const TCHAR *get_relative_path(const TCHAR *p);
TCHAR *replace_slashes(TCHAR *buffer, size_t buffer_size, const TCHAR *path);
bool is_relative_path(const TCHAR *path);
TCHAR *make_bb_path(TCHAR *dest, size_t dest_size, const TCHAR *src);

/* folder changed notification register */
UINT add_change_notify_entry(HWND hwnd, const struct _ITEMIDLIST *pidl);
void remove_change_notify_entry(UINT id_notify);

/* filetime */
int get_filetime(const TCHAR *fn, FILETIME *ft);
int check_filetime(const TCHAR *fn, FILETIME *ft0);

/* drawing */
void arrow_bullet (HDC buf, int x, int y, int d);
//void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r);
int get_fontheight(HFONT hFont);

/* other */
API_EXPORT(size_t) GetAppByWindow(HWND Window, LPTSTR processName, size_t processName_size);
INT_PTR EditBox(const TCHAR *caption, const TCHAR *message, const TCHAR *initvalue, TCHAR *buffer, size_t buffer_size);
// HWND GetRootWindow(HWND hwnd);
void SetOnTop (HWND hwnd);

/* Logging */
extern "C" void log_printf(int flag, const TCHAR *fmt, ...);
void reset_logging(void);

void init_log(void);
void exit_log(void);
void show_log(bool fShow);
void write_log(const TCHAR *szString);
int BBDrawText(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, StyleItem* pSI);
HICON GetIconFromHWND(HWND hWnd);

/* macro -> func */
void _CopyRect(RECT * lprcDst, const RECT * lprcSrc);
void _InflateRect(RECT * lprc, int dx, int dy);
void _OffsetRect(RECT * lprc, int dx, int dy);
void _SetRect(RECT * lprc, int xLeft, int yTop, int xRight, int yBottom);
void _CopyOffsetRect(RECT * lprcDst, const RECT * lprcSrc, int dx, int dy);
int GetRectWidth(const RECT * lprc);
int GetRectHeight(const RECT * lprc);
void BitBltRect(HDC hdc_to, HDC hdc_from, const RECT * lprc);

// ==============================================================
/* stackdump.cpp - stack trace */
DWORD except_filter( EXCEPTION_POINTERS *ep );

// ==============================================================
/* pidl.cpp - shellfolders */
bool sh_getfolderpath(LPTSTR szPath, size_t szPath_size, UINT csidl);
TCHAR* replace_shellfolders(TCHAR *buffer, size_t buffer_size, const TCHAR *path, bool search_path);

// ==============================================================
/* drag and drop - dragsource.cpp / droptarget.cpp */
void init_drop(HWND hwnd);
void exit_drop(HWND hwnd);
void drag_pidl(const struct _ITEMIDLIST *pidl);
class CDropTarget *init_drop_targ(HWND hwnd, const struct _ITEMIDLIST *pidl);
void exit_drop_targ(class CDropTarget  *);
bool in_drop(class CDropTarget *dt);

// ==============================================================
/* shell context menus */
Menu *GetContextMenu(const struct _ITEMIDLIST *pidl);

// ==============================================================
/* workspaces and tasks */
bool focus_top_window(void);
void ForceForegroundWindow(HWND theWin);
struct hwnd_list { struct hwnd_list *next; HWND hwnd; };

// ==============================================================
/* BBApi.cpp - some (non api) utils */

unsigned long getfileversion(const TCHAR *path, TCHAR *buffer, size_t buffer_size);

BOOL BBExecute_command(const TCHAR *command, const TCHAR *arguments, bool no_errors);
BOOL BBExecute_string(const TCHAR *s, bool no_errors);
bool ShellCommand(const TCHAR *cmd, const TCHAR *work_dir, bool wait);

/* tokenizer */
LPTSTR NextToken(LPTSTR buf, size_t buf_size, LPCTSTR *string, const TCHAR *delims = NULL);

/* rc-reader */
void reset_reader(void);
void write_rcfiles(void);
FILE *create_rcfile(const TCHAR *path);
TCHAR *read_file_into_buffer(const TCHAR *path, int max_len);
TCHAR scan_line(TCHAR **pp, TCHAR **ss, size_t *ll);
int match(const TCHAR *str, const TCHAR *pat);
bool read_next_line(FILE *fp, LPTSTR szBuffer, DWORD dwLength);
bool is_stylefile(const TCHAR *path);
bool is_newstyle(LPCTSTR path);
void read_extension_style(LPCTSTR fileName, LPCTSTR szKey);
void free_extension_style(void);

/* color parsing */
COLORREF ReadColorFromString(LPCTSTR string);

/* window */
void ClearSticky();
void dbg_window(HWND window, const TCHAR *fmt, ...);

/* generic hash function */
unsigned calc_hash(TCHAR *p, const TCHAR *s, size_t *pLen);

// ==============================================================
/* Blackbox.cpp */

void post_command(const TCHAR *cmd);
int exec_pidl(const _ITEMIDLIST *pidl, LPCTSTR verb, LPCTSTR arguments);
int get_workspace_number(const TCHAR *s);
void set_opaquemove(void);

/* Menu */
bool IsMenu(HWND hwnd);


#define SIGN(n) (((ptrdiff_t)(n) < 0) ? -1 : (((ptrdiff_t)(n) > 0) ? 1 : 0))

// ==============================================================
/* Experimental Nationalized Language support */

#ifdef BBOPT_SUPPORT_NLS
const TCHAR *nls1(const TCHAR *p);
const TCHAR *nls2a(const TCHAR *i, const TCHAR *p);
const TCHAR *nls2b(const TCHAR *p);
void free_nls(void);
#define NLS0(S) S
#define NLS1(S) nls1(S)
#define NLS2(I,S) nls2b(I S)
#else
#define free_nls()
#define NLS0(S) S
#define NLS1(S) (S)
#define NLS2(I,S) (S)
#endif

// ==============================================================
// for msvcrt.dll
#ifdef NOSTDLIB
#define memicmp  _memicmp
#define stricmp  _stricmp
#define stricoll _stricoll
#define strlwr   _strlwr
#define strnicmp _strnicmp
#define strupr   _strupr
#define	snprintf _snprintf
#define itoa     _itoa
#endif
#ifdef NOSTDLIB
#define _return ExitProcess
#else
#define _return return
#endif
// ==============================================================
#ifdef _UNICODE
#define _tmemmove wmemmove
#define _tmemcmp wmemcmp
#define _tmemchr wmemchr
#define _tmemcpy wmemcpy
#else
#define _tmemmove memmove
#define _tmemcmp memcmp
#define _tmemchr memchr
#define _tmemcpy memcpy
#endif
#endif
