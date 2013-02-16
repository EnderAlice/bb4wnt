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
#include "PluginManager.h"
#include <commdlg.h>

#define ST static

struct plugins *bbplugins;
FILETIME PluginManager_FT;
HWND hSlit;

void load_plugin(struct plugins *q);
void load_all_plugins(void);
bool write_plugins(void);
struct plugins *parse_plugin(const TCHAR *path);

//===========================================================================
void PluginManager_Exit()
{
	reverse_list(&bbplugins);
	struct plugins *q;
	dolist(q, bbplugins) q->enabled = false;
	load_all_plugins();
	freeall(&bbplugins);
}

//===========================================================================
// read in the plugins.rc for the first time

void PluginManager_Init()
{
	bbplugins = NULL;
	PluginManager_FT.dwLowDateTime =
		PluginManager_FT.dwHighDateTime = 0;

	const TCHAR *path=plugrcPath();
	get_filetime(path, &PluginManager_FT);

	FILE *fp;
	if (!_tfopen_s(&fp,path,_T("rt, ccs=UTF-8")))
	{
		TCHAR szBuffer[MAX_PATH];
		while (read_next_line(fp, szBuffer, MAX_PATH))
			parse_plugin(szBuffer);
		fclose(fp);
	}
	load_all_plugins();
}

//===========================================================================
// parse a line from plugins,rc and put it into the pluginlist struct

struct plugins *parse_plugin(const TCHAR *rcline)
{
	size_t rcline_l = _tcslen(rcline) + 1;
	struct plugins *q = (struct plugins*)c_alloc(sizeof(*q) + sizeof(TCHAR) * rcline_l);
	if (_T('#') == *rcline || 0 == *rcline)
		q->is_comment = true;
	else
		if (_T('!') == *rcline)
			while (_T(' ')== *++rcline);
		else
			q->enabled = true;

	if (_T('&') == *rcline)
	{
		q->useslit = true;
		while (_T(' ')== *++rcline);
	}

	TCHAR *s = q->fullpath;
	_tcsncpy_s(q->fullpath, rcline_l, rcline, _TRUNCATE);
	if (false == q->is_comment)
	{
		_tcsncpy_s(q->name, 80, get_file(s), _TRUNCATE); // copy name
		*(TCHAR*)get_ext(s) = 0; // strip ".dll"

		// accept BBSlit and BBSlitX
		q->is_slit = 0 == _tcsnicmp(s, _T("BBSlit"), 6) && (s[6] == 0 || s[7] == 0);

		// lookup for duplicates
		int n = 0; size_t l = _tcsnlen(s,80); struct plugins *q2;
		dolist (q2, bbplugins)
		{
			TCHAR *p = q2->name;
			if (0 == _tcsnicmp(p, s, l) && (0 == *(p+=l) || _T('/') == *p))
				if (0 == n++) _tcsncpy_s(p, 80, _T("/1"), _TRUNCATE);
		}
		if (n) _sntprintf_s(s+l, 80-l, _TRUNCATE, _T("/%d"), 1+n);
	}

	//dbg_printf(_T("%d %d %d %s %s"), q->is_comment, q->enabled, q->inslit, q->fullpath, q->name);
	append_node(&bbplugins, q);
	return q;
}

//===========================================================================
// run though plugin list and load/unload changed plugins

void load_all_plugins(void)
{
	struct plugins *q;
	hSlit = NULL;

	// load slit at first place
	dolist(q, bbplugins)
		if (q->is_slit && q->enabled)
		{
			load_plugin(q);
			do hSlit = FindWindowEx(NULL, hSlit, _T("BBSlit"), NULL);
			while (hSlit && GetWindowThreadProcessId(hSlit, NULL) != GetCurrentThreadId());
		}

	// load/unload other plugins
	dolist(q, bbplugins)
		if (false == q->is_comment && false == q->is_slit)
			load_plugin(q);

	// unload slit at last
	dolist(q, bbplugins)
		if (q->is_slit && false == q->enabled)
			load_plugin(q);
}

//===========================================================================
// plugin error message
enum
{
	error_is_built_in       = 1,
	error_dll_not_found     ,
	error_dll_needs_module  ,
	error_does_not_load     ,
	error_missing_entry     ,
	error_not_slitable      ,
	error_fail_to_load      ,
	error_crash_on_load     ,
	error_crash_on_unload
};

void plugin_error(struct plugins *q, int error)
{
	const TCHAR *msg;
	switch (error)
	{
	case error_is_built_in      :
		msg = NLS2(_T("$PluginError_IsBuiltIn$"),
				   _T("Dont load this plugin with bbLean. It is built-in.")
				   ); break;
	case error_dll_not_found    :
		msg = NLS2(_T("$PluginError_NotFound$"),
				   _T("The plugin was not found.")
				   ); break;
	case error_dll_needs_module :
		msg = NLS2(_T("$PluginError_MissingModule$"),
				   _T("This plugin cannot be loaded. Possible reason:")
				   _T("\n- The plugin requires another dll which is not in place.")
				   ); break;
	case error_does_not_load    :
		msg = NLS2(_T("$PluginError_DoesNotLoad$"),
				   _T("This plugin cannot be loaded. Possible reasons:")
				   _T("\n- The plugin is incompatible with the windows version.")
				   _T("\n- The plugin is incompatible with this blackbox version.")
				   ); break;
	case error_missing_entry    :
		msg = NLS2(_T("$PluginError_MissingEntry$"),
				   _T("This plugin misses one of the required functions. Possible reasons:")
				   _T("\n- The plugin is not a plugin made for Blackbox for Windows.")
				   _T("\n- The plugin is incompatible with this blackbox version.")
				   ); break;
	case error_not_slitable     :
		msg = NLS2(_T("$PluginError_NotSlitable$"),
				   _T("This plugin is not designed to be loaded into the slit.")
				   ); break;
	case error_fail_to_load     :
		msg = NLS2(_T("$PluginError_IniFailed$"),
				   _T("This plugin failed to initialize.")
				   ); break;
	case error_crash_on_load    :
		msg = NLS2(_T("$PluginError_CrashedOnLoad$"),
				   _T("This plugin caused a general protection violation on initializing.")
				   _T("\nPlease contact the plugin author.")
				   ); break;
	case error_crash_on_unload  :
		msg = NLS2(_T("$PluginError_CrashedOnShutdown$"),
				   _T("This plugin caused a general protection violation on shutdown.")
				   _T("\nPlease contact the plugin author.")
				   ); break;

	default:
		msg = _T("(Unknown Error)");
	}
	BBMessageBox(
		MB_OK,
		NLS2(_T("$BBPluginError$"), _T("Error: %s\n%s")),
		q->fullpath,
		msg
		);
}

//===========================================================================
// load/unload one plugin, when the state has changed

void load_plugin(struct plugins *q)
{
	int error = 0;
	bool useslit = q->useslit && NULL != hSlit;

	if (q->hmodule)
	{
		if (q->enabled && useslit == q->inslit)
			return;

		//---------------------------------------
		// unload plugin
		TRY
		{
			q->endPlugin(q->hmodule);
		}
		EXCEPT
		{
			error = error_crash_on_unload;
		}
		FreeLibrary(q->hmodule);
		q->hmodule = NULL;
	}

	if (q->enabled)
	{
		//---------------------------------------
		// load plugin

		HINSTANCE hModule = NULL;
		do
		{
			int i;

			if (error) break; // dont load if it just crashed

			//---------------------------------------
			// check for compatibility

			if (0 == _tcsicmp(q->name, _T("BBDDE")))
			{
				error = error_is_built_in;
				break;
			}

			//---------------------------------------
			// load the dll

			hModule = LoadLibrary(q->fullpath);

			if (NULL == hModule)
			{
				TCHAR buffer[MAX_PATH];
				//dbg_printf(_T("errror %d"), GetLastError());
				if (ERROR_DLL_NOT_FOUND == GetLastError())
					if (FileExists(make_bb_path(buffer, MAX_PATH, q->fullpath)))
						error = error_dll_needs_module;
					else
						error = error_dll_not_found;
                else
					error = error_does_not_load;
				break;
			}

			//---------------------------------------
			// grab interface functions
			{
				static const char* function_names[] = {
					"pluginInfo"        ,
					"beginPlugin"       ,
					"endPlugin"         ,
					"beginSlitPlugin"   ,
					"beginPluginEx"
					};

				FARPROC* pp = (FARPROC*)&q->pluginInfo;
				for (i=0; i<5; i++)
					*pp++ = GetProcAddress(hModule, function_names[i]);
			}

			//---------------------------------------
			// check interface presence

			if (NULL == q->endPlugin)
			{
				error = error_missing_entry;
				break;
			}

			if (q->useslit && NULL == q->beginSlitPlugin && NULL == q->beginPluginEx)
			{
				error = error_not_slitable;
				q->useslit = useslit = false;
				write_plugins();
			}

			if (false == useslit && NULL == q->beginPlugin && NULL == q->beginPluginEx)
			{
				error = error_missing_entry;
				break;
			}

			//---------------------------------------
			// inititalize plugin

			TRY
			{
				if (useslit)
					if (q->beginPluginEx)
						i = q->beginPluginEx(hModule, hSlit);
					else
						i = q->beginSlitPlugin(hModule, hSlit);
                else
					if (q->beginPlugin)
						i = q->beginPlugin(hModule);
					else
						i = q->beginPluginEx(hModule, NULL);

				if (0 == i)
					q->hmodule = hModule, q->inslit = useslit;
				else
					if (2 != i)
						error = error_fail_to_load;
			}
			EXCEPT
			{
				error = error_crash_on_load;
			}

		} while (0);

		//---------------------------------------
		// clean up after error

		if (NULL == q->hmodule)
		{
			if (hModule) FreeLibrary(hModule);
			q->enabled = false;
			write_plugins();
		}
	}

	//---------------------------------------
	// display errors

	if (error) plugin_error(q, error);
}

//===========================================================================
// write back the plugin list to "plugins.rc"

bool write_plugins(void)
{
	FILE *fp;
	const TCHAR *path=plugrcPath();
	if (NULL==(fp=create_rcfile(path)))
		return false;

	struct plugins *q;
	dolist(q, bbplugins)
	{
		TCHAR buffer[MAX_PATH], *d = buffer;
		if (false == q->is_comment)
		{
			if (false == q->enabled)
				*d++ = _T('!'), *d++ = _T(' ');
			if (q->useslit)
				*d++ = _T('&');
		}
		_tcsncpy_s(d, MAX_PATH-(d-buffer), q->fullpath, _TRUNCATE);
#if 0
		OutputDebugString(TEXT("Blackbox: "));
		OutputDebugString(buffer);
		OutputDebugString(TEXT("\n"));
#endif
		_ftprintf(fp,_T("%s\n"), buffer);
	}
	fclose(fp);
	get_filetime(path, &PluginManager_FT);
	return true;
}

//===========================================================================

void PluginManager_aboutPlugins()
{
	TCHAR buff[8000]; int x = 0; bool slitted = false;
	do {
		struct plugins *q;
		dolist(q, bbplugins)
		{
			if (q->enabled && slitted == q->inslit)
			{
				LPCTSTR (*pluginInfo)(int) = q->pluginInfo;
				if (slitted) buff[x++] = _T('&');
				if (pluginInfo)
					x+=_sntprintf_s(buff+x, 8000-x, _TRUNCATE, _T("%s %s %s %s (%s)\n"),
								 pluginInfo(PLUGIN_NAME),
								 pluginInfo(PLUGIN_VERSION),
								 NLS2(_T("$BBAboutPlugins_by$"), _T("by")),
								 pluginInfo(PLUGIN_AUTHOR),
								 pluginInfo(PLUGIN_RELEASE)
								 );
				else
					x+=_sntprintf_s(buff+x, 8000-x, _TRUNCATE, _T("%s\n"), q->fullpath);
			}
		}
	} while (false != (slitted = false == slitted));
	BBMessageBox(MB_OK,
				 _T("#bbLean - %s#%s\t\t\t"),
				 NLS2(_T("$BBAboutPlugins_Title$"), _T("About loaded plugins")),
				 x ? buff : NLS1(_T("No plugins loaded."))
				 );
}

//===========================================================================
// OpenFileName Dialog to add plugins

UINT_PTR APIENTRY OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == uiMsg)
	{
		// center 'open file' dialog on screen
		RECT r, w; HWND hwnd = GetParent(hdlg);
		GetWindowRect(hwnd, &r);
		GetMonitorRect(hwnd, &w, GETMON_WORKAREA|GETMON_FROM_WINDOW);
		MoveWindow(hwnd,
				   (w.right+w.left+r.left-r.right) / 2,
				   (w.bottom+w.top+r.top-r.bottom) / 2,
				   r.right - r.left,
				   r.bottom - r.top,
				   TRUE
				   );
	}
	return 0;
}


bool browse_file(const TCHAR *title, const TCHAR *filters, TCHAR *buffer, size_t buffer_size)
{
	OPENFILENAME ofCmdFile;
	memset(&ofCmdFile, 0, sizeof(OPENFILENAME));
	ofCmdFile.lStructSize = sizeof(OPENFILENAME);
	buffer[0] = 0;
	ofCmdFile.lpstrFilter = filters;
	ofCmdFile.nFilterIndex = 1;
	ofCmdFile.hwndOwner = NULL;
	ofCmdFile.lpstrFile = buffer;
	ofCmdFile.nMaxFile = (DWORD)buffer_size;
	//ofCmdFile.lpstrFileTitle = NULL;
	//ofCmdFile.nMaxFileTitle = 0;
	//ofCmdFile.lpstrInitialDir = make_bb_path(pluginPath, _T("plugins"));
	ofCmdFile.lpstrTitle = title;
	ofCmdFile.Flags = OFN_FILEMUSTEXIST|OFN_EXPLORER|OFN_ENABLEHOOK|OFN_ENABLESIZING|OFN_HIDEREADONLY;
	if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		ofCmdFile.Flags ^= (OFN_LONGNAMES|OFN_EXPLORER|OFN_ENABLESIZING);
	}
	ofCmdFile.lpfnHook = OFNHookProc;
	bool ret = false;
/*
	BOOL (WINAPI *pGetOpenFileName)(LPOPENFILENAME lpofn);
	HMODULE hLib;
	if (NULL != (hLib = LoadLibrary(_T("comdlg32.dll")))
		&& NULL != (*(FARPROC*)&pGetOpenFileName = GetProcAddress(hLib, "GetOpenFileNameA")))
	{
		ret = pGetOpenFileName(&ofCmdFile);
		FreeLibrary(hLib);
	}
*/
	ret = GetOpenFileName(&ofCmdFile) != FALSE;
	return ret;
}

//===========================================================================
// handle the "@BBCfg.plugin.xxxxx" bro@ms from the config menu

void PluginManager_handleBroam(const TCHAR *submessage)
{
	TCHAR cmd[80], arg[MAX_PATH], boolarg[40], *tokens[2] = { cmd, arg };
	BBTokenize (submessage, tokens, 2, boolarg, 40);

	static const TCHAR *actions[] = {
		//  0        1        2       3
		_T("remove"), _T("load"), _T("inslit"), _T("add"), NULL
		};

	intptr_t action = get_string_index(cmd, actions);
	if (-1 == action) return;

	struct plugins *q;

	if (3 == action)
	{
		if (0 == *arg
			&& false == browse_file(
				NLS1(_T("Add Plugin")),
				_T("Plugins (*.dll)\0*.dll\0All Files (*.*)\0*.*\0"), arg, MAX_PATH))
			return;

		q = parse_plugin(get_relative_path(arg));
	}
	else
	{
		dolist (q, bbplugins)
			if (0 == _tcsicmp(q->name, arg))
				break;
	}

	if (NULL == q) return;

	if (0 == action) q->enabled = false;
	if (1 == action) set_bool(&q->enabled, boolarg);
	if (2 == action) set_bool(&q->useslit, boolarg);

	load_all_plugins();

	if (0 == action || (3 == action && false == q->enabled))
		remove_item(&bbplugins, q);

	write_plugins();
}

//===========================================================================
