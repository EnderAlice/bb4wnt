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
#include "Desk.h"
#include "PluginManager.h"
#include "Workspaces.h"
#include "Toolbar.h"
#include "Menu/MenuMaker.h"

#define SUB_PLUGIN_LOAD 1
#define SUB_PLUGIN_SLIT 2

//===========================================================================
struct int_item { int *v; short minval, maxval, offval; }  int_items[] = {
	{ &Settings_menuMousewheelfac          ,  1,  10,   -2  },
	{ &Settings_menuPopupDelay             ,  0, 400,   -2  },
	{ &Settings_menuMaxWidth               ,100, 600,   -2  },
	{ &Settings_menuAlphaValue             ,  0, 255,  255  },
	{ (int*)&Settings_desktopMargin.left   , -1, 10000, -1  },
	{ (int*)&Settings_desktopMargin.right  , -1, 10000, -1  },
	{ (int*)&Settings_desktopMargin.top    , -1, 10000, -1  },
	{ (int*)&Settings_desktopMargin.bottom , -1, 10000, -1  },
	{ &Settings_edgeSnapThreshold          ,  0, 20,     0  },
	{ &Settings_edgeSnapPadding            , -4, 100,   -4  },
	{ &Settings_toolbarWidthPercent        , 10, 100,   -2  },
	{ &Settings_toolbarAlphaValue          ,  0, 255,  255  },
	{ NULL, 0, 0, 0}
};

struct int_item *get_int_item(const void *v)
{
	struct int_item *p = int_items;
	do if (p->v == v) return p;
	while ((++p)->v);
	return NULL;
}

bool is_string_item(const void *v,size_t *size)
{
	bool is_str;

	is_str = false;
	if(v == Settings_preferredEditor)
	{
		*size = sizeof(Settings_preferredEditor)/sizeof(TCHAR);
		is_str = true;
	}
	else if(v == Settings_toolbarStrftimeFormat)
	{
		*size = sizeof(Settings_toolbarStrftimeFormat)/sizeof(TCHAR);
		is_str = true;
	}
	return is_str;
}

bool is_fixed_string(const void *v,size_t *size)
{
	bool is_fstr;

	is_fstr = false;
	if(v == Settings_focusModel)
	{
		*size = sizeof(Settings_focusModel)/sizeof(TCHAR);
		is_fstr = true;
	}
	else if(v == Settings_menuBulletPosition_cfg)
	{
		*size = sizeof(Settings_menuBulletPosition_cfg)/sizeof(TCHAR);
		is_fstr = true;
	}
	else if(v == Settings_toolbarPlacement)
	{
		*size = sizeof(Settings_toolbarPlacement)/sizeof(TCHAR);
	}
	return is_fstr;
}

//===========================================================================
Menu * GetPluginMenu(
    const TCHAR *title, TCHAR *menu_id, size_t menu_id_size, bool pop, int mode, struct plugins **qp)
{
	TCHAR *save_id = _tcschr(menu_id, 0);
	Menu *pMenu = MakeNamedMenu(title, menu_id, pop);

	struct plugins *q;
	while (NULL != (q = *qp))
	{
		*qp = q->next;

		if (q->is_comment)
		{
			if (mode == SUB_PLUGIN_SLIT)
				continue;

			TCHAR command[40], label[80], *cp = q->fullpath;
			if (false == get_string_within(command, &cp, _T("[]"), false))
				continue;

			get_string_within(label, &cp, _T("()"), false);
			if (0 == _tcsicmp(command, _T("nop")))
				MakeMenuNOP(pMenu, label);
			else
				if (0 == _tcsicmp(command, _T("submenu")) && *label)
				{
					_sntprintf_s(save_id, menu_id_size-(save_id-menu_id), _TRUNCATE, _T("_%s"), label);
					MakeSubmenu(pMenu, GetPluginMenu(label, menu_id, 80, pop, mode, qp), label);
				}
				else
					if (0 == _tcsicmp(command, _T("end")))
						break;

			continue;
		}

		bool checked; const TCHAR *cmd;
		if (mode == SUB_PLUGIN_LOAD)
		{
			cmd = _T("@BBCfg.plugin.load %s");
			checked = q->enabled;
		}
		else
			if (mode == SUB_PLUGIN_SLIT)
			{
				if (false == q->enabled)
					continue;

				if (NULL == hSlit)
					continue;

				if (NULL == q->beginSlitPlugin && NULL == q->beginPluginEx)
					continue;

				cmd = _T("@BBCfg.plugin.inslit %s");
				checked = q->useslit;
			}
			else
				continue;

		TCHAR buf[80]; _sntprintf_s(buf, 80, _TRUNCATE, cmd, q->name); //, false_true_string(false==checked));
		MakeMenuItem(pMenu, q->name, buf, checked);
	}
	*save_id = 0;
	return pMenu;
}

//===========================================================================

Menu *CfgMenuMaker(const TCHAR *title, const struct cfgmenu *pm, bool pop, TCHAR *menu_id, size_t menu_id_size)
{
	if (SUB_PLUGIN_LOAD == (long)pm || SUB_PLUGIN_SLIT == (long)pm)
	{
		struct plugins *q = bbplugins;
		return GetPluginMenu(title, menu_id, menu_id_size, pop, (long)pm, &q);
	}

	TCHAR *save_id = _tcschr(menu_id, 0);
	Menu *pMenu = MakeNamedMenu(title, menu_id, pop);
	size_t s;
	TCHAR buf[80]; _tcsncpy_s(buf, 80, _T("@BBCfg."), _TRUNCATE);
	while (pm->text)
	{
		const TCHAR *item_text = pm->text;
		if (pm->command)
		{
			const TCHAR *cmd = pm->command;
			if (_T('@') != *cmd) _tcsncpy_s(buf+7, 73, cmd, _TRUNCATE), cmd=buf;
			struct int_item *iip = get_int_item(pm->pvalue);
			if (iip)
			{
				MenuItem *pItem = MakeMenuItemInt(
					pMenu, item_text, cmd, *iip->v, iip->minval, iip->maxval);
				if (-2 != iip->offval)
					MenuItemInt_SetOffValue(
						pItem, iip->offval, 10000 == iip->maxval ? _T("auto") : NULL);
			}
			else
				if (is_string_item(pm->pvalue,&s))
				{
					MakeMenuItemString(pMenu, item_text, cmd, (LPCTSTR)pm->pvalue);
				}
				else
				{
					bool checked = false;
					if (is_fixed_string(pm->pvalue,&s))
						checked = 0==_tcsicmp((const TCHAR *)pm->pvalue, _tcsrchr(cmd, _T(' '))+1);
					else
						if (pm->pvalue)
							checked = *(bool*)pm->pvalue;

					bool disabled = false;
					if (pm->pvalue == &Settings_smartWallpaper)
					{
						if (Settings_desktopHook || false == Settings_background_enabled)
							disabled = true;
					}

					MakeMenuItem(pMenu, item_text, cmd, checked && false == disabled);
					if (disabled) DisableLastItem(pMenu);

				}
		}
		else
		{
			struct cfgmenu* sub = (struct cfgmenu*)pm->pvalue;
			if (sub)
			{
				_sntprintf_s(save_id, menu_id_size-(save_id-menu_id), _TRUNCATE, _T("_%s"), item_text);
				Menu *pSub = CfgMenuMaker(item_text, sub, pop, menu_id, menu_id_size);
				if (pSub)
				{
					MakeSubmenu(pMenu, pSub, item_text);
					if (SUB_PLUGIN_SLIT == (long)sub && NULL == hSlit)
						DisableLastItem(pMenu);
				}
			}
			else
			{
				MakeMenuNOP(pMenu, item_text);
			}
		}
		pm++;
	}
	*save_id = 0;
	return pMenu;
}

//===========================================================================
// FIXME: howto forward-declare static variables?
extern const struct cfgmenu cfg_main[];
extern const struct cfgmenu cfg_sub_plugins[];
extern const struct cfgmenu cfg_sub_menu[];
extern const struct cfgmenu cfg_sub_menubullet[];
extern const struct cfgmenu cfg_sub_graphics[];
extern const struct cfgmenu cfg_sub_misc[];
extern const struct cfgmenu cfg_sub_dm[];
extern const struct cfgmenu cfg_sub_focusmodel[];
extern const struct cfgmenu cfg_sub_snap[];
extern const struct cfgmenu cfg_sub_workspace[];

Menu *MakeConfigMenu(bool popup)
{
	TCHAR menu_id[200];
	_tcsncpy_s(menu_id, 200, _T("IDRoot_configuration"), _TRUNCATE);
	Menu *m = CfgMenuMaker(NLS0(_T("Configuration")), cfg_main, popup, menu_id, 200);
	return m;
}

const struct cfgmenu cfg_main[] = {
	{ NLS0(_T("Plugins")),            NULL, cfg_sub_plugins },
	{ NLS0(_T("Menus")),              NULL, cfg_sub_menu },
	{ NLS0(_T("Graphics")),           NULL, cfg_sub_graphics },
	{ NLS0(_T("Misc.")),              NULL, cfg_sub_misc },
	{ NULL }
};

const struct cfgmenu cfg_sub_plugins[] = {
	{ NLS0(_T("Load/Unload")),        NULL, (void*)SUB_PLUGIN_LOAD },
	{ NLS0(_T("In Slit")),            NULL, (void*)SUB_PLUGIN_SLIT },
	{ NLS0(_T("Add Plugin...")),      _T("plugin.add"), NULL },
	{ NLS0(_T("Edit plugins.rc")),    _T("@BBCore.editPlugins"), NULL },
	{ NLS0(_T("About Plugins")),      _T("@BBCore.aboutPlugins"), NULL },
	{ NULL }
};

const struct cfgmenu cfg_sub_menu[] = {
	{ NLS0(_T("Bullet Position")),    NULL, cfg_sub_menubullet },
	{ NLS0(_T("Maximal Width")),      _T("menu.maxWidth"),        &Settings_menuMaxWidth  },
	{ NLS0(_T("Popup Delay")),        _T("menu.popupDelay"),      &Settings_menuPopupDelay  },
	{ NLS0(_T("Wheel Factor")),       _T("menu.mouseWheelFactor"), &Settings_menuMousewheelfac  },
	{ _T(""), NULL, NULL },
	{ NLS0(_T("Transparency")),       _T("menu.alpha.Enabled"),   &Settings_menuAlphaEnabled  },
	{ NLS0(_T("Alpha Value")),        _T("menu.alpha.Value"),     &Settings_menuAlphaValue  },
	{ _T(""), NULL, NULL },
	{ NLS0(_T("Always On Top")),      _T("menu.onTop"),           &Settings_menusOnTop  },
	{ NLS0(_T("Snap To Edges")),      _T("menu.snapWindow"),      &Settings_menusSnapWindow  },
	{ NLS0(_T("Toggle With Plugins")), _T("menu.pluginToggle"),   &Settings_menuspluginToggle  },
	{ NLS0(_T("Sort By Extension")),  _T("menu.sortbyExtension"), &Settings_menusExtensionSort  },
	{ _T(""), NULL, NULL },
	{ NLS0(_T("Show Bro@ms")),        _T("menu.showBr@ams"),      &Settings_menusBroamMode },
	{ NULL }
};

const struct cfgmenu cfg_sub_menubullet[] = {
	{ NLS0(_T("Default")),            _T("menu.bulletPosition default"), Settings_menuBulletPosition_cfg },
	{ NLS0(_T("Left")),               _T("menu.bulletPosition left"),    Settings_menuBulletPosition_cfg },
	{ NLS0(_T("Right")),              _T("menu.bulletPosition right"),   Settings_menuBulletPosition_cfg },
	{ NULL }
};

const struct cfgmenu cfg_sub_graphics[] = {
	{ NLS0(_T("Enable Background")),  _T("enableBackground"),   &Settings_background_enabled },
	{ NLS0(_T("Smart Wallpaper")),    _T("smartWallpaper"),     &Settings_smartWallpaper  },
	{ _T(""), NULL, NULL },
	{ NLS0(_T("*Nix Bullets")),       _T("bulletUnix"),         &mStyle.bulletUnix  },
	{ NLS0(_T("*Nix Arrows")),        _T("arrowUnix"),          &Settings_arrowUnix  },
	//{ NLS0(_T("*Nix Metrics")),       _T("metricsUnix"),        &Settings_newMetrics  },
	{ _T(""), NULL, NULL },
	{ NLS0(_T("Image Dithering")),    _T("imageDither"),        &Settings_imageDither  },
	{ NLS0(_T("Global Font Override")), _T("globalFonts"),      &Settings_globalFonts  },
	{ NULL }
};

const struct cfgmenu cfg_sub_misc[] = {
	{ NLS0(_T("Desktop Margins")),    NULL, cfg_sub_dm },
	{ NLS0(_T("Focus Model")),        NULL, cfg_sub_focusmodel },
	{ NLS0(_T("Snap")),               NULL, cfg_sub_snap },
	{ NLS0(_T("Workspaces")),         NULL, cfg_sub_workspace },
	{ _T(""), NULL, NULL },
	{ NLS0(_T("Enable Toolbar")),     _T("toolbar.enabled"),      &Settings_toolbarEnabled  },
	{ NLS0(_T("Opaque Window Move")), _T("opaqueMove"),           &Settings_opaqueMove },
	{ NLS0(_T("Blackbox Editor")),    _T("setBBEditor"),          &Settings_preferredEditor },
	{ _T(""), NULL, NULL },
	{ NLS0(_T("Show Appnames")),      _T("@BBCore.showAppnames"), NULL },
	{ NULL }
};

const struct cfgmenu cfg_sub_dm[] = {
	{ NLS0(_T("Top")),                _T("desktop.marginTop"),    &Settings_desktopMargin.top  },
	{ NLS0(_T("Bottom")),             _T("desktop.marginBottom"), &Settings_desktopMargin.bottom  },
	{ NLS0(_T("Left")),               _T("desktop.marginLeft"),   &Settings_desktopMargin.left  },
	{ NLS0(_T("Right")),              _T("desktop.marginRight"),  &Settings_desktopMargin.right  },
	{ NLS0(_T("Full Maximization")),  _T("fullMaximization") ,    &Settings_fullMaximization  },
	{ NULL }
};

const struct cfgmenu cfg_sub_focusmodel[] = {
	{ NLS0(_T("Click To Focus")),     _T("focusModel ClickToFocus"), Settings_focusModel },
	{ NLS0(_T("Sloppy Focus")),       _T("focusModel SloppyFocus"),  Settings_focusModel },
	{ NLS0(_T("Auto Raise")),         _T("focusModel AutoRaise"),    Settings_focusModel },
	{ NULL }
};

const struct cfgmenu cfg_sub_snap[] = {
	{ NLS0(_T("Snap To Plugins")),    _T("snap.plugins"), &Settings_edgeSnapPlugins },
	{ NLS0(_T("Padding")),            _T("snap.padding"),  &Settings_edgeSnapPadding },
	{ NLS0(_T("Threshold")),          _T("snap.threshold"), &Settings_edgeSnapThreshold },
	{ NULL }
};

const struct cfgmenu cfg_sub_workspace[] = {
	{ NLS0(_T("Follow Active Task")), _T("workspaces.followActive"),    &Settings_followActive },
	{ NLS0(_T("Restore To Current")), _T("workspaces.restoreToCurrent"), &Settings_restoreToCurrent },
	{ NLS0(_T("Alternate Method")),   _T("workspaces.altMethod"),       &Settings_altMethod },
	{ NLS0(_T("Follow Moved Task")),   _T("workspaces.followMoved"),       &Settings_followMoved },
	//{ NLS0(_T("Mousewheel Changing")), _T("workspaces.wheeling"),     &Settings_desktopWheel },
	//{ NLS0(_T("XP-Fix (Max'd Windows)")), _T("workspaces.xpfix"),    &Settings_workspacesPCo },
	{ NULL }
};

void RedrawConfigMenu(void)
{
	Menu_All_Update(MENU_IS_CONFIG);
}

//===========================================================================

const struct cfgmenu * find_cfg_item(
    const TCHAR *cmd, const struct cfgmenu *pmenu, const struct cfgmenu **pp_menu)
{
	const struct cfgmenu *p;
	for (p = pmenu; p->text; ++p)
		if (NULL == p->command)
		{
			const struct cfgmenu* psub = (struct cfgmenu*)p->pvalue;
			if ((unsigned long)psub >= 100 && NULL != (psub = find_cfg_item(cmd, psub, pp_menu)))
				return psub;
		}
		else
			if (0==_tcsnicmp(cmd, p->command, _tcslen(p->command)))
			{
				*pp_menu = pmenu;
				return p;
			}
	return NULL;
}

const void *exec_internal_broam(
        const TCHAR *argument, const struct cfgmenu *menu_root,
        const struct cfgmenu **p_menu, const struct cfgmenu**p_item)
{
	const void *v = NULL;
	size_t l;
	*p_item = find_cfg_item(argument, menu_root, p_menu);
	if (NULL == *p_item) return v;

	// scan for a possible argument to the command
	while (!IS_SPC(*argument)) ++argument;
	while (_T(' ') == *argument) ++argument;

	v = (*p_item)->pvalue;
	if (NULL == v) return v;

	// now set the appropriate variable
	if (is_fixed_string(v,&l) || is_string_item(v,&l))
	{
		_tcsncpy_s((TCHAR *)v, l, argument, _TRUNCATE);
	}
	else
		if (get_int_item(v))
		{
			if (*argument) *(int*)v = _ttoi(argument);
		}
		else
		{
			set_bool((bool*)v, argument);
		}

	// write to blackbox.rc or extensions.rc (automatic)
	Settings_WriteRCSetting(v);
	return v;
}

void exec_cfg_command(const TCHAR *argument)
{
    // is it a plugin related command?
    if (0 == _tcsnicmp(argument, _T("plugin."), 7))
    {
        PluginManager_handleBroam(argument+7);
        RedrawConfigMenu();
        return;
    }

    // search the item in above structures
    const struct cfgmenu *p_menu, *p_item;
    const void *v = exec_internal_broam(argument, cfg_main, &p_menu, &p_item);
    if (NULL == v) return;

    // update the menu checkmarks
    RedrawConfigMenu();

    // now take care for some item-specific refreshes
    if (cfg_sub_menu == p_menu || cfg_sub_menubullet == p_menu)
    {
        if (v == &Settings_menusExtensionSort)
            PostMessage(BBhwnd, BB_REDRAWGUI, BBRG_MENU|BBRG_FOLDER, 0);
        else
            PostMessage(BBhwnd, BB_REDRAWGUI, BBRG_MENU, 0);
    }
    else
    if (cfg_sub_graphics == p_menu)
    {
        PostMessage(BBhwnd, BB_RECONFIGURE, 0, 0);
        if (v == &Settings_smartWallpaper)
            Desk_reset_rootCommand();
    }
    else
    if (cfg_sub_dm == p_menu)
    {
        SetDesktopMargin(NULL, BB_DM_REFRESH, 0);
    }
    else
    if (cfg_sub_workspace == p_menu)
    {
        Workspaces_Reconfigure();
    }
    else
    if (v == Settings_focusModel)
    {
        set_focus_model(Settings_focusModel);
    }
    else
    if (v == &Settings_opaqueMove)
    {
        set_opaquemove();
    }
    else
    if (v == &Settings_toolbarEnabled)
    {
        if (Settings_toolbarEnabled)
            beginToolbar(hMainInstance);
        else
            endToolbar(hMainInstance);
    }
}

//===========================================================================

