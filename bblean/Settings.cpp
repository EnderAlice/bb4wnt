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

#ifndef BBSETTING_STYLEREADER_ONLY

#include "BB.h"
#define BBSETTING
#include "Settings.h"
#include "Menu/MenuMaker.h"

// to be used in multimonitor setups (in the future, maybe ...)
int screenNumber = 0;

#endif

//===========================================================================
// check a font if it is available on the system

int CALLBACK EnumFontFamProc(
    ENUMLOGFONT FAR *lpelf,     // pointer to logical-font data 
    NEWTEXTMETRIC FAR *lpntm,   // pointer to physical-font data 
    int FontType,               // type of font 
    LPARAM lParam               // address of application-defined data  
   )
{
	(*(int*)lParam)++;
	return 0;
}   

int checkfont (TCHAR *face)
{
	int data = 0;
	HDC hdc = CreateCompatibleDC(NULL);
	EnumFontFamilies(hdc, face, (FONTENUMPROC)EnumFontFamProc, (LPARAM)&data);
	DeleteDC(hdc);
	return data;
}

//===========================================================================
/*
    *nix font spec:
    -foundry-family-weight-slant-setwidth-addstyle-pixel-point
    -resx-resy-spacing-width-charset-encoding-
    weight: "-medium-", "-bold-", "-demibold-", "-regular-",
    slant: "-r-", "-i-", "-o-", "-ri-", "-ro-",
*/

int getweight (const TCHAR *p)
{
	static const TCHAR *fontweightstrings[] = {
		_T("thin"), _T("extralight"), _T("light"), _T("normal"),
		_T("medium"), _T("demibold"), _T("bold"), _T("extrabold"),
		_T("heavy"), _T("regular"), _T("semibold"), NULL
		};
	size_t i = get_string_index(p, fontweightstrings) + 1;
	if (i==0 || i==10) i = 4;
	if (i==11) i=6;
	return (int)i*100;
}

void tokenize_string(TCHAR *buffer, size_t buffer_size, TCHAR **pp, const TCHAR *src, int n, const TCHAR *delims)
{
	while (n--)
	{
		NextToken(buffer, buffer_size, &src, delims);
		buffer+=_tcslen(*pp = buffer) + 1;
		++pp;
	}
}

void parse_font(StyleItem *si, const TCHAR *font)
{
	static const TCHAR *scanlist[] =
	{
		_T("lucidatypewriter")  ,
		_T("fixed")             ,
		_T("lucida")            ,
		_T("helvetica")         ,
		_T("calisto mt")        ,
		_T("8x8\\ system\\ font") ,
		_T("8x8 system font") ,
		NULL
		};

	static const TCHAR *replacelist[] =
	{
		_T("edges")             ,
		_T("lucida console")    ,
		_T("lucida sans")       ,
		_T("tahoma")            ,
		_T("verdana")           ,
		_T("edges") ,
		_T("edges") ,
	};

	TCHAR fontstring[256]; TCHAR *p[16]; TCHAR *b;
	if (_T('-') == *font)
	{
		tokenize_string(fontstring, 256, p, font+1, 16, _T("-"));
		intptr_t i = get_string_index(b = p[1], scanlist);
		_tcsncpy_s(si->Font, 128, -1 == i ? b : replacelist[i], _TRUNCATE);

		if (*(b = p[2]))
		{
			si->FontWeight = getweight(b);
			si->validated |= VALID_FONTWEIGHT;
		}

		if (*(b = p[6])>=_T('1') && *b<=_T('9'))
		{
			si->FontHeight = _tstoi(b) * 12 / 10;
			si->validated |= VALID_FONTHEIGHT;
		}
		else
			if (*(b = p[7])>=_T('1') && *b<=_T('9'))
			{
				si->FontHeight = _tstoi(b) * 12 / 100;
				si->validated |= VALID_FONTHEIGHT;
			}
	}
	else
		if (_tcschr(font, _T('/')))
		{
			tokenize_string(fontstring, 256, p, font, 3, _T("/"));
			_tcsncpy_s(si->Font, 128, p[0], _TRUNCATE);
			if (*(b = p[1]))
			{
				si->FontHeight = _tstoi(b);
				si->validated |= VALID_FONTHEIGHT;
			}

			if (*(b = p[2]))
			{
				if (stristr(b, _T("shadow")))
					si->FontShadow = true;
				if (stristr(b, _T("bold")))
				{
					si->FontWeight = FW_BOLD;
					si->validated |= VALID_FONTWEIGHT;
				}
			}

			//dbg_printf(_T("<%s> <%s> <%s>"), p[0], p[1], p[2]);
			//dbg_printf(_T("<%s> %d %d"), si->Font, si->FontHeight, si->FontWeight);
		}
		else
		{
			_tcsncpy_s(si->Font, 128, font, _TRUNCATE);
			_tcsncpy_s(fontstring, 256, font, _TRUNCATE); _tcslwr_s(fontstring, 256); b = fontstring;
			if (_tcsstr(b, _T("lucidasans")))
			{
				_tcsncpy_s(si->Font, 128, _T("gelly"), _TRUNCATE);
			}
		}

	if (0 == checkfont(si->Font))
	{
		LOGFONT logFont;
		SystemParametersInfo(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
		_tcsncpy_s(si->Font, 128, logFont.lfFaceName, _TRUNCATE);
	}
}

//===========================================================================
// API: CreateStyleFont
// Purpose: Create a Font, possible substitutions have been already applied.
//===========================================================================

API_EXPORT(HFONT) CreateStyleFont(StyleItem * si)
{
	return CreateFont(
		si->FontHeight,
		0, 0, 0,
		si->FontWeight,
		false, false, false,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH|FF_DONTCARE,
		si->Font
		);
}

//===========================================================================
enum
{
	C_INT = 1,
	C_BOL,
	C_STR,
	C_COL,
	C_STY,

	C_TEX,
	C_CO1,
	C_CO2,
	C_CO3,
	C_CO4,
	C_CO5,
	C_CO6,

	C_FHEI,
	C_FWEI,
	C_FONT,
	C_JUST,
	C_ALPHA,

	C_MARG,
	C_BOCO,
	C_BOWD,

	C_CO1ST,
	C_CO2ST,
};

struct ShortStyleItem
{
	/* 0.0.80 */
	int bevelstyle;
	int bevelposition;
	int type;
	bool parentRelative;
	bool interlaced;
	/* 0.0.90 */
	COLORREF Color;
	COLORREF ColorTo;
	COLORREF TextColor;
	int FontHeight;
	int FontWeight;
	int Justify;
	int validated;
	TCHAR Font[16];
};

ShortStyleItem DefStyleA =
{
	BEVEL_RAISED,  BEVEL1, B_DIAGONAL, FALSE, FALSE,
	0xEEEEEE, 0xCCCCCC, 0x555555, 12, FW_NORMAL, DT_LEFT, 0, _T("verdana")
	};

ShortStyleItem DefStyleB =
{
	BEVEL_RAISED,  BEVEL1, B_VERTICAL, FALSE, FALSE,
	0xCCCCCC, 0xAAAAAA, 0x333333, 12, FW_NORMAL, DT_CENTER, 0, _T("verdana")
	};

COLORREF DefBorderColor = 0x777777;

//===========================================================================
struct items { void *v; TCHAR * rc_string; void *def; unsigned short id; unsigned flags; };

#define HAS_TEXTURE (VALID_TEXTURE|VALID_COLORFROM|VALID_COLORTO|VALID_BORDER|VALID_BORDERCOLOR|VALID_FROMSPLITTO|VALID_TOSPLITTO)
#define HAS_FONT (VALID_FONT|VALID_FONTHEIGHT|VALID_FONTWEIGHT)
#define DEFAULT_MARGIN (1<<14)
#define DEFAULT_BORDER (1<<15)

struct items StyleItems[] = {
{   &mStyle.borderWidth         , _T("borderWidth:"),           (void*) 1,                      C_INT, 0 },
{   &mStyle.borderColor         , _T("borderColor:"),           (void*) &DefBorderColor,        C_COL, 0 },
{   &mStyle.bevelWidth          , _T("bevelWidth:"),            (void*) 1,                      C_INT, 0 },
{   &mStyle.handleHeight        , _T("handleWidth:"),           (void*) 5,                      C_INT, 0 },
{   &mStyle.rootCommand         , _T("rootCommand:"),           (void*)_T(""),                  C_STR, sizeof(mStyle.rootCommand)/sizeof(TCHAR)},

{   &mStyle.Toolbar             , _T("toolbar"),                (void*) &DefStyleA, C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_MARGIN|HAS_FONT|VALID_JUSTIFY|DEFAULT_BORDER|DEFAULT_MARGIN|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.ToolbarButton       , _T("toolbar.button"),         (void*) &DefStyleB, C_STY,  HAS_TEXTURE|VALID_PICCOLOR|VALID_MARGIN },
{   &mStyle.ToolbarButtonPressed, _T("toolbar.button.pressed"), (void*) &DefStyleA, C_STY,  HAS_TEXTURE|VALID_PICCOLOR },
{   &mStyle.ToolbarLabel        , _T("toolbar.label"),          (void*) &DefStyleB, C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_MARGIN|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.ToolbarWindowLabel  , _T("toolbar.windowLabel"),    (void*) &DefStyleA, C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.ToolbarClock        , _T("toolbar.clock"),          (void*) &DefStyleB, C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },

{   &mStyle.MenuTitle           , _T("menu.title"),             (void*) &DefStyleB, C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_MARGIN|HAS_FONT|VALID_JUSTIFY|DEFAULT_MARGIN|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.MenuFrame           , _T("menu.frame"),             (void*) &DefStyleA, C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_PICCOLOR|VALID_MARGIN|HAS_FONT|VALID_JUSTIFY|DEFAULT_BORDER|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.MenuFrameExt        , _T("menu.frame.ext.*"),       (void*) &mStyle.MenuFrame, C_STY,  VALID_TEXTCOLOR|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.MenuHilite          , _T("menu.hilite"),            (void*) &DefStyleB, C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_PICCOLOR|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
// menu.item.marginWidth:
{   &mStyle.MenuHilite          , _T("menu.item"),              (void*) &DefStyleB, C_STY,  VALID_MARGIN },

{   &mStyle.menuBullet          , _T("menu.bullet:"),           (void*) _T("triangle"), C_STR, sizeof(mStyle.menuBullet)/sizeof(TCHAR)  },
{   &mStyle.menuBulletPosition  , _T("menu.bullet.position:"),  (void*) _T("right"),  C_STR, sizeof(mStyle.menuBulletPosition)/sizeof(TCHAR)  },

{   &mStyle.MenuFrame.disabledColor , _T("menu.frame.disableColor:"), (void*) &mStyle.MenuFrame.TextColor, C_COL, 0 },

#ifndef BBSETTING_NOWINDOW
// window.font:
{   &mStyle.windowLabelFocus    , _T("window"),                 (void*) &mStyle.Toolbar,                C_STY,  HAS_FONT|VALID_JUSTIFY },

{   &mStyle.windowTitleFocus    , _T("window.title.focus"),     (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowLabelFocus    , _T("window.label.focus"),     (void*) &mStyle.ToolbarWindowLabel,     C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|HAS_FONT|VALID_JUSTIFY|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.windowHandleFocus   , _T("window.handle.focus"),    (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowGripFocus     , _T("window.grip.focus"),      (void*) &mStyle.ToolbarWindowLabel,     C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowButtonFocus   , _T("window.button.focus"),    (void*) &mStyle.ToolbarButton,          C_STY,  HAS_TEXTURE|VALID_PICCOLOR },
{   &mStyle.windowButtonPressed , _T("window.button.pressed"),  (void*) &mStyle.ToolbarButtonPressed,   C_STY,  HAS_TEXTURE|VALID_PICCOLOR },

{   &mStyle.windowTitleUnfocus  , _T("window.title.unfocus"),   (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER  },
{   &mStyle.windowLabelUnfocus  , _T("window.label.unfocus"),   (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.windowHandleUnfocus , _T("window.handle.unfocus"),  (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowGripUnfocus   , _T("window.grip.unfocus"),    (void*) &mStyle.ToolbarLabel,           C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowButtonUnfocus , _T("window.button.unfocus"),  (void*) &mStyle.ToolbarButton,          C_STY,  HAS_TEXTURE|VALID_PICCOLOR },

// new bb4nix 070 style props
{   &mStyle.handleHeight            , _T("window.handleHeight:")                , (void*) &mStyle.handleHeight, C_INT, 0 },
{   &mStyle.frameWidth              , _T("window.frame.borderWidth:")           , (void*) &mStyle.borderWidth, C_INT, 0 },
{   &mStyle.windowFrameFocusColor   , _T("window.frame.focus.borderColor:")     , (void*) &mStyle.borderColor, C_COL, 0 },
{   &mStyle.windowFrameUnfocusColor , _T("window.frame.unfocus.borderColor:")   , (void*) &mStyle.borderColor, C_COL, 0 },

// bb4nix 0.65 style props, ignored in bb4win
//{   &mStyle.frameWidth              , _T("frameWidth:")                   , (void*) &mStyle.borderWidth, C_INT, 0 },
//{   &mStyle.windowFrameFocusColor   , _T("window.frame.focusColor:")      , (void*) &mStyle.borderColor, C_COL, 0 },
//{   &mStyle.windowFrameUnfocusColor , _T("window.frame.unfocusColor:")    , (void*) &mStyle.borderColor, C_COL, 0 },

// window margins
{   &mStyle.windowTitleFocus    , _T("window.title"),           (void*) &mStyle.Toolbar,                C_STY,  VALID_MARGIN|DEFAULT_MARGIN },
{   &mStyle.windowLabelFocus    , _T("window.label"),           (void*) &mStyle.ToolbarLabel,           C_STY,  VALID_MARGIN },
{   &mStyle.windowButtonFocus   , _T("window.button"),          (void*) &mStyle.ToolbarButton,          C_STY,  VALID_MARGIN },
#endif

{   &mStyle.MenuSeparator       , _T("menu.separator"),         (void*) &DefStyleB,                     C_STY,  HAS_TEXTURE|DEFAULT_BORDER|VALID_SHADOWCOLOR },
{   &mStyle.MenuVolume          , _T("menu.volume"),            (void*) &mStyle.MenuHilite,             C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_PICCOLOR|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },

{   &mStyle.windowButtonCloseFocus   , _T("window.button.close.focus"),   (void*) &mStyle.windowButtonFocus,   C_STY, HAS_TEXTURE|VALID_PICCOLOR },
{   &mStyle.windowButtonClosePressed , _T("window.button.close.pressed"), (void*) &mStyle.windowButtonPressed, C_STY, HAS_TEXTURE|VALID_PICCOLOR },
{   &mStyle.windowButtonCloseUnfocus , _T("window.button.close.unfocus"), (void*) &mStyle.windowButtonUnfocus, C_STY, HAS_TEXTURE|VALID_PICCOLOR },

{ NULL, NULL, NULL, 0, 0 }
};

//===========================================================================
// API: GetSettingPtr - retrieve a pointer to a setting var/struct

API_EXPORT(void*) GetSettingPtr(int i)
{
	switch (i) {

	case SN_STYLESTRUCT             : return &mStyle;

	case SN_TOOLBAR                 : return &mStyle.Toolbar                ;
	case SN_TOOLBARBUTTON           : return &mStyle.ToolbarButton          ;
	case SN_TOOLBARBUTTONP          : return &mStyle.ToolbarButtonPressed   ;
	case SN_TOOLBARLABEL            : return &mStyle.ToolbarLabel           ;
	case SN_TOOLBARWINDOWLABEL      : return &mStyle.ToolbarWindowLabel     ;
	case SN_TOOLBARCLOCK            : return &mStyle.ToolbarClock           ;
	case SN_MENUTITLE               : return &mStyle.MenuTitle              ;
	case SN_MENUFRAME               : return &mStyle.MenuFrame              ;
	case SN_MENUHILITE              : return &mStyle.MenuHilite             ;

	case SN_MENUBULLET              : return &mStyle.menuBullet             ;
	case SN_MENUBULLETPOS           : return &mStyle.menuBulletPosition     ;

	case SN_BORDERWIDTH             : return &mStyle.borderWidth            ;
	case SN_BORDERCOLOR             : return &mStyle.borderColor            ;
	case SN_BEVELWIDTH              : return &mStyle.bevelWidth             ;
	case SN_FRAMEWIDTH              : return &mStyle.frameWidth             ;
	case SN_HANDLEHEIGHT            : return &mStyle.handleHeight           ;
	case SN_ROOTCOMMAND             : return &mStyle.rootCommand            ;

	case SN_MENUALPHA               : return &Settings_menuAlpha            ;
	case SN_TOOLBARALPHA            : return &Settings_toolbarAlpha         ;
	case SN_METRICSUNIX             : return &mStyle.metricsUnix            ;
	case SN_BULLETUNIX              : return &mStyle.bulletUnix             ;

	case SN_WINFOCUS_TITLE          : return &mStyle.windowTitleFocus       ;
	case SN_WINFOCUS_LABEL          : return &mStyle.windowLabelFocus       ;
	case SN_WINFOCUS_HANDLE         : return &mStyle.windowHandleFocus      ;
	case SN_WINFOCUS_GRIP           : return &mStyle.windowGripFocus        ;
	case SN_WINFOCUS_BUTTON         : return &mStyle.windowButtonFocus      ;
	case SN_WINFOCUS_BUTTONP        : return &mStyle.windowButtonPressed    ;
	case SN_WINUNFOCUS_TITLE        : return &mStyle.windowTitleUnfocus     ;
	case SN_WINUNFOCUS_LABEL        : return &mStyle.windowLabelUnfocus     ;
	case SN_WINUNFOCUS_HANDLE       : return &mStyle.windowHandleUnfocus    ;
	case SN_WINUNFOCUS_GRIP         : return &mStyle.windowGripUnfocus      ;
	case SN_WINUNFOCUS_BUTTON       : return &mStyle.windowButtonUnfocus    ;

	case SN_WINFOCUS_FRAME_COLOR    : return &mStyle.windowFrameFocusColor  ;
	case SN_WINUNFOCUS_FRAME_COLOR  : return &mStyle.windowFrameUnfocusColor;

	case SN_NEWMETRICS              : return (void*)Settings_newMetrics     ;

	case SN_MENUSEPARATOR           : return &mStyle.MenuSeparator          ;
	case SN_MENUVOLUME              : return &mStyle.MenuVolume             ;

	case SN_WINFOCUS_BUTTONC        : return &mStyle.windowButtonCloseFocus ;
	case SN_WINFOCUS_BUTTONCP       : return &mStyle.windowButtonClosePressed;
	case SN_WINUNFOCUS_BUTTONC      : return &mStyle.windowButtonCloseUnfocus;

	default                         : return NULL;
	}
}

//===========================================================================
int Settings_ItemSize(int i)
{
	switch (i) {

	case SN_STYLESTRUCT             : return sizeof (StyleStruct);

	case SN_TOOLBAR                 :
	case SN_TOOLBARBUTTON           :
	case SN_TOOLBARBUTTONP          :
	case SN_TOOLBARLABEL            :
	case SN_TOOLBARWINDOWLABEL      :
	case SN_TOOLBARCLOCK            :
	case SN_MENUTITLE               :
	case SN_MENUFRAME               :
	case SN_MENUHILITE              : return sizeof (StyleItem);

	case SN_MENUBULLET              :
	case SN_MENUBULLETPOS           : return -1; // string, have to take strlen

	case SN_BORDERWIDTH             : return sizeof (int);
	case SN_BORDERCOLOR             : return sizeof (COLORREF);
	case SN_BEVELWIDTH              : return sizeof (int);
	case SN_FRAMEWIDTH              : return sizeof (int);
	case SN_HANDLEHEIGHT            : return sizeof (int);
	case SN_ROOTCOMMAND             : return -1; // string, have to take strlen

	case SN_MENUALPHA               : return sizeof (int);
	case SN_TOOLBARALPHA            : return sizeof (int);
	case SN_METRICSUNIX             : return sizeof (bool);
	case SN_BULLETUNIX              : return sizeof (bool);

	case SN_WINFOCUS_TITLE          :
	case SN_WINFOCUS_LABEL          :
	case SN_WINFOCUS_HANDLE         :
	case SN_WINFOCUS_GRIP           :
	case SN_WINFOCUS_BUTTON         :
	case SN_WINFOCUS_BUTTONP        :
	case SN_WINUNFOCUS_TITLE        :
	case SN_WINUNFOCUS_LABEL        :
	case SN_WINUNFOCUS_HANDLE       :
	case SN_WINUNFOCUS_GRIP         :
	case SN_WINUNFOCUS_BUTTON       : return sizeof (StyleItem);

	case SN_WINFOCUS_FRAME_COLOR    :
	case SN_WINUNFOCUS_FRAME_COLOR  : return sizeof (COLORREF);

	case SN_MENUSEPARATOR           :
	case SN_MENUVOLUME              : return sizeof (StyleItem);

	case SN_WINFOCUS_BUTTONC        :
	case SN_WINFOCUS_BUTTONCP       :
	case SN_WINUNFOCUS_BUTTONC      : return sizeof (StyleItem);

	default                         : return 0;
	}
}

//===========================================================================

int ParseJustify (const TCHAR *buff)
{
	if (0==_tcsicmp(buff, _T("center")))   return DT_CENTER;
	if (0==_tcsicmp(buff, _T("right")))    return DT_RIGHT;
	return DT_LEFT;
}

const TCHAR *check_global_font(const TCHAR *p, const TCHAR *fullkey)
{
	if (Settings_globalFonts)
	{
		TCHAR globalkey[80];
		_tcsncpy_s(globalkey, 80, _T("blackbox.global."), _TRUNCATE); _tcsncat_s(globalkey, 80, fullkey, _TRUNCATE);
		const TCHAR *p2 = ReadValue(extensionsrcPath(), globalkey, NULL);
		//dbg_printf(_T("<%s> <%s>"), globalkey, p2);
		if (p2 && p2[0]) return p2;
	}
	return p;
}

//===========================================================================

void read_style_item (const TCHAR * style, StyleItem *si, TCHAR *key, int v,  StyleItem *def)
{
	static struct s_prop { TCHAR *k; TCHAR mode; int v; } s_prop[]= {

		// texture type
		{ _T(":"),                  C_TEX   , VALID_TEXTURE  },
		// colors, from, to, text, pics
		{ _T(".color:"),            C_CO1   , VALID_COLORFROM  },
		{ _T(".colorTo:"),          C_CO2   , VALID_COLORTO  },
		{ _T(".textColor:"),        C_CO3   , VALID_TEXTCOLOR },
		{ _T(".picColor:"),         C_CO4   , VALID_PICCOLOR },
		{ _T(".shadowColor:"),      C_CO5   , VALID_SHADOWCOLOR },
		{ _T(".outlineColor:"),     C_CO6   , VALID_OUTLINECOLOR },
		// font settings
		{ _T(".font:"),             C_FONT  , VALID_FONT  },
		{ _T(".fontHeight:"),       C_FHEI  , VALID_FONTHEIGHT  },
		{ _T(".fontWeight:"),       C_FWEI  , VALID_FONTWEIGHT  },
		{ _T(".justify:"),          C_JUST  , VALID_JUSTIFY  },
		// _new in BBNix 0.70
		{ _T(".borderWidth:"),      C_BOWD  , VALID_BORDER  },
		{ _T(".borderColor:"),      C_BOCO  , VALID_BORDERCOLOR  },
		{ _T(".marginWidth:"),      C_MARG  , VALID_MARGIN   },
		// xoblite
		{ _T(".bulletColor:"),      C_CO4   , VALID_PICCOLOR  },
		// OpenBox
		{ _T(".color.splitTo:"),    C_CO1ST , VALID_FROMSPLITTO },
		{ _T(".colorTo.splitTo:"),  C_CO2ST , VALID_TOSPLITTO },

		{ NULL, 0, 0}
	};

	COLORREF cr;
	size_t l = _tcsnlen(key, 79);
	struct s_prop *cp = s_prop;
	TCHAR fullkey[80]; _tmemcpy(fullkey, key, sizeof(TCHAR) * l);

	si->nVersion = 2;
	si->ShadowX = 1;
	si->ShadowY = 1;

	do
	{
		if (cp->v & v)
		{
			_tcsncpy_s(fullkey + l, 80 - l, cp->k, _TRUNCATE);
			const TCHAR *p = ReadValue(style, fullkey, NULL);
			int found = FoundLastValue();
			if ((si->validated & cp->v) && 1 != found)
				continue;

			switch (cp->mode)
			{
				// --- textture ---
			case C_TEX:
				if (p)
				{
					ParseItem(p, si);
					si->bordered = NULL != stristr(p, _T("border"));
				}
				else
					memcpy(si, def, sizeof(ShortStyleItem));
				break;

				// --- colors ---
			case C_CO1:
				cr = ReadColorFromString(p);
				si->Color = ((COLORREF)-1) != cr ? cr : def ->Color;
				break;

			case C_CO2:
				cr = ReadColorFromString(p);
				si->ColorTo = ((COLORREF)-1) != cr ? cr : def ->ColorTo;
				break;

			case C_CO3:
				cr = ReadColorFromString(p);
				si->TextColor = ((COLORREF)-1) != cr ? cr : def ->TextColor;
				break;

			case C_CO4:
				cr = ReadColorFromString(p);
				if (v & VALID_TEXTCOLOR)
					si->foregroundColor = ((COLORREF)-1) != cr ? cr : si->picColor;
				else
					si->picColor = ((COLORREF)-1) != cr ? cr : def->picColor;
				break;

			case C_CO5:
				si->ShadowColor = ReadColorFromString(p);
				break;

			case C_CO6:
				si->OutlineColor = ReadColorFromString(p);
				break;

				// --- Border & margin ---
			case C_BOCO:
				cr = ReadColorFromString(p);
				si->borderColor = ((COLORREF)-1) != cr ? cr : mStyle.borderColor;
				break;

			case C_BOWD:
				if ((v & DEFAULT_BORDER) || si->bordered)
				{
					if (p) si->borderWidth = _tstoi(p);
					else si->borderWidth = mStyle.borderWidth;
				}
				else
				{
					si->borderWidth = 0;
					continue;
				}
				break;

			case C_MARG:
				if (p)
					si->marginWidth = _tstoi(p);
				else
					if (v & DEFAULT_MARGIN)
						si->marginWidth = mStyle.bevelWidth;
					else
						si->marginWidth = 0;
				break;

				// --- Font ---
			case C_FONT:
				p = check_global_font(p, fullkey);
				if (p) parse_font(si, p);
				else _tcsncpy_s(si->Font, 128, def->Font, _TRUNCATE);
				break;

			case C_FHEI:
				p = check_global_font(p, fullkey);
				if (p) si->FontHeight = _tstoi(p);
				else
					if (si->validated & VALID_FONT) si->FontHeight = 12;
					else si->FontHeight = def->FontHeight;
				break;

			case C_FWEI:
				p = check_global_font(p, fullkey);
				if (p) si->FontWeight = getweight(p);
				else
					if (si->validated & VALID_FONT) si->FontWeight = FW_NORMAL;
					else si->FontWeight = def->FontWeight;
				break;

				// --- Alignment ---
			case C_JUST:
				if (p) si->Justify = ParseJustify(p);
				else si->Justify = def->Justify;
				break;

				// --- Split Color ---
			case C_CO1ST:
				si->ColorSplitTo = ReadColorFromString(p);
				break;

			case C_CO2ST:
				si->ColorToSplitTo = ReadColorFromString(p);
				break;
			}

			if (p) si->validated |= cp->v;
		}
	}
	while ((++cp)->k);
}

//===========================================================================
void ReadStyle(const TCHAR *style)
{
	ZeroMemory(&mStyle, (unsigned)&((StyleStruct*)NULL)->bulletUnix);
	mStyle.metricsUnix = true;
	Settings_newMetrics = is_newstyle(style);

	struct items *p = StyleItems;
	do
	{
		void *def = p->def; int n; bool b; COLORREF cr;
		switch (p->id)
		{
		case C_STY:
			if (0 == _tcscmp(p->rc_string, _T("menu.frame.ext.*"))){
				read_extension_style(style, p->rc_string);
				struct ext_list *el;
				dolist(el, Settings_menuExtStyle){
					read_style_item(style, &el->si, el->key, p->flags, (StyleItem*)p->def);
				}
			}
			else{
				read_style_item (style, (StyleItem*)p->v, p->rc_string, p->flags, (StyleItem*)p->def);
			}
			break;

		case C_INT:
			n = HIWORD(def) ? *(int*)def : (int) def;
			*(int*)p->v = ReadInt(style, p->rc_string, n);
			break;

		case C_BOL:
			b = HIWORD(def) ? *(bool*)def : (def != 0);
			*(bool*)p->v = ReadBool(style, p->rc_string, b);
			break;

		case C_ALPHA:
			n = HIWORD(def) ? *(int*)def : (int) def;
			*(BYTE*)p->v = ReadInt(style, p->rc_string, n);
			break;

		case C_COL:
			cr = ReadColorFromString(ReadValue(style, p->rc_string, NULL));
			*(COLORREF*)p->v = ((COLORREF)-1) != cr ? cr : *(COLORREF*)def;
			break;

		case C_STR:
			strcpy_max((TCHAR*)p->v, ReadString(style, p->rc_string, (TCHAR*)def), p->flags);
			break;
		}
	} while ((++p)->v);
}

//===========================================================================
#ifndef BBSETTING_STYLEREADER_ONLY
//===========================================================================
struct rccfg { TCHAR *key; TCHAR mode; void *def; void *ptr; size_t ptr_size; };

struct rccfg ext_rccfg[] = {

	//{ _T("blackbox.appearance.metrics.unix:"),      C_BOL, (void*)true,     &Settings_newMetrics },

	{ _T("blackbox.appearance.bullet.unix:"),       C_BOL, (void*)true,     &mStyle.bulletUnix, sizeof(mStyle.bulletUnix) },
	{ _T("blackbox.appearance.arrow.unix:"),        C_BOL, (void*)false,    &Settings_arrowUnix, sizeof(Settings_arrowUnix) },
	{ _T("blackbox.appearance.cursor.usedefault:"), C_BOL, (void*)false,    &Settings_usedefCursor, sizeof(Settings_usedefCursor) },

	{ _T("blackbox.desktop.marginLeft:"),           C_INT, (void*)-1,       &Settings_desktopMargin.left, sizeof(Settings_desktopMargin.left) },
	{ _T("blackbox.desktop.marginRight:"),          C_INT, (void*)-1,       &Settings_desktopMargin.right, sizeof(Settings_desktopMargin.right) },
	{ _T("blackbox.desktop.marginTop:"),            C_INT, (void*)-1,       &Settings_desktopMargin.top, sizeof(Settings_desktopMargin.top) },
	{ _T("blackbox.desktop.marginBottom:"),         C_INT, (void*)-1,       &Settings_desktopMargin.bottom, sizeof(Settings_desktopMargin.bottom) },

	{ _T("blackbox.snap.toPlugins:"),               C_BOL, (void*)true,     &Settings_edgeSnapPlugins, sizeof(Settings_edgeSnapPlugins) },
	{ _T("blackbox.snap.padding:"),                 C_INT, (void*)2,        &Settings_edgeSnapPadding, sizeof(Settings_edgeSnapPadding) },
	{ _T("blackbox.snap.threshold:"),               C_INT, (void*)7,        &Settings_edgeSnapThreshold, sizeof(Settings_edgeSnapThreshold) },

	{ _T("blackbox.background.enabled:"),           C_BOL, (void*)true,     &Settings_background_enabled, sizeof(Settings_background_enabled)  },
	{ _T("blackbox.background.smartWallpaper:"),    C_BOL, (void*)true,     &Settings_smartWallpaper, sizeof(Settings_smartWallpaper) },

	{ _T("blackbox.workspaces.followActive:"),      C_BOL, (void*)true,     &Settings_followActive, sizeof(Settings_followActive) },
	{ _T("blackbox.workspaces.followMoved:"),       C_BOL, (void*)true,     &Settings_followMoved, sizeof(Settings_followMoved) },
	{ _T("blackbox.workspaces.altMethod:"),         C_BOL, (void*)false,    &Settings_altMethod, sizeof(Settings_altMethod) },
	{ _T("blackbox.workspaces.restoreToCurrent:"),  C_BOL, (void*)true,     &Settings_restoreToCurrent, sizeof(Settings_restoreToCurrent) },
	{ _T("blackbox.workspaces.xpfix:"),             C_BOL, (void*)false,    &Settings_workspacesPCo, sizeof(Settings_workspacesPCo) },

	{ _T("blackbox.options.shellContextMenu:"),     C_BOL, (void*)false,    &Settings_shellContextMenu, sizeof(Settings_shellContextMenu) },
	{ _T("blackbox.options.desktopHook:"),          C_BOL, (void*)false,    &Settings_desktopHook, sizeof(Settings_desktopHook)   },
	{ _T("blackbox.options.logging:"),              C_INT, (void*)0,        &Settings_LogFlag, sizeof(Settings_LogFlag) },

	{ _T("blackbox.global.fonts.enabled:"),         C_BOL, (void*)false,    &Settings_globalFonts, sizeof(Settings_globalFonts) },
	{ _T("blackbox.editor:"),                       C_STR, (void*)_T("notepad.exe"), Settings_preferredEditor, sizeof(Settings_preferredEditor)/sizeof(TCHAR) },

	{ _T("blackbox.menu.volumeWidth:"),             C_INT, (void*)0,        &Settings_menuVolumeWidth, sizeof(Settings_menuVolumeWidth) },
	{ _T("blackbox.menu.volumeHeight:"),            C_INT, (void*)18,       &Settings_menuVolumeHeight, sizeof(Settings_menuVolumeHeight) },
	{ _T("blackbox.menu.volumeHilite:"),            C_BOL, (void*)true,     &Settings_menuVolumeHilite, sizeof(Settings_menuVolumeHilite) },
	{ _T("blackbox.menu.keyBindVI:"),               C_BOL, (void*)false,    &Settings_menuKeyBindVI, sizeof(Settings_menuKeyBindVI) },
	{ _T("blackbox.menu.iconSize:"),                C_INT, (void*)16,       &Settings_menuIconSize, sizeof(Settings_menuIconSize) },
	{ _T("blackbox.menu.maxHeightRatio:"),          C_INT, (void*)75,       &Settings_menuMaxHeightRatio, sizeof(Settings_menuMaxHeightRatio) },
	{ _T("blackbox.recent.menuFile:"),              C_STR, (void*)_T(""),   &Settings_recentMenu, sizeof(Settings_recentMenu)/sizeof(TCHAR) },
	{ _T("blackbox.recent.itemKeepSize:"),          C_INT, (void*)3,        &Settings_recentItemKeepSize, sizeof(Settings_recentItemKeepSize) },
	{ _T("blackbox.recent.itemSortSize:"),          C_INT, (void*)5,        &Settings_recentItemSortSize, sizeof(Settings_recentItemSortSize) },
	{ _T("blackbox.recent.withBeginEnd:"),          C_BOL, (void*)true,     &Settings_recentBeginEnd, sizeof(Settings_recentBeginEnd) },
	{ _T("blackbox.separator.margin:"),             C_INT, (void*)0,        &Settings_separatorMargin, sizeof(Settings_separatorMargin) },
	{ _T("blackbox.separator.style:"),              C_STR, (void*)_T(""),   &Settings_separatorStyle, sizeof(Settings_separatorStyle)/sizeof(TCHAR) },
	// --------------------------------

	{ NULL, 0, NULL, NULL, 0 }
};

//===========================================================================
struct rccfg rccfg[] = {
	{ _T("#toolbar.enabled:"),          C_BOL, (void*)true,     &Settings_toolbarEnabled,sizeof(Settings_toolbarEnabled) },
	{ _T("#toolbar.placement:"),        C_STR, (void*)_T("TopCenter"), Settings_toolbarPlacement,sizeof(Settings_toolbarPlacement)/sizeof(TCHAR) },
	{ _T("#toolbar.widthPercent:"),     C_INT, (void*)66,       &Settings_toolbarWidthPercent,sizeof(Settings_toolbarWidthPercent) },
	{ _T("#toolbar.onTop:"),            C_BOL, (void*)false,    &Settings_toolbarOnTop,sizeof(Settings_toolbarOnTop) },
	{ _T("#toolbar.autoHide:"),         C_BOL, (void*)false,    &Settings_toolbarAutoHide,sizeof(Settings_toolbarAutoHide) },
	{ _T("#toolbar.pluginToggle:"),     C_BOL, (void*)true ,    &Settings_toolbarPluginToggle,sizeof(Settings_toolbarPluginToggle) },
	{ _T("#toolbar.alpha.enabled:"),    C_BOL, (void*)false,    &Settings_toolbarAlphaEnabled,sizeof(Settings_toolbarAlphaEnabled) },
	{ _T("#toolbar.alpha.value:"),      C_INT, (void*)255,      &Settings_toolbarAlphaValue,sizeof(Settings_toolbarAlphaValue) },

	{ _T(".menu.position.x:"),          C_INT, (void*)100,      &Settings_menuPositionX,sizeof(Settings_menuPositionX) },
	{ _T(".menu.position.y:"),          C_INT, (void*)100,      &Settings_menuPositionY,sizeof(Settings_menuPositionY) },
	{ _T(".menu.maxWidth:"),            C_INT, (void*)240,      &Settings_menuMaxWidth,sizeof(Settings_menuMaxWidth) },
	{ _T(".menu.popupDelay:"),          C_INT, (void*)80,       &Settings_menuPopupDelay,sizeof(Settings_menuPopupDelay) },
	{ _T(".menu.mouseWheelFactor:"),    C_INT, (void*)3,        &Settings_menuMousewheelfac,sizeof(Settings_menuMousewheelfac) },
	{ _T(".menu.alpha.enabled:"),       C_BOL, (void*)false,    &Settings_menuAlphaEnabled,sizeof(Settings_menuAlphaEnabled) },
	{ _T(".menu.alpha.value:"),         C_INT, (void*)255,      &Settings_menuAlphaValue,sizeof(Settings_menuAlphaValue) },
	{ _T(".menu.onTop:"),               C_BOL, (void*)false,    &Settings_menusOnTop,sizeof(Settings_menusOnTop) },
	{ _T(".menu.snapWindow:"),          C_BOL, (void*)true,     &Settings_menusSnapWindow,sizeof(Settings_menusSnapWindow) },
	{ _T(".menu.pluginToggle:"),        C_BOL, (void*)true,     &Settings_menuspluginToggle,sizeof(Settings_menuspluginToggle) },
	{ _T(".menu.bulletPosition:"),      C_STR, (void*)_T("default"), &Settings_menuBulletPosition_cfg,sizeof(Settings_menuBulletPosition_cfg)/sizeof(TCHAR) },
	{ _T(".menu.sortbyExtension:"),     C_BOL, (void*)false,    &Settings_menusExtensionSort,sizeof(Settings_menusExtensionSort) },

	{ _T("#workspaces:"),               C_INT, (void*)4,        &Settings_workspaces,sizeof(Settings_workspaces) },
	{ _T("#workspaceNames:"),           C_STR, (void*)_T("alpha,beta,gamma,delta"), &Settings_workspaceNames,sizeof(Settings_workspaceNames)/sizeof(TCHAR) },
	{ _T("#strftimeFormat:"),           C_STR, (void*)_T("%I:%M %p"), Settings_toolbarStrftimeFormat,sizeof(Settings_toolbarStrftimeFormat)/sizeof(TCHAR) },
	{ _T("#fullMaximization:"),         C_BOL, (void*)false,    &Settings_fullMaximization,sizeof(Settings_fullMaximization) },
	{ _T("#focusModel:"),               C_STR, (void*)_T("ClickToFocus"), Settings_focusModel,sizeof(Settings_focusModel)/sizeof(TCHAR) },

	{ _T(".imageDither:"),              C_BOL, (void*)true,     &Settings_imageDither,sizeof(Settings_imageDither) },
	{ _T(".opaqueMove:"),               C_BOL, (void*)true,     &Settings_opaqueMove,sizeof(Settings_opaqueMove) },
	{ _T(".autoRaiseDelay:"),           C_INT, (void*)250,      &Settings_autoRaiseDelay,sizeof(Settings_autoRaiseDelay) },

	//{ _T(".changeWorkspaceWithMouseWheel:"), C_BOL, (void*)false,    &Settings_desktopWheel   },

	/* *nix settings, not used here
	// ----------------------------------
	{ _T("#edgeSnapThreshold:"),        C_INT, (void*)7,        &Settings_edgeSnapThreshold },

	{ _T("#focusLastWindow:"),          C_BOL, (void*)false,    &Settings_focusLastWindow },
	{ _T("#focusNewWindows:"),          C_BOL, (void*)false,    &Settings_focusNewWindows },
	{ _T("#windowPlacement:"),          C_STR, (void*)_T("RowSmartPlacement"), &Settings_windowPlacement },
	{ _T("#colPlacementDirection:"),    C_STR, (void*)_T("TopToBottom"), &Settings_colPlacementDirection },
	{ _T("#rowPlacementDirection:"),    C_STR, (void*)_T("LeftToRight"), &Settings_rowPlacementDirection },

	{ _T(".colorsPerChannel:"),         C_INT, (void*)4,        &Settings_colorsPerChannel },
	{ _T(".doubleClickInterval:"),      C_INT, (void*)250,      &Settings_dblClickInterval },
	{ _T(".cacheLife:"),                C_INT, (void*)5,        &Settings_cacheLife },
	{ _T(".cacheMax:"),                 C_INT, (void*)200,      &Settings_cacheMax },
	// ---------------------------------- */

	{ NULL }
};

//===========================================================================
const TCHAR * makekey(TCHAR *buff, size_t buff_size, struct rccfg * cp)
{
	const TCHAR *k = cp->key;
	if (k[0]==_T('.'))
		_sntprintf_s(buff, buff_size, _TRUNCATE, _T("session%s"), k);
	else
		if (k[0]==_T('#'))
			_sntprintf_s(buff, buff_size, _TRUNCATE, _T("session.screen%d.%s"), screenNumber, k+1);
		else
			return k;
	return buff;
}

void Settings_ReadSettings(const TCHAR *bbrc, struct rccfg * cp)
{
	do {
		TCHAR keystr[80]; const TCHAR *key = makekey(keystr, 80, cp);
		switch (cp->mode)
		{
		case C_INT:
			*(int*) cp->ptr = ReadInt(bbrc, key, (int) cp->def);
			break;
		case C_BOL:
			*(bool*) cp->ptr = ReadBool (bbrc, key, cp->def != 0);
			break;
		case C_STR:
			_tcsncpy_s((TCHAR*)cp->ptr, cp->ptr_size, ReadString (bbrc, key, (TCHAR*) cp->def), _TRUNCATE);
			break;
		}
	} while ((++cp)->key);
}

bool Settings_WriteSetting(const TCHAR *bbrc, struct rccfg * cp, const void *v)
{
	do if (NULL == v || cp->ptr == v)
	{
		TCHAR keystr[80]; const TCHAR *key = makekey(keystr, 80, cp);
		switch (cp->mode)
		{
		case C_INT:
			WriteInt (bbrc, key, *(int*) cp->ptr);
			break;
		case C_BOL:
			WriteBool (bbrc, key, *(bool*) cp->ptr);
			break;
		case C_STR:
			WriteString (bbrc, key, (TCHAR*) cp->ptr);
			break;
		}
		if (v) return true;
	} while ((++cp)->key);
	return false;
}

//===========================================================================

//===========================================================================
void Settings_WriteRCSetting(const void *v)
{
	Settings_WriteSetting(bbrcPath(), rccfg, v)
		||
			Settings_WriteSetting(extensionsrcPath(), ext_rccfg, v);
}

void Settings_ReadRCSettings()
{
	Settings_ReadSettings(bbrcPath(), rccfg);
	Settings_ReadSettings(extensionsrcPath(), ext_rccfg);
	menuPath(ReadString(bbrcPath(), _T("session.menuFile:"), NULL));
	stylePath(ReadString(bbrcPath(), _T("session.styleFile:"), _T("")));
}

//===========================================================================

void Settings_ReadStyleSettings()
{
	free_extension_style();
	ReadStyle(stylePath());

#if 1
	// ----------------------------------------------------
	// set some defaults for missing style settings

	if (mStyle.Toolbar.validated & VALID_TEXTCOLOR)
	{
		if (0==(mStyle.ToolbarLabel.validated & VALID_TEXTCOLOR))
			mStyle.ToolbarLabel.TextColor = mStyle.Toolbar.TextColor;

		if (0==(mStyle.ToolbarClock.validated & VALID_TEXTCOLOR))
			mStyle.ToolbarClock.TextColor = mStyle.Toolbar.TextColor;

		if (0==(mStyle.ToolbarWindowLabel.validated & VALID_TEXTCOLOR))
			mStyle.ToolbarWindowLabel.TextColor = mStyle.Toolbar.TextColor;
	}
	else
	{
		if (mStyle.ToolbarLabel.parentRelative)
			mStyle.Toolbar.TextColor = mStyle.ToolbarLabel.TextColor;
		else
			if (mStyle.ToolbarClock.parentRelative)
				mStyle.Toolbar.TextColor = mStyle.ToolbarClock.TextColor;
			else
				if (mStyle.ToolbarWindowLabel.parentRelative)
					mStyle.Toolbar.TextColor = mStyle.ToolbarWindowLabel.TextColor;
				else
					mStyle.Toolbar.TextColor = mStyle.ToolbarLabel.TextColor;
	}

	if (0==(mStyle.ToolbarButtonPressed.validated & VALID_PICCOLOR))
		mStyle.ToolbarButtonPressed.picColor = mStyle.ToolbarButton.picColor;

	if (0==(mStyle.Toolbar.validated & VALID_JUSTIFY))
		mStyle.Toolbar.Justify = DT_CENTER;

	if (0==(mStyle.ToolbarWindowLabel.validated & VALID_TEXTURE))
		if (mStyle.ToolbarLabel.validated & VALID_TEXTURE)
			mStyle.ToolbarWindowLabel = mStyle.ToolbarLabel;

	if (0 == (mStyle.MenuFrame.validated & VALID_TEXTURE)
		&& 0 == mStyle.rootCommand[0])
		_tcsncpy_s(mStyle.rootCommand, MAX_PATH+80, _T("bsetroot -mod 4 4 -fg grey55 -bg grey60"), _TRUNCATE);
#endif

	// default SplitColor
	struct items *p = StyleItems;
	do{
		StyleItem* pSI = (StyleItem*)(p->v);
		bool is_split = (pSI->type == B_SPLITVERTICAL) || (pSI->type == B_SPLITHORIZONTAL);
		if (p->flags & VALID_FROMSPLITTO){
			if(is_split && !(pSI->validated & VALID_FROMSPLITTO)){
				unsigned int r = GetRValue(pSI->Color);
				unsigned int g = GetGValue(pSI->Color);
				unsigned int b = GetBValue(pSI->Color);
				r = (unsigned int)iminmax(r + (r>>2), 0, 255);
				g = (unsigned int)iminmax(g + (g>>2), 0, 255);
				b = (unsigned int)iminmax(b + (b>>2), 0, 255);
				pSI->ColorSplitTo = RGB(r, g, b);
				pSI->validated |= VALID_FROMSPLITTO;
			}
		}
		if (p->flags & VALID_TOSPLITTO){
			if(is_split && !(pSI->validated & VALID_TOSPLITTO)){
				unsigned int r = GetRValue(pSI->ColorTo);
				unsigned int g = GetGValue(pSI->ColorTo);
				unsigned int b = GetBValue(pSI->ColorTo);
				r = (unsigned int)iminmax(r + (r>>4), 0, 255);
				g = (unsigned int)iminmax(g + (g>>4), 0, 255);
				b = (unsigned int)iminmax(b + (b>>4), 0, 255);
				pSI->ColorToSplitTo = RGB(r, g, b);
				pSI->validated |= VALID_TOSPLITTO;
			}
		}
	} while ((++p)->v);
}

//===========================================================================
#endif // #ifndef BBSETTING_STYLEREADER_ONLY
//===========================================================================
