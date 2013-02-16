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
#include "Tinylist.h"
#include "MinimizeToTray.h"
#include "limits.h"

nid_list *nidl;

BOOL CALLBACK EnumWindowsHideProc(HWND hWnd, LPARAM lParam){
	nid_list *nl = (nid_list*)lParam;

	HWND hOwner = NULL;
	bool bChild = (GetWindowLongPtr(hWnd ,GWL_STYLE) & WS_CHILD) == WS_CHILD;

	if (!bChild)
		hOwner = (HWND)GetWindowLongPtr(hWnd, GWLP_HWNDPARENT); // 64bit

	if (hWnd == nl->hWnd){
		// hide window
		ShowWindow(hWnd, SW_HIDE);
		// remove task from tasks
		PostMessage(BBhwnd, BB_WORKSPACE, BBWS_REMOVETASK, (LPARAM)nl->hWnd_tl);
	}
	else if (hOwner == nl->hWnd_tl){
		if (IsWindowVisible(hWnd)){
			hWnd = GetAncestor(hWnd, GA_ROOTOWNER);
			// hide window
			ShowWindow(hWnd, SW_HIDE);
			// remove task from tasks
			PostMessage(BBhwnd, BB_WORKSPACE, BBWS_REMOVETASK, (LPARAM)nl->hWnd_tl);
			nl->hWnd = hWnd;
		}
	}
	return TRUE;
}

int MinimizeToTray(HWND hWnd, HWND hWnd_tl){
	// Get Icon
	HICON hIcon = GetIconFromHWND(hWnd_tl);
	static UINT i = UINT_MAX;

	// create NOTIFYICONDATA node
	nid_list *nl = (nid_list*)c_alloc(sizeof(nid_list));
	nl->NID.cbSize = sizeof(NOTIFYICONDATA);
	nl->NID.hWnd = BBhwnd;
	nl->NID.uID = i--;
	nl->NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nl->NID.uCallbackMessage = WM_TRAYICONMESSAGE;
	nl->NID.hIcon = hIcon;
	GetWindowText(hWnd, nl->NID.szTip, sizeof(nl->NID.szTip));
	nl->hWnd = hWnd;
	nl->hWnd_tl = hWnd_tl;

	// add icon to tray
	Shell_NotifyIcon(NIM_ADD, &nl->NID);
	EnumWindows(EnumWindowsHideProc, (LPARAM)nl);
	// append node to nidl
	append_node(&nidl, nl);

	return 1;
}

int RestoreFromTray(UINT uID){
	nid_list *p;
	dolist(p, nidl){
		if(p->NID.uID == uID){
			// remove icon from tray
			Shell_NotifyIcon(NIM_DELETE, &p->NID);
			// add task to tasks
			PostMessage(BBhwnd, BB_WORKSPACE, BBWS_ADDTASK, (LPARAM)p->hWnd_tl);
			// activate window
			ShowWindow(p->hWnd, SW_SHOW);
			PostMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)p->hWnd);
			// remove node from nidl
			remove_item(&nidl, p);
			break;
		}
	}
	return 1;
}

int RemoveFromTray(HWND hWnd){
	nid_list *p;
	dolist(p, nidl){
		if(p->hWnd == hWnd){
			// remove icon from tray
			Shell_NotifyIcon(NIM_DELETE, &p->NID);
			// remove node from nidl
			remove_item(&nidl, p);
			break;
		}
	}
	return 1;
}

void FreeNIDList(){
	nid_list *p;
	dolist(p, nidl){ // 非表示にしたウィンドウを戻してからリスト開放
		if(p->NID.uID) ShowWindow(p->hWnd, SW_SHOW);
	}
	freeall(&nidl);
}

