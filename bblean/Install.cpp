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

// perform the 'install / uninstall' options
// set / unset blackbox as the default shell

#include "BB.H"

enum { A_DEL, A_DW, A_SZ };

int write_key(int action, HKEY root, const TCHAR *ckey, const TCHAR *cval, const TCHAR *cdata)
{
	HKEY k;
	DWORD result;
	int r = RegCreateKeyEx(root, ckey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &k, &result);
	if (ERROR_SUCCESS == r)
	{
		if (A_DEL==action)
			r = RegDeleteValue(k, cval);
		else
			if (A_DW==action)
				r = RegSetValueEx(k, cval, 0, REG_DWORD, (LPBYTE)&cdata, sizeof(DWORD));
			else
				if (A_SZ==action)
					r = RegSetValueEx(k, cval, 0, REG_SZ, (LPBYTE)cdata, (DWORD)_tcsnlen(cdata,0xFFFFFFFEUL)+sizeof TCHAR);

		RegCloseKey(k);
	}
	return r == ERROR_SUCCESS;
}                

bool write_ini (TCHAR *boot)
{
	TCHAR szWinDir[MAX_PATH];
	GetWindowsDirectory(szWinDir, MAX_PATH);
	_tcsncat_s(szWinDir, MAX_PATH, _T("\\SYSTEM.INI"), _TRUNCATE);
	return WritePrivateProfileString(_T("Boot"), _T("Shell"), boot, szWinDir) != FALSE;
}

TCHAR inimapstr       [] = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\system.ini\\boot");
TCHAR sys_bootOption  [] = _T("SYS:Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
TCHAR usr_bootOption  [] = _T("USR:Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
TCHAR logonstr        [] = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
TCHAR explorer_option [] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer");
TCHAR szExplorer      [] = _T("explorer.exe");

bool install_message(const TCHAR *msg, bool succ)
{
	if (succ)
		BBMessageBox(MB_OK, NLS2(_T("$BBInstall_Success$"),
								 _T("%s has been set as shell.\nReboot or logoff to apply.")), msg);
	else
		BBMessageBox(MB_OK, NLS2(_T("$BBInstall_Failed$"),
								 _T("Error: Could not set %s as shell.")), msg);
	return succ;
}

//====================
bool install_nt(
    TCHAR *bootOption,
    TCHAR *defaultShell,
    TCHAR *userShell,
    bool setDesktopProcess
    )
{
	// First we open the boot options registry key...
	// ...and set the "Shell" registry value that defines if the shell setting should
	// be read from HKLM (for all users) or from HKCU (for the current user), see above...

	if (0==write_key(A_SZ,
					 HKEY_LOCAL_MACHINE, inimapstr, _T("Shell"), bootOption
					 ))
		goto fail;

	// Next we set the HKLM shell registry key (this needs to be done at all times!)
	if (0==write_key(A_SZ,
					 HKEY_LOCAL_MACHINE, logonstr, _T("Shell"), defaultShell
					 ))
		goto fail;

	// If installing for the current user only, we also need to set the
	// HKCU shell registry key, else delete_it ...

	if (0==write_key(userShell ? A_SZ : A_DEL,
					 HKEY_CURRENT_USER, logonstr, _T("Shell"), userShell
					 ))
		if (userShell)
			goto fail;

	// ...as well as the DesktopProcess registry key that allows Explorer
	// to be used as file manager while using an alternative shell...

	if (0==write_key(A_DW,
					 HKEY_CURRENT_USER, explorer_option, _T("DesktopProcess"),
					 setDesktopProcess ? (TCHAR*)1 : (TCHAR*)0
					 ))
		goto fail;

	if (0==write_key(A_SZ,
					 HKEY_CURRENT_USER, explorer_option, _T("BrowseNewProcess"),
					 setDesktopProcess ? _T("yes") : _T("no")
					 ))
		goto fail;

	return true;
fail:
	return false;
}

//====================
bool installBlackbox()
{
	if(IDYES!=BBMessageBox(MB_YESNO, NLS2(_T("$BBInstall_Query$"),
										  _T("Do you want to install Blackbox as your default shell?"))))
		return false;

	TCHAR szBlackbox[MAX_PATH];
	GetModuleFileName(NULL, szBlackbox, MAX_PATH);
	bool result = false;

	if (GetShortPathName(szBlackbox, szBlackbox, MAX_PATH))
	{
		//strcat(szBlackbox, _T(" -startup"));
		if (usingNT)
		{
			int answer = BBMessageBox(MB_YESNOCANCEL, NLS2(_T("$BBInstall_QueryAllOrCurrent$"),
														   _T("Do you want to install Blackbox as the default shell for All Users?")
														   _T("\n(Selecting \"No\" will install for the Current User only).")
														   ));

			if(IDCANCEL==answer)
				return false;

			if(IDYES==answer)
			{
				// If installing for all users the HKLM shell setting should point to Blackbox...
				result = install_nt(sys_bootOption, szBlackbox, NULL, false);
			}

			if(IDNO==answer)
			{
				// If installing only for the current user the HKLM shell setting should point to Explorer...
				// If installing for the current user only, we also need to set the HKCU shell registry key...

				// ...as well as the DesktopProcess registry key that allows Explorer
				// to be used as file manager while using an alternative shell...

				result = install_nt(usr_bootOption, szExplorer, szBlackbox, true);
			}
		}
		else // Windows9x/ME
		{
			result = write_ini(szBlackbox);
		}
	}

	return install_message(_T("Blackbox"), result);
}

//====================
bool uninstallBlackbox()
{
	if (IDYES != BBMessageBox(MB_YESNO, NLS2(_T("$BBInstall_QueryUninstall$"),
											 _T("Do you want to reset explorer as your default shell?"))))
		return false;

	bool result;

	if (usingNT)
	{
		result = install_nt(sys_bootOption, szExplorer, NULL, false);
	}
	else // Windows9x/ME
	{
		result = write_ini(szExplorer);
	}

	return install_message(_T("Explorer"), result);
}

//====================
