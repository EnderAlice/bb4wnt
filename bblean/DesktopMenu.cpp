﻿/*
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
#include "Workspaces.h"
#include "Menu/MenuMaker.h"
#include <psapi.h>

//===========================================================================
struct en { Menu *m; int desk; HWND hwndTop; int i; };

BOOL task_enum_func(struct tasklist *tl, LPARAM lParam)
{
	struct en * en = (struct en *)lParam;
	int k = IsIconic(tl->hwnd);
	if ((k && en->desk < 0) || (0 == k && en->desk == tl->wkspc))
	{
		TCHAR buf[80];
		_sntprintf_s(buf, 80, _TRUNCATE, _T("@BBCore.ActivateTask %d"), en->i+1);
		MakeMenuItem(en->m, tl->caption, buf, en->desk >= 0 && tl->hwnd == en->hwndTop);
	}
	en->i++;
	return TRUE;
}

Menu * build_task_folder(int desk, const TCHAR *title, bool popup)
{
	TCHAR buf[80];
	_sntprintf_s(buf, 80, _TRUNCATE, (-1 == desk)? _T("IDRoot_icons") : _T("IDRoot_workspace%d"), desk+1);
	Menu *m = MakeNamedMenu(title, buf, popup);
	if (m)
	{
		struct en en = { m, desk, GetTopTask(), 0 };
		EnumTasks(task_enum_func, (LPARAM)&en);
	}
	return m;
}

Menu * GetTaskFolder(int n, bool popup)
{
	if (n < 0) return NULL;
	DesktopInfo DI; get_desktop_info(&DI, n);
	return build_task_folder(n, DI.name, popup);
}

struct dn { Menu *m; bool popup; };

BOOL desk_enum_func(struct DesktopInfo *DI, LPARAM lParam)
{
	struct dn *dn = (struct dn *)lParam;
	TCHAR buf[80];
	_sntprintf_s(buf, 80, _TRUNCATE, _T("@BBCore.SwitchToWorkspace %d"), DI->number+1);
	Menu *s = build_task_folder(DI->number, DI->name, dn->popup);
	MenuItem *fi = MakeSubmenu(dn->m, s, DI->name);
	SetFolderItemCommand(fi, buf);
	CheckMenuItem(fi, DI->isCurrent);
	return TRUE;
}

Menu* MakeIconsMenu(bool popup)
{
	Menu *m = build_task_folder(-1, NLS0(_T("Icons")), popup);
	return m;
}

Menu* MakeDesktopMenu(bool popup)
{
	Menu *m = MakeNamedMenu(NLS0(_T("Workspaces")), _T("IDRoot_workspaces"), popup);

	struct dn dn; dn.m = m; dn.popup = popup;
	EnumDesks(desk_enum_func, (LPARAM)&dn);

	MakeMenuNOP(m);
	MakeSubmenu(m, MakeIconsMenu(popup), NLS0(_T("Icons")));

	const TCHAR *CFG = NLS0(_T("New/Remove"));
	Menu *s = MakeNamedMenu(CFG, _T("IDRoot_workspaces_setup"), popup);
	MakeSubmenu(m, s, CFG);

	MakeMenuItem(s, NLS0(_T("New Workspace")), _T("@BBCore.AddWorkspace"), false);
	if (Settings_workspaces>1) MakeMenuItem(s, NLS0(_T("Remove Last")), _T("@BBCore.DelWorkspace"), false);
	MakeMenuItem(s, NLS0(_T("Edit Workspace Names")), _T("@BBCore.EditWorkspaceNames"), false);

	return m;
}

//===========================================================================

DWORD GetProcessName(DWORD processID, LPTSTR name, size_t name_size)
{
	HANDLE hProcess = OpenProcess(
		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
		FALSE,
		processID
	);

	// Get the process name.
	if (NULL != hProcess){
		HMODULE hMod;
		DWORD cbNeeded, dwSize = 0;
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)){
			dwSize = GetModuleBaseName(hProcess, (HINSTANCE__*)hMod, name, (DWORD)name_size);
		}
		CloseHandle(hProcess);
		return dwSize;
	}
	return 0;
}

Menu* MakeProcessesMenu(bool popup)
{
	DWORD ph[1024];
	DWORD phs;
	if(!EnumProcesses(ph, 1024, &phs)) phs=0;
	phs/=sizeof(DWORD);
	TCHAR buff[128];
	_sntprintf_s(buff, 128, _TRUNCATE, _T("Process Killer Menu - %ld"), phs);
	Menu *m = MakeNamedMenu(buff, _T("IDRoot_processes"), popup);

	for (int i=phs-1; i>=0; i--){
		TCHAR name[128], buf[80];
		if (GetProcessName(ph[i], name, 128)){
			_sntprintf_s(buf, 80, _TRUNCATE, _T("@KillProcess %ld"), ph[i]);
			MakeMenuItem(m, name, buf, false);
		}
	}
	return m;
}
