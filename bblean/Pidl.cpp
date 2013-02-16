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

// deal with the Shellfolder ITEMIDLISTs

#ifndef __BBAPI_H_
#include "BB.H"
#endif

#include "Pidl.h"

#define NO_INTSHCUT_GUIDS
#define NO_SHDOCVW_GUIDS
#include <shlobj.h>
#include <shellapi.h>
#include <Shlwapi.h>

//////////////////////////////////////////////////////////////////////
int GetIDListSize(const _ITEMIDLIST * pidl)
{
	int cb; int c = 0;
	if (pidl)
	{
		while (0!=(cb=pidl->mkid.cb))
		{
			c += cb;
			pidl = (const _ITEMIDLIST *)((BYTE*)pidl + cb);
		}
		c += sizeof(short);
	}
	return c;
}

//////////////////////////////////////////////////////////////////////
_ITEMIDLIST * duplicateIDlist(const _ITEMIDLIST * pidl)
{
	_ITEMIDLIST *pidlNew; int cb;
	if (NULL==pidl) return NULL;
	pidlNew = (_ITEMIDLIST*)m_alloc(cb = GetIDListSize(pidl));
	memcpy(pidlNew, pidl, cb);
	return pidlNew;
}

//////////////////////////////////////////////////////////////////////
_ITEMIDLIST * joinIDlists(const _ITEMIDLIST * pidlA, const _ITEMIDLIST * pidlB)
{
	_ITEMIDLIST *pidl; int cbA, cbB;

	cbA = GetIDListSize(pidlA);
	if (cbA) cbA -= sizeof(short);
	cbB = GetIDListSize(pidlB);

	pidl = (_ITEMIDLIST*)m_alloc(cbA + cbB);
	if (pidl)
	{
		memcpy(pidl, pidlA, cbA);
		memcpy((BYTE*)pidl + cbA, pidlB, cbB);
	}
	return pidl;
}

//////////////////////////////////////////////////////////////////////
void SHMalloc_Free(void * pidl)
{
	IMalloc *pMalloc;
	if (pidl && SUCCEEDED(SHGetMalloc(&pMalloc)))
	{
		pMalloc->Free(pidl);
		pMalloc->Release();
	}
}

//////////////////////////////////////////////////////////////////////

BOOL sh_get_displayname(LPSHELLFOLDER piFolder, const _ITEMIDLIST * pidl,
						 DWORD dwFlags, LPTSTR pszName)
{
	STRRET str;
	BOOL fDesktop = FALSE;
	BOOL fSuccess = TRUE;
	//
	// Check to see if a parent folder was specified.  If not, get a pointer
	// to the desktop folder.
	//
	if (NULL == piFolder)
	{
		HRESULT hr = SHGetDesktopFolder(&piFolder);

		if (FAILED(hr))
			return (FALSE);

		fDesktop = TRUE;
	}
	//
	// Get the display name from the folder.  Then do any conversions necessary
	// depending on the type of string returned.
	//
	if (NOERROR == piFolder->GetDisplayNameOf(pidl, dwFlags, &str))
	{
#if 1
		StrRetToBuf(&str, pidl, pszName, MAX_PATH);
#else
		switch (str.uType)
		{
		case STRRET_WSTR:
			wcscpy(pszName, str.pOleStr);
			SHMalloc_Free(str.pOleStr);
			//dbg_printf(_T("WSTR: %s"), pszName);
			break;

		case STRRET_OFFSET:
			_tcscpy(pszName, (LPTSTR)pidl + str.uOffset);
			//dbg_printf(_T("OFFS: %s"), pszName);
			break;

		case STRRET_CSTR:
			MultiByteToWideChar(CP_ACP, 0, str.cStr, -1, pszName, MAX_PATH);
			//dbg_printf(_T("CSTR: %s"), pszName);
			break;

		default:
			fSuccess = FALSE;
			break;
		}
#endif
	}
	else
	{
		fSuccess = FALSE;
	}

	if (fDesktop)
		piFolder->Release();

	return (fSuccess);
}

//////////////////////////////////////////////////////////////////////
// Function:    sh_get_folder_interface
// Purpose:     get the IShellfolder interface from PIDL
//////////////////////////////////////////////////////////////////////

IShellFolder* sh_get_folder_interface(const _ITEMIDLIST * pIDFolder)
{
	IShellFolder* pShellFolder = NULL;
	IShellFolder* pThisFolder  = NULL;
	HRESULT hr;

	hr = SHGetDesktopFolder(&pShellFolder);
	if (NOERROR != hr)
		return NULL;

	if (NextID(pIDFolder) == pIDFolder)
		return pShellFolder;

	hr = pShellFolder->BindToObject(
		pIDFolder, NULL, IID_IShellFolder, (LPVOID*)&pThisFolder);

	pShellFolder->Release();

	if (NOERROR != hr)
		return NULL;

	return pThisFolder;
}

//===========================================================================
// support for DragSource / DropTarget / ContextMenu

bool sh_get_uiobject(
	const _ITEMIDLIST * pidl,
	_ITEMIDLIST ** ppidlPath,
	_ITEMIDLIST ** ppidlItem,
	struct IShellFolder **ppsfFolder,
	const struct _GUID riid,
	void **pObject
	)
{
	*ppidlItem  = NULL;
	*ppsfFolder = NULL;
	*pObject    = NULL;

	_ITEMIDLIST * p = *ppidlPath = duplicateIDlist(pidl);
	if (NULL == p) return false;

	//get last shit.emid - the file
	while (NextID(p)->mkid.cb) p = NextID(p);
	*ppidlItem = duplicateIDlist(p);
	p->mkid.cb=0;

	if (NULL == (*ppsfFolder = sh_get_folder_interface(*ppidlPath)))
		return false;

	//fake ITEMIDLIST - array for 1 file object
	HRESULT hr = (*ppsfFolder)->GetUIObjectOf((HWND)NULL,
											  1, (const _ITEMIDLIST **)ppidlItem, riid, NULL, pObject);

	return SUCCEEDED(hr) && NULL != *pObject;
}

//////////////////////////////////////////////////////////////////////
// Function:    sh_getpidl
// Purpose:     get the PIDL from a local folderitem by path-name
//              using the supplied folder interface or the desktop interface
//////////////////////////////////////////////////////////////////////

_ITEMIDLIST * sh_getpidl (IShellFolder *pSF, const TCHAR *path)
{
	HRESULT hr; WCHAR wpath[MAX_PATH]; ULONG chEaten;
	_ITEMIDLIST * pIDFolder;

	if (NULL == pSF)
	{
		hr = SHGetDesktopFolder(&pSF);
		if (NOERROR!=hr)
			return NULL;
	}

	TCHAR temp[MAX_PATH];
	replace_slashes(temp, MAX_PATH, path);

#ifdef UNICODE
	wcsncpy_s(wpath, MAX_PATH, temp, _TRUNCATE);
#else
	MultiByteToWideChar(CP_ACP, 0, temp, -1, wpath, MAX_PATH);
#endif
	hr = pSF->ParseDisplayName(
		NULL, NULL, wpath, &chEaten, &pIDFolder, NULL);

	pSF->Release();
	if (NOERROR!=hr)
		return NULL;

	_ITEMIDLIST * pID = duplicateIDlist(pIDFolder);
	SHMalloc_Free(pIDFolder);

	return pID;
}

//////////////////////////////////////////////////////////////////////
// Function:    sh_getfolderpath
// Purpose:     replacement for SHGetFolderPath, get path from CSIDL_
//////////////////////////////////////////////////////////////////////

bool sh_getfolderpath(LPTSTR szPath, size_t szPath_size, UINT csidl)
{
	LPITEMIDLIST item;
	TCHAR temp[MAX_PATH];
	if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &item)))
	{
		BOOL result = SHGetPathFromIDList(item, temp);
		_tcsncpy_s(szPath, szPath_size, temp, _TRUNCATE);
		SHMalloc_Free(item);
		return FALSE != result;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// Function:    sh_getdisplayname
// Purpose:
//////////////////////////////////////////////////////////////////////

void sh_getdisplayname (const _ITEMIDLIST * pID, TCHAR *buffer, size_t buffer_size)
{
	SHFILEINFO shinfo;
	shinfo.szDisplayName[0] = 0;
	SHGetFileInfo((LPCTSTR)pID, 0, &shinfo, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_DISPLAYNAME);
	_tcsncpy_s(buffer, buffer_size, shinfo.szDisplayName, _TRUNCATE);
}

//////////////////////////////////////////////////////////////////////
// Function:    sh_geticon
// Purpose:
//////////////////////////////////////////////////////////////////////
/*
HICON sh_geticon (const _ITEMIDLIST * pID, int iconsize)
{
	HIMAGELIST sysimgl;
	SHFILEINFO shinfo;
	UINT cbfileinfo;
	if (iconsize > 24)
		cbfileinfo = SHGFI_PIDL | SHGFI_SYSICONINDEX;
	else
		cbfileinfo = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;

	sysimgl = (HIMAGELIST)SHGetFileInfo((LPCTSTR)pID, 0, &shinfo, sizeof(SHFILEINFO), cbfileinfo);
	if (sysimgl)
		return ImageList_ExtractIcon(NULL, sysimgl, shinfo.iIcon);

	return NULL;
}
*/
//////////////////////////////////////////////////////////////////////
// Function:    get_csidl
// Purpose:     for paths like 'APPDATA\myprog' - get the CSIDL index
//              of the special folder in front of a path and store the
//              rest of the path, if any, to the TCHAR**
//////////////////////////////////////////////////////////////////////

#define CSIDL_BLACKBOX  0x0032
#define CSIDL_CURTHEME  0x0033
#define LAST_CSIDL      0x0034
#define NO_CSIDL        -1

int get_csidl(const TCHAR **pPath)
{
	static const TCHAR idl[] = {
	  _T("DESKTOP")                   // 0x0000 <desktop>
	_T("\0INTERNET")                  // 0x0001 Internet Explorer (icon on desktop)
	_T("\0PROGRAMS")                  // 0x0002 Start Menu\Programs
	_T("\0CONTROLS")                  // 0x0003 My Computer\Control Panel
	_T("\0PRINTERS")                  // 0x0004 My Computer\Printers
	_T("\0PERSONAL")                  // 0x0005 My Documents
	_T("\0FAVORITES")                 // 0x0006 <user name>\Favorites
	_T("\0STARTUP")                   // 0x0007 Start Menu\Programs\Startup
	_T("\0RECENT")                    // 0x0008 <user name>\Recent
	_T("\0SENDTO")                    // 0x0009 <user name>\SendTo
	_T("\0BITBUCKET")                 // 0x000a <desktop>\Recycle Bin
	_T("\0STARTMENU")                 // 0x000b <user name>\Start Menu
	_T("\0")                          // 0x000c
	_T("\0")                          // 0x000d
	_T("\0")                          // 0x000e
	_T("\0")                          // 0x000f
	_T("\0DESKTOPDIRECTORY")          // 0x0010 <user name>\Desktop
	_T("\0DRIVES")                    // 0x0011 My Computer
	_T("\0NETWORK")                   // 0x0012 Network Neighborhood
	_T("\0NETHOOD")                   // 0x0013 <user name>\nethood
	_T("\0FONTS")                     // 0x0014 windows\fonts
	_T("\0TEMPLATES")                 // 0x0015
	_T("\0COMMON_STARTMENU")          // 0x0016 All Users\Start Menu
	_T("\0COMMON_PROGRAMS")           // 0X0017 All Users\Programs
	_T("\0COMMON_STARTUP")            // 0x0018 All Users\Startup
	_T("\0COMMON_DESKTOPDIRECTORY")   // 0x0019 All Users\Desktop
	_T("\0APPDATA")                   // 0x001a <user name>\Application Data
	_T("\0PRINTHOOD")                 // 0x001b <user name>\PrintHood
	_T("\0LOCAL_APPDATA")             // 0x001c <user name>\Local Settings\Applicaiton Data (non roaming)
	_T("\0ALTSTARTUP")                // 0x001d non localized startup
	_T("\0COMMON_ALTSTARTUP")         // 0x001e non localized common startup
	_T("\0COMMON_FAVORITES")          // 0x001f
	_T("\0INTERNET_CACHE")            // 0x0020
	_T("\0COOKIES")                   // 0x0021
	_T("\0HISTORY")                   // 0x0022
	_T("\0COMMON_APPDATA")            // 0x0023 All Users\Application Data
	_T("\0WINDOWS")                   // 0x0024 GetWindowsDirectory()
	_T("\0SYSTEM")                    // 0x0025 GetSystemDirectory()
	_T("\0PROGRAM_FILES")             // 0x0026 C:\Program Files
	_T("\0MYPICTURES")                // 0x0027 C:\Program Files\My Pictures
	_T("\0PROFILE")                   // 0x0028 USERPROFILE
	_T("\0SYSTEMX86")                 // 0x0029 x86 system directory on RISC
	_T("\0PROGRAM_FILESX86")          // 0x002a x86 C:\Program Files on RISC
	_T("\0PROGRAM_FILES_COMMON")      // 0x002b C:\Program Files\Common
	_T("\0PROGRAM_FILES_COMMONX86")   // 0x002c x86 Program Files\Common on RISC
	_T("\0COMMON_TEMPLATES")          // 0x002d All Users\Templates
	_T("\0COMMON_DOCUMENTS")          // 0x002e All Users\Documents
	_T("\0COMMON_ADMINTOOLS")         // 0x002f All Users\Start Menu\Programs\Administrative Tools
	_T("\0ADMINTOOLS")                // 0x0030 <user name>\Start Menu\Programs\Administrative Tools
	_T("\0CONNECTIONS")               // 0x0031 Network and Dial-up Connections

	// --- other ---
	_T("\0BLACKBOX")                  // 0x0032 BLACKBOX HOME
	_T("\0CURRENTTHEME")              // 0x0033

	// --- xoblite aliases ---
	_T("\0PROGRAMFILES")              // 0x0034
	_T("\0USERAPPDATA")               // 0x0035
	_T("\0COMMONSTARTMENU")           // 0x0036
	_T("\0.")
	};

	static const TCHAR xob_ids[] = {
		CSIDL_PROGRAM_FILES,
		CSIDL_APPDATA,
		CSIDL_COMMON_STARTMENU
	};

	TCHAR buffer[MAX_PATH];
	const TCHAR *psub, *s, *path = s = *pPath;

	if (NULL == path) return NO_CSIDL;
	size_t l = _tcslen(path);

	// let's see, if there is a subfolder
	if (NULL != (psub = _tcschr(path, _T('\\'))) || NULL != (psub = _tcschr(path, _T('/'))))
		l = psub - path;

	if (0 == l) return NO_CSIDL;

	// check XOBLITE/Litestep style special folders like $Blackbox$
	if (l>2 && path[0]==_T('$') && path[l-1]==_T('$'))
	{
		_tcsncpy_s(buffer, MAX_PATH, path, _TRUNCATE); StrRemoveEncap(buffer, MAX_PATH);
		s = buffer; _tcsupr_s(buffer, MAX_PATH); // we need upper letter case
		l-=2;
	}
	// search the list above
	const TCHAR *cp = idl; int id = CSIDL_DESKTOP;
	do {
		size_t k = _tcslen(cp);
		if (k == l && 0 == _tmemcmp(s, cp, k))
		{
			*pPath = psub;  // pointer to the subfolder, or NULL
			if (id < LAST_CSIDL) return id;
			return xob_ids[id - LAST_CSIDL];
		}
		id ++, cp += k+1;
	} while (_T('.') != *cp);

	return NO_CSIDL; // not found
}

//////////////////////////////////////////////////////////////////////
// Function: get_folder_pidl
// Purpose:  get the PIDL from path-string, with parsing for special
//           folders
//////////////////////////////////////////////////////////////////////

_ITEMIDLIST * get_folder_pidl (const TCHAR *rawpath)
{
	if (NULL==rawpath) return NULL;

	TCHAR path [MAX_PATH];
	TCHAR temp [MAX_PATH];
	unquote(temp, MAX_PATH, rawpath);

	if (false == is_relative_path(temp))
	{
		if (is_alpha(temp[0]) && temp[1] == _T(':') && temp[2] == 0)
			temp[2] = _T('\\'), temp[3] = 0;
		return sh_getpidl(NULL, temp);
	}

	const TCHAR *p = temp;
	int id = get_csidl(&p);
	if (NO_CSIDL == id || CSIDL_BLACKBOX == id || CSIDL_CURTHEME == id)
	{
		GetBlackboxPath(path, MAX_PATH);
		if (NO_CSIDL != id) path[_tcslen(path)-1] = 0;
		if (p) _tcsncat_s(path, MAX_PATH, p, _TRUNCATE);
		return sh_getpidl(NULL, path);
	}

	// special folders, like CONTROLS
	_ITEMIDLIST *pID1, *pID;

	if (NOERROR != SHGetSpecialFolderLocation(NULL, id, &pID1))
		return sh_getpidl(NULL, temp);

	if (NULL == p)
	{
		pID = duplicateIDlist(pID1);
	}
	else
	{
		pID = NULL;
		// a subdirectory is specified, like APPDATA\microsoft
		// so get its local pidl and append it
		IShellFolder*  pThisFolder = sh_get_folder_interface(pID1);
		if (pThisFolder)
		{
			_ITEMIDLIST * pID2 = sh_getpidl(pThisFolder, p+1);
			if (pID2)
			{
				pID = joinIDlists(pID1, pID2);
				m_free(pID2);
			}
		}
	}

	SHMalloc_Free(pID1);
	return pID;
}

//////////////////////////////////////////////////////////////////////
// Function: get_folder_pidl_list
// Purpose:  get a list of pidls, specified by string, separated by '|'
//////////////////////////////////////////////////////////////////////

struct pidl_node *get_folder_pidl_list (const TCHAR *paths)
{
	struct pidl_node *p = NULL, **pp = &p;
	TCHAR buffer[MAX_PATH];
	while (*NextToken(buffer, MAX_PATH, &paths, _T("|")))
	{
		LPITEMIDLIST pID = get_folder_pidl(buffer);
		//dbg_printf(_T("pidl %x <%s>"), pID, buffer);
		if (pID) pp = &(*pp = (struct pidl_node*)new_node(pID))->next;
	}
	return p;
}

//================================================
void delete_pidl_list (struct pidl_node **ppList)
{
	struct pidl_node *p;
	dolist (p, *ppList) m_free(p->v);
	freeall(ppList);
}

//================================================
struct pidl_node *copy_pidl_list(const struct pidl_node *old_pidl_list)
{
	const struct pidl_node *p;
	struct pidl_node *new_pidl_list = NULL;
	dolist (p, old_pidl_list)
		append_node(&new_pidl_list, new_node(duplicateIDlist(p->v)));
	return new_pidl_list;
}

//////////////////////////////////////////////////////////////////////
// Function:    replace_shellfolders
// Purpose:     parse for special folders and convert back to a path
//              string
//////////////////////////////////////////////////////////////////////

TCHAR *replace_shellfolders(TCHAR *buffer, size_t buffer_size, const TCHAR *path, bool search_path)
{
	TCHAR temp [MAX_PATH];
	const TCHAR *p = unquote(temp, MAX_PATH, path);

	if (false == is_relative_path(temp))
	{
		_tcsncpy_s(buffer,buffer_size,temp,_TRUNCATE);
		return buffer;
	}

	int id = get_csidl(&p);

	if (CSIDL_BLACKBOX == id || CSIDL_CURTHEME == id)
	{
		GetBlackboxPath(buffer, MAX_PATH);
		buffer[_tcslen(buffer)-1] = 0;
	}
	else
	if (NO_CSIDL == id)
	{
		if (search_path)
		{
			if (SearchPath(NULL, temp, _T(".exe"), (DWORD)buffer_size, buffer, NULL))
				return buffer;
			else
			{
				_tcsncpy_s(buffer, buffer_size, temp, _TRUNCATE);
				return buffer;
			}
		}
		GetBlackboxPath(buffer, MAX_PATH);
	}
	else
	{
		// special folders, like CONTROLS
		_ITEMIDLIST * pID;
		HRESULT hr = SHGetSpecialFolderLocation(NULL, id, &pID);
		if (NOERROR != hr)
		{
			_tcsncpy_s(buffer, buffer_size, temp, _TRUNCATE);
			return buffer;
		}

		// returns also things like "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
		// (unlike SHGetPathFromIDList)
		BOOL result = sh_get_displayname(NULL, pID, SHGDN_FORPARSING, buffer);
		SHMalloc_Free(pID);
		if (FALSE == result)
		{
			_tcsncpy_s(buffer, buffer_size, temp, _TRUNCATE);
			return buffer;
		}
	}

	if (p) _tcsncat_s(buffer, MAX_PATH, p, _TRUNCATE);
	return buffer;
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
