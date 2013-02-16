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

#ifndef __MENU_H
#define __MENU_H

// for VolumeItem Class
#include "../VolumeControl.h"

// global vars
extern int g_menu_count;
extern int g_menu_item_count;

// utility
HWND window_under_mouse(void);

struct MenuList { struct MenuList *next; class Menu *m; };

//===========================================================================
class Menu  
{
protected:
	int         m_refc;         // menu reference count

	Menu*       m_pParent;      // parent menu, if onscreen
	Menu*       m_pChild;       // child menu, if onscreen
	Menu*       m_pLastChild;   // remember child while in menu update

	MenuItem*   m_pMenuItems;   // items, first is title (always present)
	MenuItem*   m_pParentItem;  // parentmenu's folderitem linked to this
	MenuItem*   m_pActiveItem;  // currently hilited item
	MenuItem**  m_ppAddItem;    // helper while adding items

	int         m_MenuID;       // see below
	TCHAR*       m_IDString;     // unique ID for plugin menus

	int         m_itemcount;    // total items
	int         m_topindex;     // top item index
	int         m_pagesize;     // visible items
	int         m_firstitem_top;    // in pixel

	int         m_scrollpos;    // scroll button location (pixel from trackstart)
	int         m_captureflg;   // when the mouse is captured, see below
	int         m_keyboard_item_pos;    // item hilited by the keyboard
	int         m_active_item_pos;  // temporarily while in update

	bool        m_bOnTop;       // z-order
	bool        m_bPinned;      // pinned
	bool        m_bNoTitle;     // dont draw title
	bool        m_bKBDInvoked;  // was invoked from kbd
	bool        m_bIsDropTarg;  // is registered as DT
	bool        m_bIconized;    // iconized to titlebar

	bool        m_bMouseOver;   // current states:
	bool        m_bHasFocus;
	bool        m_dblClicked;
	bool        m_bMoving;      // moved by the user
	bool        m_bInDrag;      // in drag&drop operation
	bool        m_bPopup;       // for plugin menus, false when updating

	BYTE        m_alpha;        // transparency

	int         m_xpos;         // window position and sizes
	int         m_ypos;
	int         m_width;
	int         m_height;

	HBITMAP     m_hBitMap;      // background bitmap, only while onscreen
	HWND        m_hwnd;         // window handle, only while onscreen

	class CDropTarget *m_droptarget;

	// ----------------------
	// static vars

	static MenuList *g_MenuWindowList;  // all menus with a window
	static MenuList *g_MenuStructList;  // all menus
	static MenuList *g_MenuFocusList;   // list of menus with focus recently

	static int  g_MouseWheelAccu;
	static int  g_DiscardMouseMoves;

	// ----------------------
	// class methods

	Menu(const TCHAR *pszTitle);
	virtual ~Menu();

	int decref(void);
	int incref(void);

	MenuItem *AddMenuItem(MenuItem* m);
	void DeleteMenuItems();

	bool IsPinned() { return m_bPinned; };
	void SetPinned( bool bPinned );

	void LinkToParentItem(MenuItem *Item);
	void Detach(void);

	void Hide(void);
	void HideThis(void);
	void HideChild(void);
	void UnHilite(void);

	void Show(int xpos, int ypos, int flags);

	void SetZPos(void);
	void Validate(void);
	void Paint();
	void MenuTimer(WPARAM);

	bool Handle_Key(UINT msg, WPARAM wParam);
	void Handle_Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void UpdateFolder(void);
	virtual void register_droptarget(bool set);

	// ----------------------
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// ----------------------

	// popup/close delay
	void set_timer(bool set);

	// init / exit
	void make_menu_window(void);
	void destroy_menu_window(void);

	// scrolling
	int get_y_range(int *py0, int *ps);
	void get_vscroller_rect(RECT* rw);
	int calc_topindex (bool adjust);
	void set_vscroller(int ymouse);
	void scroll_assign_items(int n);
	void scroll_menu(int n);

	// keyboard
	void set_keyboard_active_item(void);
	MenuItem * match_next_shortcut(const TCHAR *d);
	void hilite(MenuItem *Item);
	void activate_by_key(MenuItem *Item);
	void hide_on_click(void);

	// retrieve specific things
	Menu *menu_root (void);
	int get_item_index(MenuItem *item);
	int get_active_index (void);
	bool has_in_chain(HWND hwnd);
	bool has_focus_in_chain(void);
	MenuItem * nth_item(int a);

	// mouse
	void mouse_over(bool indrag);
	void mouse_leave(void);
	const _ITEMIDLIST *dragover(POINT* ppt);
	void start_drag(const TCHAR *arg, const _ITEMIDLIST *pidl);
	void set_focus(void);

	// other
	void insert_at_last (void);
	void cons_focus(void);
	void bring_menu_ontop(void);
	void menu_set_pos(HWND after, UINT flags);
	void redraw(void);
	void redraw_structure(void);
	void write_menu_pos(void);

	// add special folder items
	void add_folder_contents(MenuItem *pItems, bool join);

	// set/reset menu title;
	void SetTitle(LPTSTR pszTitle);
	void ResetTitle(void);

	// -------------------------------------
	// overall menu functions
	static Menu *last_active_menu_root(void);
	static void Hide_All_But(Menu *m);
	static Menu *FindNamedMenu(LPCTSTR IDString);

	// -------------------------------------
	// for incremental search
	bool m_bSearch;
	UINT_PTR m_LastKey;
	// -------------------------------------

	// init/exit
	friend void register_menuclass(void);
	friend void un_register_menuclass(void);
	friend void Menu_All_Delete(void);

	// PluginMenu API friends
	friend API_EXPORT(Menu*) MakeMenu(LPCTSTR HeaderText);
	friend API_EXPORT(Menu*) MakeNamedMenu(LPCTSTR HeaderText, LPCTSTR IDString, bool popup);
	friend API_EXPORT(void) ShowMenu(Menu *PluginMenu);
	friend API_EXPORT(bool) MenuExists(const TCHAR* IDString_start);
	friend API_EXPORT(MenuItem*) MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, LPCTSTR Title);
	friend MenuItem* MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, LPCTSTR Title, LPCTSTR Icon);
	friend API_EXPORT(MenuItem*) MakeMenuItem(Menu *PluginMenu, LPCTSTR Title, LPCTSTR Cmd, bool ShowIndicator);
	friend MenuItem* MakeMenuItem(Menu *PluginMenu, LPCTSTR Title, LPCTSTR Cmd, bool ShowIndicator, LPCTSTR Icon);
	friend MenuItem* make_helper_menu(Menu *PluginMenu, LPCTSTR Title, int menuID, MenuItem *Item);
	friend API_EXPORT(MenuItem*) MakeMenuItemInt(Menu *PluginMenu, LPCTSTR Title, LPCTSTR Cmd, int val, int minval, int maxval);
	friend API_EXPORT(MenuItem*) MakeMenuItemString(Menu *PluginMenu, LPCTSTR Title, LPCTSTR Cmd, LPCTSTR init_string);
	friend API_EXPORT(MenuItem*) MakeMenuNOP(Menu *PluginMenu, LPCTSTR Title);
	friend API_EXPORT(MenuItem*) MakeMenuSEP(Menu *PluginMenu);
	friend MenuItem* MakeMenuVOL(Menu *PluginMenu, LPCTSTR Title, LPCTSTR DllName);
	friend API_EXPORT(MenuItem*) MakePathMenu(Menu *ParentMenu, LPCTSTR Title, LPCTSTR path, LPCTSTR Cmd);
	friend MenuItem* MakePathMenu(Menu *ParentMenu, LPCTSTR Title, LPCTSTR path, LPCTSTR Cmd, LPCTSTR Icon);
	friend API_EXPORT(void) DelMenu(Menu *PluginMenu);
	friend API_EXPORT(void) DisableLastItem(Menu *PluginMenu);


	// general
	friend Menu* ParseMenu(FILE **fp, int *fc, const TCHAR *path, const TCHAR *title, const TCHAR *IDString, bool popup);
	friend Menu* SingleFolderMenu(const TCHAR* path);
	friend bool MenuMaker_ShowMenu(int id, LPARAM param);
	friend bool check_menu_toggle(const TCHAR* menu_id, bool kbd_invoked);
	friend Menu *MakeRootMenu(const TCHAR *menu_id, const TCHAR *path, const TCHAR *default_menu);
	friend MenuItem *get_real_item(MenuItem *pItem);

	// other
	friend bool Menu_Activate_Last(void);
	friend void Menu_All_Redraw(int flags);
	friend void Menu_Tab_Next(Menu *start);
	friend void Menu_All_Toggle(bool hide);
	friend void Menu_All_BringOnTop(void);
	friend void Menu_All_Hide(void);
	friend void Menu_All_Update(int special);
	friend void Menu_Fire_Timer(void);
	friend void Menu_ShowFirst(Menu *pMenu, bool from_kbd);
	friend bool IsMenu(HWND);

	// friend classes
	friend class MenuItem;
	friend class TitleItem;
	friend class FolderItem;
	friend class CommandItem;
	friend class CommandItemEx;
	friend class IntegerItem;
	friend class VolumeItem;
	friend class StringItem;
	friend class ContextItem;
	friend class ContextMenu;
	friend class SpecialFolderItem;
	friend class SpecialFolder;
	friend class SeparatorItem;
};

//---------------------------------
#define MENU_POPUP_TIMER        1
#define MENU_TRACKMOUSE_TIMER   2
#define MENU_INTITEM_TIMER      3

//---------------------------------
// values for m_MenuID

#define MENU_ID_NORMAL      0
#define MENU_ID_SF          1
#define MENU_ID_SHCONTEXT   2
#define MENU_ID_STRING      4
#define MENU_ID_INT         8

//---------------------------------
// values for m_captureflg

#define MENU_CAPT_SCROLL    1
#define MENU_CAPT_TWEAKINT  2

// flags for void Menu::Show(int xpos, int ypos, int flags);
#define MENUSHOW_LEFT 0
#define MENUSHOW_CENTER 1
#define MENUSHOW_RIGHT 2
#define MENUSHOW_UPDATE 4

//===========================================================================

//===========================================================================

class MenuItem
{
public:
	MenuItem *next;

	MenuItem(const TCHAR* pszTitle);
	virtual ~MenuItem();

	virtual void Measure(HDC hDC, SIZE *size);
	virtual void Paint(HDC hDC);
	virtual void Invoke(int button);
	virtual void Mouse(HWND hw, UINT nMsg, WPARAM, LPARAM);
	virtual void Key(UINT nMsg, WPARAM wParam);
	virtual void ItemTimer(WPARAM nTimer);
	virtual void ShowSubMenu(); 

	void UnlinkSubmenu(void);
	void LinkSubmenu(Menu *pSubMenu);
	void Active(int bActive);
	void ShowContextMenu(const TCHAR *path, const struct _ITEMIDLIST *pidl);

	void GetItemRect(RECT* r);
	void GetTextRect(RECT* r);
	const TCHAR*  GetDisplayString(void);
	inline bool within(int y) { return y >= m_nTop && y < m_nTop + m_nHeight; }
	MenuItem *Sort(int(__cdecl *cmp_fn)(MenuItem**, MenuItem**));

	// ----------------------
	TCHAR* m_pszTitle;
	TCHAR* m_pszTitle_old;
	int m_nSortPriority;
	int m_ItemID;

	int m_nTop;
	int m_nLeft;
	int m_nWidth;
	int m_nHeight;

	bool m_bActive;
	bool m_isChecked;
	TCHAR m_isNOP;

	Menu* m_pMenu;
	Menu* m_pSubMenu;

	_ITEMIDLIST *m_pidl;
	TCHAR *m_pszCommand;
	HICON* m_phIcon;
	StyleItem *m_pSI;
	HICON* GetIcon(const TCHAR* pszIcon);
	void FreeIcon(HICON *phIcon);

	// ----------------------
	static int center_justify;
	static int left_justify;
};

//---------------------------------
// values/bitflags for m_ItemID

#define MENUITEM_ID_NORMAL  0
#define MENUITEM_ID_FOLDER  1
#define MENUITEM_ID_CI      2
#define MENUITEM_ID_SF      (4 | MENUITEM_ID_FOLDER)
#define MENUITEM_ID_CIInt   (8 | MENUITEM_ID_CI)
#define MENUITEM_ID_CIStr   (16| MENUITEM_ID_CI)
#define MENUITEM_ID_STYLE   32
#define MENUITEM_ID_CONTEXT 64

// values for m_isNOP
#define MI_NOP_TEXT 1
#define MI_NOP_SEP 2
#define MI_NOP_DISABLED 4

// values for m_nSortPriority
#define M_SORT_NORMAL 1
#define M_SORT_NAME   3
#define M_SORT_SEP    4
#define M_SORT_FOLDER 5

// button values for void Invoke(int button);
#define INVOKE_DBL      1
#define INVOKE_LEFT     2
#define INVOKE_RIGHT    4
#define INVOKE_MID      8
#define INVOKE_DRAG    16

// FolderItem bullet style / position
#define BS_EMPTY        0
#define BS_DIAMOND      1
#define BS_SQUARE       2
#define BS_TRIANGLE     3
#define BS_CIRCLE       4
#define BS_BMP          5

#define FOLDER_LEFT     0
#define FOLDER_RIGHT    1

// DrawText flags
#define DT_MENU_STANDARD (DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP | DT_EXPANDTABS)
#define DT_MENU_MEASURE_STANDARD (DT_LEFT | DT_EXPANDTABS | DT_CALCRECT)

//===========================================================================

//===========================================================================

class TitleItem : public MenuItem
{
public:
	TitleItem(const TCHAR* pszTitle) : MenuItem(pszTitle) {}
	void Paint(HDC hDC);
	void Mouse(HWND hw, UINT nMsg, WPARAM wP, LPARAM lP);
};

//===========================================================================

//===========================================================================
// An menuitem that is a pointer to a sub menu, theese folder items
// typically contain a |> icon at their right side.

class FolderItem : public MenuItem
{
public:
	FolderItem(Menu* pSubMenu, const TCHAR* pszTitle, const TCHAR* pszIcon);
	void Paint(HDC hDC);
	void Invoke(int button);
	static int m_nBulletStyle;
	static int m_nBulletPosition;
	static int m_nBulletBmpSize;
	static BYTE **m_byBulletBmp;
};

//===========================================================================

//===========================================================================

class CommandItem : public MenuItem
{
public:
	CommandItem(const TCHAR* pszCommand, const TCHAR* pszTitle, bool isChecked, const TCHAR* pszIcon);
	~CommandItem();
	void Invoke(int button);
private:
	TCHAR *m_pszIcon;
};

//=======================================
class CommandItemEx : public CommandItem
{
	CommandItemEx(const TCHAR *pszCommand, const TCHAR *fmt);
	void next_item (WPARAM wParam);
	friend class IntegerItem;
	friend class StringItem;
};

//=======================================
class IntegerItem : public CommandItemEx
{
public:
	IntegerItem(const TCHAR* pszCommand, intptr_t value, intptr_t minval, intptr_t maxval);

	void Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Invoke(int button);
	void ItemTimer(WPARAM nTimer);
	void Measure(HDC hDC, SIZE *size);
	void Key(UINT nMsg, WPARAM wParam);
	void set_next_value();

	intptr_t m_value;
	intptr_t m_min;
	intptr_t m_max;
	intptr_t m_count;
	int direction;
	int oldsize;
	intptr_t offvalue;
	const TCHAR *offstring;
};

//=======================================
class VolumeItem : public CommandItem, public VolumeControl
{
public:
	VolumeItem(const TCHAR *pszTitle, const TCHAR *pszDllName);
	void Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Measure(HDC hDC, SIZE *size);
	void Paint(HDC hDC);
private:
	int m_nVol;
	bool m_bMute;
};

//===========================================================================

class StringItem : public CommandItemEx
{
public:
	StringItem(const TCHAR* pszCommand, const TCHAR *init_string);
	~StringItem();

	void Paint(HDC hDC);
	void Measure(HDC hDC, SIZE *size);
	void SetPosition(void);
	void Key(UINT nMsg, WPARAM wParam);
	void Invoke(int button);

	static LRESULT CALLBACK EditProc(HWND hText, UINT msg, WPARAM wParam, LPARAM lParam);
	HWND hText;
	WNDPROC wpEditProc;
};

//===========================================================================

//===========================================================================

class SpecialFolderItem : public FolderItem
{
public:
	SpecialFolderItem(LPCTSTR pszTitle, const TCHAR *path, struct pidl_node* pidl_list, const TCHAR  *optional_command, const TCHAR* pszIcon);
	~SpecialFolderItem();
	void ShowSubMenu(); 
	void Invoke(int button);
	friend class SpecialFolder;
	friend void join_folders(SpecialFolderItem *);
	const struct _ITEMIDLIST *check_pidl(void);

private:
	struct pidl_node *m_pidl_list;
	TCHAR *m_pszPath;
	TCHAR *m_pszExtra;
};

//===========================================================================
class SpecialFolder : public Menu
{
public:
	SpecialFolder(const TCHAR *pszTitle, const struct pidl_node *pidl_list, const TCHAR  *optional_command);
	~SpecialFolder();
	void UpdateFolder(void);
	void register_droptarget(bool set);

private:
	struct pidl_node *m_pidl_list;
	UINT m_notify;
	TCHAR *m_pszExtra;
};


//===========================================================================

//===========================================================================

class ContextMenu : public Menu
{
public:
	ContextMenu (LPCTSTR title, class ShellContext* w, HMENU hm, int m);
	~ContextMenu();
private:
	void Copymenu (HMENU hm);
	ShellContext *wc;
	friend class ContextItem;
};

class ContextItem : public FolderItem
{
public:
	ContextItem(Menu *m, LPTSTR pszTitle, int id, DWORD_PTR data, UINT type);
	~ContextItem();
	void Paint(HDC hDC);
	void Measure(HDC hDC, SIZE *size);
	void Invoke(int button);
	void DrawItem(HDC hdc, int w, int h, bool active);
private:
	int   m_id;
	DWORD_PTR m_data;
	UINT  m_type;
	int m_icon_offset;
	HBITMAP m_bmp;
	int m_bmp_width;
};

//===========================================================================

//===========================================================================
class SeparatorItem : public CommandItem
{
public:
	SeparatorItem();
	void Measure(HDC hDC, SIZE *size);
	void Paint(HDC hDC);
};

//===========================================================================

#endif /* __MENU_H */
