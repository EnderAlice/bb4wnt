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

#ifndef __Tray_H
#define __Tray_H

//===========================================================================

void CleanTray(void);
void Tray_Init(void);
void Tray_Exit(void);

API_EXPORT(int) GetTraySize();
API_EXPORT(systemTray*) GetTrayIcon(int idx);

/* experimental: */
typedef BOOL (*TRAYENUMPROC)(struct systemTray *, LPARAM);
API_EXPORT(void) EnumTray(TRAYENUMPROC lpEnumFunc, LPARAM lParam);

//===========================================================================
#ifdef INCLUDE_NIDS

// ここでは変換のために使用するため、ハンドルは全て32ビット整数

typedef struct _NID_1
{
	DWORD cbSize;
	DWORD hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	DWORD hIcon;
	TCHAR szTip[64];
} NID_1,*PNID_1;

typedef struct _NID_2
{
	DWORD cbSize;
	DWORD hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	DWORD hIcon;
	TCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	TCHAR szInfo[256];
	union {
		UINT uTimeout;
		UINT uVersion;
	};
	TCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
} NID_2,*PNID_2;

typedef struct _NID_3
{
	DWORD cbSize;
	DWORD hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	DWORD hIcon;
	TCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	TCHAR szInfo[256];
	union {
		UINT uTimeout;
		UINT uVersion;
	};
	TCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
	GUID guidItem;
} NID_3,*PNID_3;

typedef struct _NID_4
{
	DWORD cbSize;
	DWORD hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	DWORD hIcon;
	TCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	TCHAR szInfo[256];
	union {
		UINT uTimeout;
		UINT uVersion;
	};
	TCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
	GUID guidItem;
	DWORD hBalloonIcon;
} NID_4,*PNID_4;

#endif

//===========================================================================
#endif
