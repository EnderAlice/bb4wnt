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
#include "../Pidl.h"
#include "../Desk.h"
#include "../Workspaces.h"
#include "../Toolbar.h"
#include "MenuMaker.h"
#include "Menu.h"

#include <ctype.h>
#include <shlobj.h>
#include <shellapi.h>
#ifdef __GNUC__
#include <shlwapi.h>
#endif

//===========================================================================
struct MenuInfo MenuInfo;

UINT bb_notify;

void MenuMaker_Init(void)
{
	register_menuclass();
	// well, that's stupid, but the first change registration
	// messes with the menu focus, so here the bbhwnd is
	// registered as a default one.
	_ITEMIDLIST *pidl = get_folder_pidl(_T("BLACKBOX"));
	bb_notify = add_change_notify_entry(BBhwnd, pidl);
	m_free(pidl);
}

//====================

void MenuMaker_Clear(void)
{
	if (MenuInfo.hTitleFont) DeleteObject(MenuInfo.hTitleFont);
	if (MenuInfo.hFrameFont) DeleteObject(MenuInfo.hFrameFont);
	MenuInfo.hTitleFont = MenuInfo.hFrameFont = NULL;
	if (FolderItem::m_byBulletBmp){
		for (int i = 0; i < FolderItem::m_nBulletBmpSize; i++){
			m_free(FolderItem::m_byBulletBmp[i]);
		}
		m_free(FolderItem::m_byBulletBmp);
	}
}

void MenuMaker_Exit(void)
{
	MenuMaker_Clear();
	un_register_menuclass();
	remove_change_notify_entry(bb_notify);
}

void Menu_All_Update(int special)
{
	switch (special)
	{
	case MENU_IS_CONFIG:
		if (MenuExists(_T("IDRoot_configuration")))
			ShowMenu(MakeConfigMenu(false));
		break;

	case MENU_IS_TASKS:
		if (MenuExists(_T("IDRoot_workspace")))
			ShowMenu(MakeDesktopMenu(false));
		else
			if (MenuExists(_T("IDRoot_icons")))
				ShowMenu(MakeIconsMenu(false));
			else
				if (MenuExists(_T("IDRoot_processes")))
					ShowMenu(MakeProcessesMenu(false));
		break;
	}
}

//===========================================================================
int get_bulletstyle (const TCHAR *tmp)
{
	static const TCHAR *bullet_strings[] = {
		_T("diamond")  ,
		_T("triangle") ,
		_T("square")   ,
		_T("circle")   ,
		_T("bitmap")   ,
		_T("empty")    ,
		NULL
		};

	static const BYTE bullet_styles[] = {
		BS_DIAMOND   ,
		BS_TRIANGLE  ,
		BS_SQUARE    ,
		BS_CIRCLE    ,
		BS_BMP       ,
		BS_EMPTY     ,
	};

	intptr_t i = get_string_index(tmp, bullet_strings);
	if (-1 != i) return bullet_styles[i];
	return BS_TRIANGLE;
}

//===========================================================================
int LoadBulletBmp(BYTE** *byBulletBmp){
	TCHAR path[MAX_PATH];
	HBITMAP hBmp = (HBITMAP)LoadImage(
		NULL, make_bb_path(path, MAX_PATH, _T("bullet.bmp")),
		IMAGE_BITMAP,
		0,
		0,
		LR_LOADFROMFILE
		);
	// get bitmap size
	BITMAP bm;
	GetObject(hBmp, sizeof(BITMAP), &bm);
	int nSize = (int)imax(bm.bmWidth - 2, (bm.bmHeight - 3)/2);
	if (hBmp == NULL) return 0;
	HDC hDC = CreateCompatibleDC(NULL);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);
	*byBulletBmp = (BYTE**)m_alloc(sizeof(BYTE*)*nSize);
	for (int i = 0; i < nSize; i++){
		(*byBulletBmp)[i] = (BYTE*)m_alloc(sizeof(BYTE)*nSize*2);
		int x = i+1;
		for (int j = 0; j < nSize*2; j++){
			int y = j+1 + j/nSize;
			(*byBulletBmp)[i][j] = 0xff - greyvalue(GetPixel(hDC, x, y));
		}
	}
	SelectObject(hDC, hOldBmp);
	DeleteDC(hDC);
	DeleteObject(hBmp);
	return nSize;
}

//===========================================================================
void MenuMaker_Configure()
{
	// clear fonts
	MenuMaker_Clear();

	MenuInfo.move_cursor = Settings_usedefCursor
		? LoadCursor(NULL, IDC_SIZEALL)
			: LoadCursor(hMainInstance, (TCHAR*)IDC_MOVEMENU);

	StyleItem *MTitle = &mStyle.MenuTitle;
	StyleItem *MFrame = &mStyle.MenuFrame;
	StyleItem *MHilite = &mStyle.MenuHilite;

	if (false == (MTitle->validated & VALID_FONT))
	{
		_tcsncpy_s(MTitle->Font, 128, MFrame->Font, _TRUNCATE);
		if (0 == (MTitle->validated & VALID_FONTHEIGHT))
			MTitle->FontHeight = MFrame->FontHeight;
		if (0 == (MTitle->validated & VALID_FONTWEIGHT))
			MTitle->FontWeight = MFrame->FontWeight;
	}

	// create fonts
	MenuInfo.hTitleFont = CreateStyleFont(MTitle);
	MenuInfo.hFrameFont = CreateStyleFont(MFrame);

	// set bullet pos & style
	const TCHAR *tmp = Settings_menuBulletPosition_cfg;
	if (0 == _tcsicmp(tmp,_T("default"))) tmp = mStyle.menuBulletPosition;

	FolderItem::m_nBulletStyle = get_bulletstyle(mStyle.menuBullet);
	FolderItem::m_nBulletPosition = stristr(tmp, _T("right")) ? DT_RIGHT : DT_LEFT;
	// load bitmap
	FolderItem::m_nBulletBmpSize = LoadBulletBmp(&FolderItem::m_byBulletBmp);

	// --------------------------------------------------------------
	// pre-calulate metrics:

	if (false == (MTitle->validated & VALID_BORDER))
		MTitle->borderWidth = MFrame->borderWidth;

	if (false == (MTitle->validated & VALID_BORDERCOLOR))
		MTitle->borderColor = MFrame->borderColor;

	MenuInfo.nFrameMargin = MFrame->borderWidth;
	if (MFrame->validated & VALID_MARGIN)
		MenuInfo.nFrameMargin += MFrame->marginWidth;
	else
		if (BEVEL_SUNKEN == MFrame->bevelstyle || BEVEL2 == MFrame->bevelposition)
			MenuInfo.nFrameMargin += MFrame->bevelposition;
		else
			if (MHilite->borderWidth)
				MenuInfo.nFrameMargin += 1;

	// --------------------------------------
	// calculate item height, indent

	int ffh = get_fontheight(MenuInfo.hFrameFont);

	int itemHeight;
	if (MHilite->validated & VALID_MARGIN)
		itemHeight = ffh + MHilite->marginWidth;
	else
		if (MFrame->validated & VALID_MARGIN)
			itemHeight = ffh + 1 + 2*(MHilite->borderWidth + (MFrame->marginWidth ? 1 : 0));
		else
			if (Settings_newMetrics)
				itemHeight = (int)imax(10, ffh) + 2 + (mStyle.bevelWidth+1)/2;
			else
				itemHeight = MFrame->FontHeight + (mStyle.bevelWidth+1)/2;

	MenuInfo.nItemHeight = itemHeight;
	MenuInfo.nItemIndent = itemHeight;

	int iconMargin;
	if (MHilite->validated & VALID_MARGIN)
		iconMargin = MHilite->marginWidth;
	else
		iconMargin = 0;

	// --------------------------------------
	// calculate item with icon height, indent

	MenuInfo.nIconItemHeight  = (int)imax(Settings_menuIconSize + iconMargin, itemHeight);
	MenuInfo.nIconItemIndent  = (int)imax(Settings_menuIconSize + iconMargin, itemHeight);

	// --------------------------------------
	// calculate title height, indent, margin

	int tfh = get_fontheight(MenuInfo.hTitleFont);

	int titleHeight;
	if (MTitle->validated & VALID_MARGIN)
		titleHeight = tfh + 2*MTitle->marginWidth;
	else
		if (Settings_newMetrics)
			titleHeight = (int)imax(10, tfh) + 2 + 2*mStyle.bevelWidth;
		else
			titleHeight = MTitle->FontHeight + 2*mStyle.bevelWidth;

	int titleMargin = (titleHeight - tfh + 1) / 2;

	MenuInfo.nTitleHeight = titleHeight + MTitle->borderWidth;
	MenuInfo.nTitleIndent = (int)imax(titleMargin, 3 + MTitle->bevelposition);
	MenuInfo.nTitlePrAdjust = 0;

	if (MTitle->parentRelative)
	{
		MenuInfo.nTitlePrAdjust = (titleMargin / 4 + (int)iminmax(titleMargin, 1, 2));
		MenuInfo.nTitleHeight -= titleMargin - MenuInfo.nTitlePrAdjust;
	}

	// --------------------------------------
	// setup a StyleItem for the scroll rectangle

	MenuInfo.Scroller = MTitle->parentRelative ? *MHilite : *MTitle;
	if (MenuInfo.Scroller.parentRelative && 0 == MenuInfo.Scroller.borderWidth)
	{
		if (MFrame->borderWidth)
			MenuInfo.Scroller.borderColor = MFrame->borderColor;
		else
			MenuInfo.Scroller.borderColor = MFrame->TextColor;

		MenuInfo.Scroller.borderWidth = 1;
	}

	if (false != (MenuInfo.Scroller.bordered = 0 != MenuInfo.Scroller.borderWidth))
	{
		if (MFrame->borderWidth)
			MenuInfo.Scroller.borderWidth = MFrame->borderWidth;
		else
			MenuInfo.Scroller.borderWidth = 1;
	}

	MenuInfo.nScrollerSideOffset = (int)imax(0, MFrame->borderWidth - MenuInfo.Scroller.borderWidth);
	MenuInfo.nScrollerSize = (int)imax(10, MenuInfo.nItemIndent+MFrame->borderWidth);

	MenuInfo.nScrollerTopOffset =
		MTitle->parentRelative
			? 0
				: MenuInfo.nFrameMargin - (int)imax(0, MenuInfo.Scroller.borderWidth - MTitle->borderWidth);

	// --------------------------------------
	// Transparency
	Settings_menuAlpha = Settings_menuAlphaEnabled ? Settings_menuAlphaValue : 255;

	// from where on does it need a scroll button:
	MenuInfo.MaxMenuHeight = GetSystemMetrics(SM_CYSCREEN) * Settings_menuMaxHeightRatio / 100;

	MenuInfo.MaxMenuWidth = Settings_menusBroamMode
		? (int)iminmax(Settings_menuMaxWidth*2, 320, 640)
			: Settings_menuMaxWidth;

	MenuInfo.nAvgFontWidth = 0;
}

//===========================================================================
bool get_string_within (TCHAR *dest, TCHAR **p_src, const TCHAR *delim, bool right)
{
	TCHAR *a, *b; *dest = 0;
	if (NULL == (a = _tcschr(*p_src, delim[0])))
		return false;
	if (NULL == (b = right ? _tcsrchr(++a, delim[1]) : _tcschr(++a, delim[1])))
		return false;
	extract_string(dest, a, b-a);
	*p_src = b+1;
	return true;
}

//===========================================================================
// separate the command from the path spec. in a string like:
//  _T("c:\bblean\backgrounds >> @BBCore.rootCommand bsetroot -full ")%1_T("")

TCHAR * get_special_command(TCHAR *out, size_t count, TCHAR *in)
{
    TCHAR *a, *b;
    a = _tcsstr(in, _T(">>"));
    if (NULL == a) return NULL;
    b = a + 1;
    while (a > in && _T(' ') == a[-1]) --a;
    *a = 0;
    while (_T(' ') == *++b);
    _tcsncpy_s(out, count, b, _TRUNCATE); b = _tcsstr(out, _T("%1"));
    if (b) b[1] = _T('s');
    return out;
}

//===========================================================================

//===========================================================================
const TCHAR default_root_menu[] =
    _T("[begin]")
_T("\0")  _T("[path](Programs){PROGRAMS}")
_T("\0")  _T("[path](Desktop){DESKTOP}")
_T("\0")  _T("[submenu](Blackbox)")
_T("\0")    _T("[config](Configuration)")
_T("\0")    _T("[stylesmenu](Styles){styles}")
_T("\0")    _T("[submenu](Edit)")
_T("\0")      _T("[editstyle](style)")
_T("\0")      _T("[editmenu](menu.rc)")
_T("\0")      _T("[editplugins](plugins.rc)")
_T("\0")      _T("[editblackbox](blackbox.rc)")
_T("\0")      _T("[editextensions](extensions.rc)")
_T("\0")      _T("[end]")
_T("\0")    _T("[about](About)")
_T("\0")    _T("[reconfig](Reconfigure)")
_T("\0")    _T("[restart](Restart)")
_T("\0")    _T("[exit](Quit)")
_T("\0")    _T("[end]")
_T("\0")  _T("[submenu](Goodbye)")
_T("\0")    _T("[logoff](Log Off)")
_T("\0")    _T("[reboot](Reboot)")
_T("\0")    _T("[shutdown](Shutdown)")
_T("\0")    _T("[end]")
_T("\0")_T("[end]")
_T("\0")
;

//===========================================================================
const TCHAR *menu_cmds[] =
{
	_T("insertpath")        ,
	_T("stylesdir")         ,
	_T("include")           ,
	// ------------------
	_T("begin")             ,
	_T("end")               ,
	_T("submenu")           ,
	// ------------------
	_T("nop")               ,
	_T("separator")         ,
	_T("path")              ,
	_T("stylesmenu")        ,
	_T("volume")            ,
	// ------------------
	_T("workspaces")        ,
	_T("tasks")             ,
	_T("config")            ,
	// ------------------
	_T("exec")              ,
	NULL
	};

enum
{
	e_insertpath        ,
	e_stylesdir         ,
	e_include           ,
	// ------------------
	e_begin             ,
	e_end               ,
	e_submenu           ,
	// ------------------
	e_nop               ,
	e_sep               ,
	e_path              ,
	e_stylesmenu        ,
	e_volume            ,
	// ------------------
	e_workspaces        ,
	e_tasks             ,
	e_config            ,
	// ------------------
	e_exec              ,
	// ------------------
	e_no_end            ,
	e_other
};

#define MENU_INCLUDE_MAXLEVEL 16

//===========================================================================

Menu* ParseMenu(FILE **fp, int *fc, const TCHAR *path, const TCHAR *title, const TCHAR *IDString, bool popup)
{
	TCHAR buffer[4096], menucommand[80];
	TCHAR label[MAX_PATH], data[2*MAX_PATH], icon[2*MAX_PATH];

	Menu *pMenu = NULL;
	MenuItem *Inserted_Items = NULL;
	int inserted = 0, f; TCHAR *s, *cp;
	intptr_t cmd_id;
	for(;;)
	{
		// read the line
		if (NULL == path)
		{
			_tcsncpy_s(buffer, 4096, *(const TCHAR**)fp, _TRUNCATE); *(const TCHAR**)fp += 1 + _tcslen(buffer);
		}
		else
			if (false == ReadNextCommand(fp[*fc], buffer, 4096))
			{
				if (*fc) // continue from included file
				{
					FileClose(fp[(*fc)--]);
					continue;
				}
				cmd_id = e_no_end;
				goto skip;
			}

		// replace %USER% etc.
		if (_tcschr(buffer, _T('%'))) ReplaceEnvVars(buffer, 4096);

		//dbg_printf(_T("Menu %08x line:%s"), pMenu, buffer);
		cp = buffer;

		// get the command
		if (false == get_string_within(menucommand, &cp, _T("[]"), false))
			continue;

		// search the command
		cmd_id = get_string_index(menucommand, menu_cmds);
		if (-1 == cmd_id) cmd_id = e_other;

		get_string_within(label, &cp, _T("()"), false);
		get_string_within(data,  &cp, _T("{}"), true);
		get_string_within(icon,  &cp, _T("<>"), false);

	skip:
		if (NULL == pMenu && cmd_id != e_begin) // first item must be begin
		{
			pMenu = MakeNamedMenu(
				title ? title : NLS0(_T("missing [begin]")), IDString, popup);
		}

		// insert collected items from [insertpath]/[stylesdir] now?
		if (inserted && cmd_id >= e_begin)
		{
			pMenu->add_folder_contents(Inserted_Items, inserted > 1);
			Inserted_Items = NULL;
			inserted = 0;
		}

		switch (cmd_id)
		{
			// If the line contains [begin] we create a title item...
			// If no menu title has been defined, display Blackbox version...
		case e_begin:
			if (0==label[0]) _tcsncpy_s(label, MAX_PATH, GetBBVersion(), _TRUNCATE);

			if (NULL == pMenu)
			{
				pMenu = MakeNamedMenu(label, IDString, popup);
				continue;
			}
			// fall through, [begin] is like [submenu] when within the menu
			//====================
		case e_submenu:
			{
				_sntprintf_s(buffer, 4096, _TRUNCATE, _T("%s_%s"), IDString, label);
				Menu *mSub = ParseMenu(fp, fc, path, data[0]?data:label, buffer, popup);
				if (mSub) MakeSubmenu(pMenu, mSub, label, icon);
				else MakeMenuNOP(pMenu, label);
			}
			continue;

			//====================
		case e_no_end:
			MakeMenuNOP(pMenu, NLS0(_T("missing [end]")));
		case e_end:
			pMenu->m_bIsDropTarg = true;
			return pMenu;

			//====================
		case e_include:
			s = unquote(buffer, 4096, label[0] ? label : data);
			if (is_relative_path(s) && path)
				_tcsncat_s(add_slash(data, 2*MAX_PATH, get_directory(data, 2*MAX_PATH, path)), 2*MAX_PATH, s, _TRUNCATE); s = data;

			if (++*fc < MENU_INCLUDE_MAXLEVEL && NULL != (fp[*fc] = FileOpen(s)))
				continue;
			--*fc;
			MakeMenuNOP(pMenu, NLS0(_T("[include] failed")));
			continue;

			//====================
		case e_nop:
			MakeMenuNOP(pMenu, label);
			continue;

		case e_volume:
			s = unquote(buffer, 4096, data);
			MakeMenuVOL(pMenu, label[0] ? label : _T("volume"), s);
			continue;

			//====================
		case e_sep:
			MakeMenuSEP(pMenu);
			continue;

			//====================
			// a [stylemenu] item is pointing to a dynamic style folder...
		case e_stylesmenu: s = _T("@BBCore.style %s"); goto s_folder;

			// a [path] item is pointing to a dynamic folder...
		case e_path: s = get_special_command(buffer, 4096, data);
		s_folder:
			MakePathMenu(pMenu, label, data, s, icon);
			continue;

			//====================
			// a [styledir] item will insert styles from a folder...
		case e_stylesdir: s = _T("@BBCore.style %s"); goto s_insert;

			//====================
			// a [insertpath] item will insert items from a folder...
		case e_insertpath: s = get_special_command(buffer, 4096, data);
		s_insert:
			{
				struct pidl_node *p, *plist = get_folder_pidl_list(label[0] ? label : data);
				dolist (p, plist) ++inserted, MenuMaker_LoadFolder(&Inserted_Items, p->v, s);
				delete_pidl_list(&plist);
				continue;
			}

			//====================
			// special items...
		case e_workspaces:
			MakeSubmenu(pMenu, MakeDesktopMenu(popup), label[0]?label:NLS0(_T("Workspaces")), icon);
			continue;

		case e_tasks:
			MakeSubmenu(pMenu, MakeIconsMenu(popup), label[0]?label:NLS0(_T("Icons")), icon);
			continue;

		case e_config:
			MakeSubmenu(pMenu, MakeConfigMenu(popup), label[0]?label:NLS0(_T("Configuration")), icon);
			continue;

			//====================
		case e_exec:
			if (_T('@') != data[0]) goto core_broam;
			// data specifies a broadcast message
			MakeMenuItem(pMenu, label, data, false, icon);
			continue;

			//====================
		case e_other:
			f = get_workspace_number(menucommand);
			if (-1 != f) // is this [workspace#]
			{
				s = label; DesktopInfo DI;
				if (0 == *s) s = DI.name, get_desktop_info(&DI, f);
				MakeSubmenu(pMenu, GetTaskFolder(f, popup), s);
				continue;
			}
			goto core_broam;

			//====================
			// everything else is converted to a '@BBCore....' broam
		core_broam:
			{
				_tcslwr_s(menucommand, 80);
				int x = _sntprintf_s(buffer, 4096, _TRUNCATE, _T("@BBCore.%s"), menucommand);
				if (data[0]) buffer[x]=_T(' '), _tcsncpy_s(buffer+x+1, 4096-x-1, data, _TRUNCATE);
				MakeMenuItem(pMenu, label, buffer, false, icon);
				continue;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
TCHAR *IDRoot_String(TCHAR *buffer, size_t buffer_size, const TCHAR *menu_id)
{
	_sntprintf_s(buffer, buffer_size, _TRUNCATE, _T("IDRoot_%s"), menu_id);
	_tcslwr_s(buffer, buffer_size);
	_tmemcpy(buffer, _T("IDRoot_"), 7);
	return buffer;
}

Menu * MakeRootMenu(const TCHAR *menu_id, const TCHAR *path, const TCHAR *default_menu)
{
	FILE *fp[MENU_INCLUDE_MAXLEVEL]; int fc = 0;
	if (NULL == (fp[0] = FileOpen(path)))
	{
		path = NULL;
		if (NULL == (fp[0] = (FILE*)default_menu))
			return NULL;
	}
	TCHAR IDString[MAX_PATH];
	Menu *m = ParseMenu(fp, &fc, path, NULL, IDRoot_String(IDString, MAX_PATH, menu_id), true);
	if (path) while (fc>=0) FileClose(fp[fc--]);
	return m;
}

//////////////////////////////////////////////////////////////////////
// Function:    MenuMaker_LoadFolder
// Purpose:     enumerate items in a folder, sort them, and insert them
//              into a menu
//////////////////////////////////////////////////////////////////////

int MenuMaker_LoadFolder(MenuItem **ppItems, const _ITEMIDLIST * pIDFolder, const TCHAR  *optional_command)
{
	LPMALLOC pMalloc = NULL;
	IShellFolder* pThisFolder;
	LPENUMIDLIST pEnumIDList = NULL;
	_ITEMIDLIST * pID; ULONG nReturned;
	int r = 0;

	// nothing to do on NULL pidl's
	if (NULL==pIDFolder) return r;

	// get a COM interface of the folder
	pThisFolder = sh_get_folder_interface(pIDFolder);
	if (NULL == pThisFolder) return r;

	// get the folders _T("EnumObjects") interface
	pThisFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIDList);
	SHGetMalloc(&pMalloc);

	// ---------------------------------------------
	if (pEnumIDList && pMalloc)
	{
		r = 1;
		while (S_FALSE!=pEnumIDList->Next(1, &pID, &nReturned) && 1==nReturned)
		{
			TCHAR szDispName[MAX_PATH];
			TCHAR szFullName[MAX_PATH];
			ULONG uAttr = SFGAO_FOLDER|SFGAO_LINK;
			szDispName[0] = szFullName[0] = 0;
			pThisFolder->GetAttributesOf(1, (const _ITEMIDLIST **)&pID, &uAttr);
			sh_get_displayname(pThisFolder, pID, SHGDN_NORMAL, szDispName);
			sh_get_displayname(pThisFolder, pID, SHGDN_FORPARSING, szFullName);

			MenuItem* pItem;

			if (uAttr & SFGAO_FOLDER)
			{
				r |= 4;
				pItem = new SpecialFolderItem(
					szDispName,
					NULL,
					(struct pidl_node*)new_node(joinIDlists(pIDFolder, pID)),
					optional_command,
					NULL
					);
			}
			else
			{
				r |= 2;
				if (optional_command)
				{
					// we need a full path here for comparison with the current style
					TCHAR buffer[2*MAX_PATH];
					_sntprintf_s(buffer, 2*MAX_PATH, _TRUNCATE, optional_command, szFullName);

					// cut off .style file-extension
					TCHAR *p = (TCHAR*)get_ext(szDispName);
					if (*p && 0 == _tcsicmp(p, _T(".style"))) *p = 0;

					pItem = new CommandItem(
						buffer,
						szDispName,
						false,
						NULL
						);

					pItem->m_nSortPriority = M_SORT_NAME;
					pItem->m_ItemID |= MENUITEM_ID_STYLE;
				}
				else
				{
					pItem = new CommandItem(
						NULL,
						szDispName,
						false,
						NULL
						);

					if ( TCHAR *p = (TCHAR*)get_ext(szFullName)){
						struct ext_list *el;
						dolist(el, Settings_menuExtStyle){
							if (0 == _tcsicmp(p, el->ext)){
								pItem->m_pSI = (StyleItem*)c_alloc(sizeof(StyleItem));
								memcpy(pItem->m_pSI, &el->si, sizeof(StyleItem));
								if (!(pItem->m_pSI->validated & VALID_SHADOWCOLOR)){
									pItem->m_pSI->validated |= (&mStyle.MenuFrame)->validated & VALID_SHADOWCOLOR;
									pItem->m_pSI->ShadowColor = (&mStyle.MenuFrame)->ShadowColor;
								}
							}
						}
					}

					if (uAttr & SFGAO_LINK) pItem->m_nSortPriority = M_SORT_NAME;
				}
				pItem->m_pidl = joinIDlists(pIDFolder, pID);

			}
			// free the relative pID
			pMalloc->Free(pID);
			// add item to the list
			pItem->next = *ppItems, *ppItems = pItem;
		}
	}
	if (pMalloc) pMalloc->Release();
	if (pEnumIDList) pEnumIDList->Release();
	pThisFolder->Release();
	return r;
}

//===========================================================================
void init_check_optional_command(const TCHAR *cmd, TCHAR *current_optional_command, size_t current_optional_command_size)
{
	const TCHAR *s;
	if (0 == _tcsnicmp(cmd, s = _T("@BBCore.style %s"), 13))
		_sntprintf_s(current_optional_command, current_optional_command_size, _TRUNCATE, s, stylePath());
	else
		if (0 == _tcsnicmp(cmd, s = _T("@BBCore.rootCommand %s"), 20))
			_sntprintf_s(current_optional_command, current_optional_command_size, _TRUNCATE, s, Desk_extended_rootCommand(NULL));
}

//===========================================================================

//===========================================================================
Menu *SingleFolderMenu(const TCHAR* path)
{
	TCHAR buffer[MAX_PATH], command[MAX_PATH],*s;
	_tcsncpy_s(buffer, MAX_PATH, path, _TRUNCATE);
	s = get_special_command(command, MAX_PATH, buffer);

	struct pidl_node* pidl_list = get_folder_pidl_list(buffer);

	TCHAR disp_name[MAX_PATH];
	if (pidl_list) sh_getdisplayname(pidl_list->v, disp_name, MAX_PATH);
	else _tcsncpy_s(disp_name, MAX_PATH, get_file(unquote(disp_name, MAX_PATH, buffer)), _TRUNCATE);

	Menu *m = new SpecialFolder(disp_name, pidl_list, s);
	delete_pidl_list(&pidl_list);

	TCHAR IDString[MAX_PATH];
	m->m_IDString = new_str(IDRoot_String(IDString, MAX_PATH, path));
	return m;
}

//===========================================================================
// for invocation from the keyboard:
// if the menu is present and has focus: hide it and switch to application
// if the menu is present, but has not focus: set focus to the menu
// if the menu is not present: return false -> show the menu.

bool check_menu_toggle(const TCHAR *menu_id, bool kbd_invoked)
{
	Menu *m; TCHAR IDString[MAX_PATH];

	if (menu_id[0])
		m = Menu::FindNamedMenu(IDRoot_String(IDString, MAX_PATH, menu_id));
	else
		m = Menu::last_active_menu_root();

	if (NULL == m || NULL == m->m_hwnd)
		return false;

	if (m->has_focus_in_chain()
		&& (kbd_invoked || window_under_mouse() == m->m_hwnd))
	{
		focus_top_window();
		Menu_All_Hide();
		return true;
	}

	if (kbd_invoked && m->IsPinned())
	{
		m->set_keyboard_active_item();
		ForceForegroundWindow(m->m_hwnd);
		m->set_focus();
		return true;
	}

	return false;
}

//===========================================================================

bool MenuMaker_ShowMenu(int id, LPARAM param)
{
	TCHAR path[MAX_PATH];
	Menu *m = NULL;
	bool from_kbd = false;
	const TCHAR *menu_id = NULL;
	intptr_t n = 0;

	static const TCHAR * menu_string_ids[] =
	{
		_T("root"),
		_T("workspaces"),
		_T("icons"),
		_T("configuration"),
		_T("processes"),
		_T(""),
		NULL
		};

	enum
	{
		e_root,
		e_workspaces,
		e_icons,
		e_configuration,
		e_processes,
		e_lastmenu,
		e_other = -1
	};

	switch (id)
	{
		// -------------------------------
	case BB_MENU_ROOT: // Main menu
		menu_id = menu_string_ids[n = e_root];
		break;

	case BB_MENU_TASKS: // Workspaces menu
		menu_id = menu_string_ids[n = e_workspaces];
		break;

	case BB_MENU_ICONS: // Iconized tasks menu
		menu_id = menu_string_ids[n = e_icons];
		break;

		// -------------------------------
	case BB_MENU_TOOLBAR: // toolbar menu
		Toolbar_ShowMenu(true);
		return true;

	case BB_MENU_CONTEXT:
		m = GetContextMenu((const _ITEMIDLIST *)param);
		break;

	case BB_MENU_PLUGIN:
		m = (Menu*)param;
		break;

		// -------------------------------
	case BB_MENU_BYBROAM_KBD: // _T("BBCore.KeyMenu [param]")
		from_kbd = true;

	case BB_MENU_BYBROAM: // _T("BBCore.ShowMenu [param]")
		menu_id = (const TCHAR *)param;
		n = get_string_index(menu_id, menu_string_ids);
		break;
	}

	if (menu_id)
	{
		if (check_menu_toggle(menu_id, from_kbd))
			return false;

		switch (n)
		{
		case e_root:
		case e_lastmenu:
			m = MakeRootMenu(menu_id, menuPath(), default_root_menu);
			break;

		case e_workspaces:
			m = MakeDesktopMenu(true);
			break;

		case e_icons:
			m = MakeIconsMenu(true);
			break;

		case e_configuration:
			m = MakeConfigMenu(true);
			break;

		case e_processes:
			m = MakeProcessesMenu(true);
			break;

		case e_other:
			if (-1 != (n = get_workspace_number(menu_id)))
				m = GetTaskFolder((int)n, true);
			else
				if (FindConfigFile(path, MAX_PATH, menu_id, NULL))
					m = MakeRootMenu(menu_id, path, NULL);
				else
					m = SingleFolderMenu(menu_id);
			break;
		}
	}

	if (NULL == m) return false;
	Menu_ShowFirst(m, from_kbd);
	return true;
}

//===========================================================================
