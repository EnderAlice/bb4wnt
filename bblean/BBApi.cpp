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

#include <shellapi.h>
#include <time.h>
#include <imm.h>

#define ST static

TCHAR tempBuf[1024];

TCHAR blackboxrc_path[MAX_PATH];
TCHAR extensionsrc_path[MAX_PATH];
TCHAR menurc_path[MAX_PATH];
TCHAR pluginrc_path[MAX_PATH];
TCHAR stylerc_path[MAX_PATH];

//===========================================================================

unsigned long getfileversion(const TCHAR *path, TCHAR *buffer, size_t buffer_size)
{
	TCHAR *info_buffer; void *value; UINT bytes; DWORD result; DWORD dwHandle;

	if (buffer) buffer[0] = 0;

	dwHandle = 0;
	result = GetFileVersionInfoSize((LPTSTR)path, &dwHandle);
	if (0 == result)
		return 0;

	if (FALSE == GetFileVersionInfo((LPTSTR)path, 0, result, info_buffer=(TCHAR*)m_alloc(result)))
		return 0;

	result = 0;

	if (buffer)
	{
		if (VerQueryValue(info_buffer,
						  // change to whatever version encoding used (currently language neutral)
						  _T("\\StringFileInfo\\000004b0\\FileVersion"),
						  &value, &bytes))
		{
			_tcsncpy_s(buffer, buffer_size, (const TCHAR*)value, _TRUNCATE);
			result = 1;
			//dbg_printf(_T("version of %s <%s>"), path, buffer);
		}
	}
	else
		if (VerQueryValue(info_buffer, _T("\\"), &value, &bytes))
		{
			VS_FIXEDFILEINFO *vs = (VS_FIXEDFILEINFO*)value;
			result = MAKELPARAM(
				MAKEWORD(LOWORD(vs->dwFileVersionLS), HIWORD(vs->dwFileVersionLS)),
				MAKEWORD(LOWORD(vs->dwFileVersionMS), HIWORD(vs->dwFileVersionMS)));
			//dbg_printf(_T("version number of %s %08x"), path, result);
		}
	m_free(info_buffer);
	return result;
}

//===========================================================================
// API: GetBBVersion
// Purpose: Returns the current version
// In: None
// Out: LPCTSTR = Formatted Version String
//===========================================================================

API_EXPORT(LPCTSTR) GetBBVersion(void)
{
	static TCHAR bb_version [40];
	if (0 == *bb_version) getfileversion(bb_exename, bb_version, 40);
	return bb_version;
}

//===========================================================================
// API: GetBBWnd
// Purpose: Returns the handle to the main Blackbox window
// In: None
// Out: HWND = Handle to the Blackbox window
//===========================================================================

API_EXPORT(HWND) GetBBWnd()
{
	return BBhwnd;
}

//===========================================================================
// API: GetOSInfo
// Purpose: Retrieves info about the current OS
// In: None
// Out: LPCTSTR = Returns a readable string containing the OS name
//===========================================================================

API_EXPORT(LPCTSTR) GetOSInfo(void)
{
	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		if (osInfo.dwMinorVersion >= 90)
			return _T("Windows ME");
		if (osInfo.dwMinorVersion >= 10)
			return _T("Windows 98");
		return _T("Windows 95");
	}
	if (osInfo.dwMajorVersion == 5)
	{
		if (osInfo.dwMinorVersion >= 1)
			return _T("Windows XP");
		return _T("Windows 2000");
	}
	static TCHAR osinfo_buf[40];
	_sntprintf_s(osinfo_buf, 40, _TRUNCATE, _T("Windows NT %d.%d"), (int)osInfo.dwMajorVersion, (int)osInfo.dwMinorVersion);
	return osinfo_buf;
}

//===========================================================================
// API: GetBlackboxPath
// Purpose: Copies the path of the Blackbox executable to the specified buffer
// In: LPTSTR pszPath = Location of the buffer for the path
// In: int nMaxLen = Maximum length for the buffer
// Out: bool = Returns status of completion
//===========================================================================

API_EXPORT(LPTSTR) GetBlackboxPath(LPTSTR pszPath, size_t nMaxLen)
{
	GetModuleFileName(NULL, pszPath, (DWORD)nMaxLen);
	*(TCHAR*)get_file(pszPath) = 0;
	return pszPath;
}

//===========================================================================
// Function: NextToken
// Purpose: Copy the first token of 'string' seperated by the delim to 'buf'
//          and move the rest of 'string' down.
// returns: 'buf'
//===========================================================================

LPTSTR NextToken(LPTSTR buf, size_t buf_size, LPCTSTR *string, const TCHAR *delims)
{
	TCHAR c, q=0;
	TCHAR* bufptr = buf;
	const TCHAR* s = *string;
	bool delim_spc = NULL == delims || _tcschr(delims, _T(' '));

	while (0 != (c=*s))
	{
		s++;
		if (0==q)
		{
			// User-specified delimiter
			if ((delims && _tcschr(delims, c)) || (delim_spc && IS_SPC(c)))
				break;
			if ((size_t)(s - *string) >= buf_size)
				break;

			if (_T('\"')==c || _T('\'')==c) q=c;
			else
				if (IS_SPC(c) && bufptr==buf)
					continue;
		}
		else
			if (c==q)
			{
				q=0;
			}
		*bufptr++ = c;
	}
	while (bufptr > buf && IS_SPC(bufptr[-1])) bufptr--;
	*bufptr = _T('\0');
	while (IS_SPC(*s) && *s) s++;
	*string = s;
	return buf;
}

//===========================================================================
// API: Tokenize
// Purpose: Retrieve the first string seperated by the delim and return a pointer to the rest of the string
// In: LPCTSTR string = String to be parsed
// In: LPTSTR buf = String where the tokenized string will be placed
// In: LPTSTR delims = The delimeter that signifies the seperation of the tokens in the string
// Out: LPTSTR = Returns a pointer to the entire remaining string
//===========================================================================

API_EXPORT(LPTSTR) Tokenize(LPCTSTR string, LPTSTR buf, size_t buf_size, LPCTSTR delims)
{
	NextToken(buf,buf_size, &string, delims);_tcsncpy_s(tempBuf, 1024, string, _TRUNCATE);// TODO: 排他処理を追加する
	return tempBuf;
}

//===========================================================================
// API: BBTokenize
// Purpose: Assigns a specified number of string variables and outputs the remaining to a variable
// In: LPCTSTR szString = The string to be parsed
// In: LPTSTR lpszBuffers[] = A pointer to the location for the tokenized strings
// In: DWORD dwNumBuffers = The amount of tokens to be parsed
// In: LPTSTR szExtraParameters = A pointer to the location for the remaining string
// Out: int = Number of tokens parsed
//===========================================================================

API_EXPORT(int) BBTokenize (LPCTSTR srcString, TCHAR **lpszBuffers, DWORD dwNumBuffers, LPTSTR szExtraParameters, size_t szExtraParameters_size)
{
	int   ol, stored; DWORD dwBufferCount;
	TCHAR  quoteChar, c, *output;
	quoteChar = 0; dwBufferCount = stored = 0;
	while (dwBufferCount < dwNumBuffers)
	{
		output = lpszBuffers[dwBufferCount]; ol = 0;
		while (0 != (c = *srcString))
		{
			srcString++;
			switch (c) {
			case _T(' '):
			case _T('\t'):
				if (quoteChar) goto _default;
				if (ol) goto next;
				continue;

			case _T('"'):
			case _T('\''):
				if (0==quoteChar) { quoteChar = c; continue; }
				if (c==quoteChar) { quoteChar = 0; goto next; }

			_default:
			default:
				output[ol]=c; ol++; continue;
			}
		}
		if (ol) next: stored++;
		output[ol]=0; dwBufferCount++;
	}
	if (szExtraParameters)
	{
		while (0 != (c = *srcString) && IS_SPC(c)) srcString++;
		_tcsncpy_s(szExtraParameters, szExtraParameters_size, srcString, _TRUNCATE);
	}
	return stored;
}

//===========================================================================
// API: StrRemoveEncap
// Purpose: Removes the first and last characters of a string
// In: LPTSTR string = The string to be altered
// Out: LPTSTR = A pointer to the altered string
//===========================================================================

API_EXPORT(LPTSTR) StrRemoveEncap(LPTSTR string, size_t string_size)
{
	size_t l = _tcsnlen(string,string_size);
	if (l>2) _tmemmove(string, string+1, l-=2);
	else l=0;
	*(string+l) = _T('\0');
	return string;
}

//===========================================================================
// API: IsInString
// Purpose: Checks a given string to an occurance of the second string
// In: LPCTSTR = string to search
// In: LPCTSTR = string to search for
// Out: bool = found or not
//===========================================================================

API_EXPORT(bool) IsInString(LPCTSTR inputString, LPCTSTR searchString)
{
	// xoblite-flavour plugins bad version test workaround
	if (0 == _tcscmp(searchString, _T("bb")) && 0 == _tcscmp(inputString, GetBBVersion()))
		return false;

	return NULL != stristr(inputString, searchString);
}

//===========================================================================
// tiny cache reader: first checks, if the file had already been read, if not,
// reads the file into a malloc'ed buffer, then for each line separates
// keyword from value, cuts off leading and trailing spaces, strlwr's the
// keyword and adds both to a list of below defined structures, where k is
// the offset to the start of the value-string. Comments or other non-keyword
// lines have a "" keyword and the line as value.

// added checking for external updates by the user.

// added *wild*card* processing, it first looks for an exact match, if it
// cant find any, returns the first wildcard value, that matches, or null,
// if none...

//===========================================================================
// structures

struct lin_list
{
	struct lin_list *next;
	unsigned hash; size_t k, o, v;
	bool is_wild;
	TCHAR str[2];
};

struct fil_list
{
	struct fil_list *next;
	struct lin_list *lines;
	struct list_node *wild;
	unsigned hash, k;
	FILETIME ft;
	DWORD tickcount;
	bool dirty;
	bool new_style;
	TCHAR path[1];
} *rc_files = NULL;

//===========================================================================

FILE *create_rcfile(const TCHAR *path)
{
	FILE *fp = NULL;
	//dbg_printf(_T("writing to %s"), path);
	if (_tfopen_s(&fp, path, _T("wt, ccs=UTF-8")))
		BBMessageBox(MB_OK, NLS2(_T("$BBError_WriteFile$"), _T("Error: Could not open \"%s\" for writing.")), string_empty_or_null(path));
	return fp;
}

void write_rcfile(struct fil_list *fl)
{
	FILE *fp;
	if (NULL==(fp=create_rcfile(fl->path)))
		return;

	size_t ml = 0;
	struct lin_list *tl;

	// calculate the max. keyword length
	dolist (tl, fl->lines) if (tl->k > ml) ml = tl->k;
	ml |= 3; // round to the next tabstop

	dolist (tl, fl->lines)
	{
		TCHAR *s = tl->str + tl->k;
		if (0 == *tl->str) _ftprintf (fp, _T("%s\n"), s); //comment
		else _ftprintf (fp, _T("%-*s %s\n"), ml, tl->str+tl->o, s);
	}
	fclose(fp);
	get_filetime(fl->path, &fl->ft);
	fl->dirty = false;
}

void mark_rc_dirty(struct fil_list *fl)
{
#ifdef BBOPT_DEVEL
#pragma message("\n"__FILE__ "(370) : warning X0: Delayed Writing to rc-files enabled.\n")
	fl->dirty = true;
	SetTimer(BBhwnd, BB_WRITERC_TIMER, 2, NULL);
#else
	write_rcfile(fl);
#endif
}

//===========================================================================

void delete_from_list(struct fil_list *fl)
{
	freeall(&fl->lines);
	freeall(&fl->wild);
	remove_item(&rc_files, fl);
}

void flush_file(struct fil_list *fl)
{
	if (fl->dirty) write_rcfile(fl);
	delete_from_list(fl);
}

void write_rcfiles(void)
{
	struct fil_list *fl;
	dolist (fl, rc_files) if (fl->dirty) write_rcfile(fl);
}

void reset_reader(void)
{
	while (rc_files) flush_file(rc_files);
}

//===========================================================================
// helpers

unsigned calc_hash(TCHAR *p, const TCHAR *s, size_t *pLen)
{
	unsigned h, c; TCHAR *d = p;
	for (h = 0; 0 != (c = *s); ++s, ++d)
	{
		if (c <= _T('Z') && c >= _T('A')) c += 32;
		*d = c;
		if ((h ^= c) & 1) h^=0xedb88320;
		h>>=1;
	}
	*d = 0;
	*pLen = d - p;
	return h;
}

// Xrm database-like wildcard pattern matcher
// ------------------------------------------
// returns: 0 for no match, else a number that is somehow a measure for
// 'how much' the items match. Btw 'toolbar*color' means just the
// same as 'toolbar.*.color' and both match e.g. 'toolbar.color'
// as well as 'toolbar.label.color', 'toolbar.button.pressed.color', ...

int xrm_match (const TCHAR *str, const TCHAR *pat)
{
	int c = 256, m = 0; const TCHAR *pp, *ss; TCHAR s, p; int l;
	for (;;)
	{
		s = *str, p = *pat;
		if (0 == s || _T(':') == s) return s == p ? m : 0;
		// scan component in the string
		for (l=1, ss=str+1; 0!=*ss && _T(':')!=*ss && _T('.')!=*ss++;++l);
		if (_T('*') == p)
		{
			// ignore dots with _T('*')
			for (pp = pat+1; _T('.')==*pp; ++pp);
			int n = xrm_match(ss, pat);
			if (n) return m + c + n*c/(256*2);
			pat = pp;
			continue;
		}
		// scan component in the pattern
		for (pp=pat+1; 0!=*pp && _T(':')!=*pp && _T('*')!=*pp && _T('.')!=*pp++;);
		if (_T('?') == p)
			m += c; // one point with matching wildcard
		else
			if (_tmemcmp(str, pat, l))
				return 0; // zero for no match
			else
				m += 2*c; // two points for exact match
		str = ss; pat = pp; c /= 2;
	}
}

struct lin_list *make_line (struct fil_list *fl, const TCHAR *key, size_t k, const TCHAR *val)
{
	struct lin_list *tl; size_t v = _tcslen(val);
	tl=(struct lin_list*)m_alloc(sizeof(*tl) + (sizeof(TCHAR) * (k*2 + v + 1)));
	tl->v = v++;
	tl->o = (tl->k = k+1) + v;
	_tmemcpy(tl->str + tl->k, val, v);
	*((TCHAR*)_tmemcpy(tl->str + tl->o, key, k) + k) = 0;
	tl->hash = calc_hash(tl->str, tl->str + tl->o, &k);
	tl->next = NULL;
	tl->is_wild = false;
	//if the key contains a wildcard
	if (k && _tmemchr(key, _T('*'), k) || _tmemchr(key, _T('?'), k))
	{
		// add it to the wildcard - list
		append_node(&fl->wild, new_node(tl));
		tl->is_wild = true;
	}
	return tl;
}

void free_line(struct fil_list *fl, struct lin_list *tl)
{
	if (tl->is_wild) delete_assoc(&fl->wild, tl);
	m_free(tl);
}

struct lin_list **search_line(struct lin_list **tlp, const TCHAR *key, struct list_node *wild)
{
	int n, m; size_t k; TCHAR buff[256]; unsigned h = calc_hash(buff, key, &k);
	struct lin_list **wlp = NULL, *tl;
	while (NULL != (tl = *tlp))
	{
		if (tl->hash==h && 0==_tmemcmp(tl->str, buff, k) && k)
			return tlp;
		tlp = &tl->next;
	}
	m = 0;
	while (NULL != wild)
	{
		tl = *(wlp = (struct lin_list **)&wild->v);
		n = xrm_match(buff, tl->str);
		if (n > m) tlp = wlp, m = n;
		wild = wild->next;
	}
	return tlp;
}

TCHAR * read_file_into_buffer (const TCHAR *path, int max_len)
{
	FILE *fp; BYTE *buf; int k;
	BYTE bom[4];
	if (_tfopen_s(&fp, path,_T("rb")))
		return NULL;

	// 最初の3バイトを確認
	fseek(fp,0,SEEK_SET);
	fread(bom, 1, 3, fp);
	// BOMならポインタを3バイトずらす 何で1バイト目が0xeeに‥‥
	if(bom[0] == 0xef && bom[1] == 0xbb && bom[2] == 0xbf)
	{
		fseek(fp,0,SEEK_END);
		k = ftell(fp) - 3;
		fseek(fp, 3, SEEK_SET);
	}
	// BOMではない場合はポインタを最初の位置に戻す
	else
	{
		fseek(fp,0,SEEK_END);
		k = ftell (fp);
		fseek(fp, 0, SEEK_SET);
	}

	// 0バイトかBOMしかない場合はNULLを返す
	if(k <= 0)
	{
		fclose(fp);
		return NULL;
	}

	if (max_len && k >= max_len) k = max_len-1;

	buf=(BYTE *)m_alloc(k+1);
	fread (buf, 1, k, fp);
	fclose(fp);
	buf[k]=0;

#ifdef UNICODE
	// freadで読み込む場合は文字コード変換が入らないので
	// 生のまま読み込んでから変換を行う
	WCHAR * wbuf;
	int wk;

	wk = MultiByteToWideChar(CP_UTF8,0,(LPCSTR)buf,k,NULL,0);
	wbuf = (WCHAR*)m_alloc(sizeof(WCHAR) * (wk + 1));
	MultiByteToWideChar(CP_UTF8,0,(LPCSTR)buf,k,wbuf,wk);
	wbuf[wk] = _T('\0');
	m_free(buf); // 元データは不要なので破棄

	return wbuf;
#else
	return buf;
#endif
}

bool is_stylefile(const TCHAR *path)
{
	TCHAR *temp = read_file_into_buffer(path, 10000);
	bool r = false;
	if (temp)
	{
		//_tcslwr_s(temp, _tcsnlen(temp, 10000));
		r = (NULL != _tcsstr(temp, _T("menu.frame")));
		m_free(temp);
	}
	return r;
}

TCHAR scan_line(TCHAR **pp, TCHAR **ss, size_t *ll)
{
	TCHAR c, e, *d, *s, *p = *pp;
	for (; 0!=(c=*p) && 10!=c && IS_SPC(c); p++);
	//find end of line
	for (s=p; 0!=(e=*p) && 10!=e; p++);
	//cut off trailing spaces
	for (d = p; d>s && IS_SPC(d[-1]); d--); *d=0;
	//ready for next line
	if (e) ++p;
	*pp = p, *ss = s, *ll = d-s;
	return c;
}

//===========================================================================
// BB070

void translate_items070(TCHAR *buffer, size_t bufsize, TCHAR **pkey, size_t *pklen)
{
	static const TCHAR *pairs [] =
	{
		// from         -->   to
		_T(".appearance")       , _T("")                ,
		_T("alignment")         , _T("justify")         ,
		_T("color1")            , _T("color")           ,
		_T("color2")            , _T("colorTo")         ,
		_T("backgroundColor")   , _T("color")           ,
		_T("foregroundColor")   , _T("picColor")        ,
		_T("disabledColor")     , _T("disableColor")    ,
		_T("menu.active")       , _T("menu.hilite")     ,
		NULL
		};

	size_t k = *pklen;
	if (k >= bufsize) return;
	*pkey = (TCHAR *)_tmemcpy(buffer, *pkey, k);
	buffer[k] = 0;
	const TCHAR **p = pairs;
	do
	{
		TCHAR *q = (TCHAR*)stristr(buffer, *p);
		if (q)
		{
			size_t lp = _tcslen(p[0]);
			size_t lq = _tcslen(q);
			size_t lr = _tcslen(p[1]);
			size_t k0 = k + lr - lp;
			if (k0 >= bufsize) break;
			_tmemmove(q + lr, q + lp, lq - lp + 1);
			_tmemmove(q, p[1], lr);
			k = k0;
		}
	} while ((p += 2)[0]);
	*pklen = k;
}

//===========================================================================
// searches for the filename and, if not found, builds a _new line-list

struct fil_list *read_file(const TCHAR *filename)
{
	struct lin_list **slp, *sl;
	struct fil_list **flp, *fl;
	TCHAR *buf, *p, *d, *s, *t, c; size_t k;

	DWORD ticknow = GetTickCount();
	TCHAR path[MAX_PATH];
	unsigned h = calc_hash(path, filename, &k);

	// ----------------------------------------------
	// first check, if the file has already been read
	for (flp = &rc_files; NULL!=(fl=*flp); flp = &fl->next)
	{
		if (fl->hash==h && 0==_tmemcmp(path, fl->path, 1+k))
		{
			// re-check the timestamp after 20 ms
			if (fl->tickcount + 20 < ticknow)
			{
				//dbg_printf(_T("check time: %s"), path);
				if (check_filetime(path, &fl->ft))
				{
					// file was externally updated
					delete_from_list(fl);
					break;
				}
				fl->tickcount = ticknow;
			}
			return fl; //... return cached line list.
		}
	}

	// ----------------------------------------------

	// limit to 8 cached files
	fl = (struct fil_list *)nth_node(rc_files, 7);
	if (fl) while (fl->next) flush_file(fl->next);

	//dbg_printf(_T("read file %s"), path);

	// allocate a _new file structure
	fl = (struct fil_list*)c_alloc(sizeof(*fl) + sizeof(TCHAR) * (k+1));
	cons_node(&rc_files, fl);

	_tmemcpy(fl->path, path, k+1); //memcpy(fl->path, path, sizeof(TCHAR) * (k+1));
	fl->hash = h;
	fl->tickcount = ticknow;

	buf = read_file_into_buffer(path, 0);
	if (NULL == buf) return fl;

	//set timestamp
	get_filetime(fl->path, &fl->ft);

	slp = &fl->lines; p = buf;

	fl->new_style =
		0 == _tcsicmp(fl->path, stylePath())
			&& stristr(buf, _T("appearance:"));

	for (;;)
	{
		c = scan_line(&p, &s, &k);
		if (0 == c) break;

		if (c == _T('#') || c == _T('!') || NULL == (d = (TCHAR*)_tmemchr(s, _T(':'), k)))
		{
			k=0, d = s;
		}
		else
		{   //skip spaces between key and value, replace tabs etc
			for (t = s; 0 != (c=*t); t++) if (IS_SPC(c)) *t = _T(' ');
			k = d - s + 1; while (*++d == _T(' '));
		}

		TCHAR buffer[128];
		if (fl->new_style && k)
			translate_items070(buffer, 128, &s, &k);

		//put it into the line structure
		sl = make_line(fl, s, k, d);
		//append it to the list
		slp = &(*slp=sl)->next;
	}
	m_free(buf);
	return fl;
}

//===========================================================================
// API: FoundLastValue
// Purpose: Was the last read value actually in the rc-file?
// returns: 0=no 1=yes 2=wildcard matched
//===========================================================================

int found_last_value;

API_EXPORT(int) FoundLastValue(void)
{
	return found_last_value;
}

//===========================================================================
// API: ReadValue
// Purpose: Searches the given file for the supplied keyword and returns a
// pointer to the value - string
// In: LPCTSTR path = String containing the name of the file to be opened
// In: LPCTSTR szKey = String containing the keyword to be looked for
// In: LONG ptr: optional: an index into the file to start search.
// Out: LPTSTR = Pointer to the value string of the keyword
//===========================================================================

API_EXPORT(LPCTSTR) ReadValue(LPCTSTR path, LPCTSTR szKey, LONG *ptr)
{
	//static int rcc; dbg_printf(_T("read %d %s"), ++rcc, szKey);
	struct fil_list *fl = read_file(path);
	struct list_node *wild = fl->wild;
	struct lin_list *tl, *tl_start = fl->lines;
	if (ptr)
	{
		// originally meant as offset to seek into the file,
		// implemented as line index
		tl_start = (struct lin_list *)nth_node(tl_start, (*ptr)++);
		wild = NULL;
	}

	tl = tl_start;
	if (szKey[0])
	{
		tl = *search_line(&tl, szKey, wild);
		if (ptr) while (tl_start != tl) (*ptr)++, tl_start = tl_start->next;
	}

	if (NULL == tl)
	{
		found_last_value = 0;
		return NULL;
	}

	if (tl->is_wild)
		found_last_value = 2;
	else
		found_last_value = 1;

	return tl->str + tl->k;
}

void read_extension_style(LPCTSTR path, LPCTSTR szKey)
{
	if (szKey[0]){
		size_t l = _tcslen(szKey) - 1;
		struct fil_list *fl = read_file(path);
		struct lin_list *tl;
		dolist (tl, fl->lines){
			if (0 == _tcsnicmp(tl->str, szKey, l)){
				if (TCHAR *p = _tcsstr(tl->str, _T(".ext."))){
					p = _tcsrchr(tl->str, _T('.'));

					// check duplicate itme
					bool bExist = false;
					struct ext_list *el0;
					dolist(el0, Settings_menuExtStyle){
						if(!_tcsnicmp(tl->str, el0->key, p - tl->str)){
							bExist = true;
							break;
						}
					}

					if(!bExist){
						struct ext_list *el = (struct ext_list*)c_alloc(sizeof(ext_list));
						_tcsncpy_s(el->key, 32, tl->str, p - tl->str);
						_tcsncpy_s(el->ext, 32, _tcsrchr(el->key, _T('.')), _TRUNCATE);
						append_node(&Settings_menuExtStyle, el);
					}
				}
			}
		}
	}
}

void free_extension_style(){
	freeall(&Settings_menuExtStyle);
}

bool is_newstyle(LPCTSTR path)
{
	struct fil_list *fl = read_file(path);
	return fl && fl->new_style;
}

//===========================================================================
// Search for the szKey in the file_list, replace, if found and the value
// did change, or append, if not found. Write to file on changes.

int stri_eq_len(const TCHAR *a, const TCHAR *b)
{
	TCHAR c, d; int n = 0;
	for (;;++a, ++b)
	{
		if (0 == (c = *b)) return n;
		if (0 == (d = *a^c))
		{
			if (_T('.') == c || _T(':') == c) ++n;
			continue;
		}
		if (d != 32 || (c |= 32) < _T('a') || c > _T('z'))
			return n;
	}
}

struct lin_list **longest_match(struct fil_list *fl, const TCHAR *key)
{
	struct lin_list **tlp = NULL, **slp, *sl;
	int n, m = 0;
	for (slp = &fl->lines; NULL!=(sl=*slp); slp = &sl->next)
	{
		n = stri_eq_len(sl->str, key);
		if (n >= m) m = n, tlp = &sl->next;
	}
	return tlp;
}

void WriteValue(LPCTSTR path, LPCTSTR szKey, LPCTSTR value)
{
	//dbg_printf(_T("WriteValue <%s> <%s> <%s>"), path, szKey, value);

	struct fil_list *fl = read_file(path);
	struct lin_list *tl, **tlp;
	tl = *(tlp = search_line(&fl->lines, szKey, NULL));
	if (tl)
	{
		if (value && 0==_tcscmp(tl->str + tl->k, value))
			return; //if it didn't change, quit

		*tlp = tl->next; free_line(fl, tl);
	}
#if 1
	else
	{
		struct lin_list **slp = longest_match(fl, szKey);
		if (slp) tlp = slp;
	}
#endif
	if (value)
	{
		tl = make_line(fl, szKey, _tcslen(szKey), value);
		tl->next = *tlp;
		*tlp = tl;
	}
	mark_rc_dirty(fl);
}

//===========================================================================
// API: DeleteSetting

API_EXPORT(bool) DeleteSetting(LPCTSTR path, LPCTSTR szKey)
{
	TCHAR buff[256]; size_t k;
	_tmemcpy(buff, szKey, 1 + (k = _tcslen(szKey))); _tcslwr_s(buff, 256);
	struct fil_list *fl = read_file(path);

	struct lin_list **slp, *sl; int dirty = 0;
	for (slp = &fl->lines; NULL!=(sl=*slp); )
	{
		//if (sl->k==1+k && 0==_tmemcmp(sl->str, buff, k))
		if ((_T('*') == *buff && 1==k) || xrm_match(sl->str, buff))
		{
			*slp = sl->next; free_line(fl, sl);
			++dirty;
		}
		else
		{
			slp = &(*slp)->next;
		}
	}
	if (dirty) mark_rc_dirty(fl);
	return 0 != dirty;
}

//===========================================================================
// API: RenameSetting

API_EXPORT(bool) RenameSetting(LPCTSTR path, LPCTSTR szKey, LPCTSTR new_keyword)
{
	TCHAR buff[256]; size_t k;
	_tmemcpy(buff, szKey, 1 + (k = _tcslen(szKey))); _tcslwr_s(buff, 256);
	struct fil_list *fl = read_file(path);

	struct lin_list **slp, *sl, *tl; int dirty = 0;
	for (slp = &fl->lines; NULL!=(sl=*slp); slp = &(*slp)->next)
	{
		if (sl->k==1+k && 0==_tmemcmp(sl->str, buff, k))
		{
			tl = make_line(fl, new_keyword, _tcslen(new_keyword), sl->str+sl->k);
			tl->next = sl->next;
			*slp = tl; free_line(fl, sl);
			++dirty;
		}
	}
	if (dirty) mark_rc_dirty(fl);
	return 0 != dirty;
}

//===========================================================================

// API: WriteString
API_EXPORT(void) WriteString(LPCTSTR fileName, LPCTSTR szKey, LPCTSTR value)
{
	WriteValue(fileName, szKey, value);
}

// API: WriteBool
API_EXPORT(void) WriteBool(LPCTSTR fileName, LPCTSTR szKey, bool value)
{
	WriteValue(fileName, szKey, value ? _T("true") : _T("false"));
}

// API: WriteInt
API_EXPORT(void) WriteInt(LPCTSTR fileName, LPCTSTR szKey, int value)
{
	TCHAR buff[32];
	_itot_s(value, buff, 32, 10); WriteValue(fileName, szKey, buff);
}

// API: WriteColor
API_EXPORT(void) WriteColor(LPCTSTR fileName, LPCTSTR szKey, COLORREF value)
{
	TCHAR buff[32];
	_sntprintf_s(buff, 32, _TRUNCATE, _T("#%06lx"), (unsigned long)switch_rgb (value));
	WriteValue(fileName, szKey, buff);
}


//===========================================================================

// API: ReadBool
API_EXPORT(bool) ReadBool(LPCTSTR fileName, LPCTSTR szKey, bool bDefault)
{
	LPCTSTR szValue = ReadValue(fileName, szKey);
	if (szValue)
	{
		if (!_tcsicmp(szValue, _T("true")))  return true;
		if (!_tcsicmp(szValue, _T("false"))) return false;
	}
	return bDefault;
}

// API: ReadInt
API_EXPORT(int) ReadInt(LPCTSTR fileName, LPCTSTR szKey, int nDefault)
{
	LPCTSTR szValue = ReadValue(fileName, szKey);
	return szValue ? _ttoi(szValue) : nDefault;
}

// API: ReadString
API_EXPORT(LPCTSTR) ReadString(LPCTSTR fileName, LPCTSTR szKey, LPCTSTR szDefault)
{
	LPCTSTR szValue = ReadValue(fileName, szKey);
	return szValue ? szValue : szDefault;
}

// API: ReadColor
API_EXPORT(COLORREF) ReadColor(LPCTSTR fileName, LPCTSTR szKey, LPCTSTR defaultString)
{
	LPCTSTR szValue = szKey[0] ? ReadValue(fileName, szKey) : NULL;
	return ReadColorFromString(szValue ? szValue : defaultString);
}

//===========================================================================
// API: FileExists
// Purpose: Checks for a files existance
// In: LPCTSTR = file to set as check for
// Out: bool = whether found or not
//===========================================================================

API_EXPORT(bool) FileExists(LPCTSTR szFileName)
{
	DWORD a = GetFileAttributes(szFileName);
	return (DWORD)-1 != a && 0 == (a & FILE_ATTRIBUTE_DIRECTORY);
}

//===========================================================================
// API: FileOpen
// Purpose: Opens file for parsing
// In: LPCTSTR = file to open
// Out: FILE* = the file itself
//===========================================================================

API_EXPORT(FILE*) FileOpen(LPCTSTR szPath)
{
	// hack to prevent BBSlit from loading plugins, since they are loaded by the built-in PluginManager.
	FILE *fp; if(_tcscmp(szPath, pluginrc_path)) _tfopen_s(&fp, szPath, _T("rt, ccs=UTF-8")); else fp = NULL;
	return fp;
}

//===========================================================================
// API: FileClose
// Purpose: Close selected file
// In: FILE *fp = The file you wish to close
// Out: bool = Returns the status of completion
//===========================================================================

API_EXPORT(bool) FileClose(FILE *fp)
{
	return fp && 0==fclose(fp);
}

//===========================================================================
// API: FileRead
// Purpose: Read's a line from given FILE and returns boolean on status
// In: FILE *stream = FILE to parse
// In: LPTSTR string = Pointer to returned line from the file
// Out: bool = Status (true if read, false if not)
//===========================================================================

API_EXPORT(bool) FileRead(FILE *fp, LPTSTR buffer)
{
	return read_next_line(fp, buffer, 1024);
}

//===========================================================================
// API: ReadNextCommand
// Purpose: Reads the next line of the file
// In: FILE* = File to be parsed
// In: LPTSTR = buffer to assign next line to
// DWORD = maximum size of line to be outputted
// Out: bool = whether there was a next line to parse
//===========================================================================

API_EXPORT(bool) ReadNextCommand(FILE *fp, LPTSTR szBuffer, DWORD dwLength)
{
	while (read_next_line(fp, szBuffer, dwLength))
	{
		TCHAR c = szBuffer[0];
		if (c && _T('#') != c && _T('!') != c) return true;
	}
	return false;
}

//===========================================================================

bool read_next_line(FILE *fp, LPTSTR szBuffer, DWORD dwLength)
{
	TCHAR *temp;
	TCHAR *p;
	bool r;

	r = false;
	temp = (TCHAR*)calloc(dwLength,sizeof(TCHAR));
	if (fp && _fgetts(temp, dwLength, fp))
	{
		p = temp;
		while(*p)
		{
			if(!IS_SPC(*p))
			{
				_tcsncpy_s(szBuffer, dwLength, p, _TRUNCATE);
				p = _tcschr(szBuffer, _T('\r'));
				if(p)
				{
					*p = 0;
				}
				else
				{
					p = _tcschr(szBuffer, _T('\n'));
					if(p)
					{
						*p = 0;
					}
				}
				r = true;
				break;
			}
			p++;
		}
		free(temp);
		return r;
	}
	else
	{
		szBuffer[0] = 0;
	}
	free(temp);
	return false;

#if 0
	{
		TCHAR *p, *q, c; p = q = szBuffer;
		while (0 != (c = *p) && IS_SPC(c)) p++;
		while (0 != (c = *p)) *q++ = IS_SPC(c) ? _T(' ') : c, p++;
		while (q > szBuffer && IS_SPC(q[-1])) q--;
		*q = 0;
		return true;
	}
	szBuffer[0] = 0;
	return false;
#endif
}

//===========================================================================
// API: ReplaceShellFolders
// Purpose: replace shell folders in a string path, like DESKTOP\...
//===========================================================================

API_EXPORT(LPTSTR) ReplaceShellFolders(LPTSTR string, size_t string_size)
{
	return replace_shellfolders(string, string_size, string, false);
}

//===========================================================================
// API: ReplaceEnvVars
// Purpose: parses a given string and replaces all %VAR% with the environment
//          variable value if such a value exists
// In: LPTSTR = string to replace env vars in
// Out: void = None
//===========================================================================

API_EXPORT(void) ReplaceEnvVars(LPTSTR string, size_t string_size)
{
	TCHAR buffer[4096];
	DWORD r = ExpandEnvironmentStrings(string, buffer, 4096);
	if (r) _tcsncpy_s(string, string_size, buffer, _TRUNCATE);
}

//===========================================================================
// API: GetBlackboxEditor
//===========================================================================

API_EXPORT(void) GetBlackboxEditor(LPTSTR editor, size_t editor_size)
{
	replace_shellfolders(editor, editor_size, Settings_preferredEditor, true);
}

//===========================================================================
// API: FindConfigFile
// Purpose: Look for a config file in the Blackbox, UserAppData and
//          (optionally) plugin directories
// In:  LPTSTR = pszOut = the location where to put the result
// In:  LPCTSTR = filename to look for, LPCTSTR = plugin directory or NULL
// Out: bool = true of found, FALSE otherwise
//===========================================================================

API_EXPORT(bool) FindConfigFile(LPTSTR pszOut, size_t pszOut_size, LPCTSTR filename, LPCTSTR pluginDir)
{
	TCHAR defaultPath[MAX_PATH], *ptr; defaultPath[0] = 0;

	if (is_relative_path(filename))
	{
/*
		// Look for the file in the $UserAppData$\Blackbox directory,
		TCHAR temp[MAX_PATH];
		_stprintf(temp, _T("APPDATA\\Blackbox\\%s"), filename);
		replace_shellfolders(pszOut, temp, false);
		if (FileExists(pszOut)) return true;
*/
		// If pluginDir is specified, we look for the file in this directory...
		if (pluginDir)
		{
			_tcsncpy_s(pszOut, pszOut_size, pluginDir, _TRUNCATE);
			// skip back to the last slash and add filename
			ptr = (TCHAR*)get_file(pszOut); _tcsncpy_s(ptr, pszOut_size - (ptr - pszOut), filename, _TRUNCATE);
			if (FileExists(pszOut)) return true;

			// save default path
			_tcsncpy_s(defaultPath, MAX_PATH, pszOut, _TRUNCATE);
		}
	}

	// Look for the file in the Blackbox directory...
	replace_shellfolders(pszOut, pszOut_size, filename, false);
	if (FileExists(pszOut)) return true;

	// If the plugin path has been set, copy it as default,
	// otherwise keep the Blackbox directory
	if (defaultPath[0]) _tcsncpy_s(pszOut, pszOut_size, defaultPath, _TRUNCATE);

	// does not exist
	return false;
}

//===========================================================================
// API: ConfigFileExists
//===========================================================================

API_EXPORT(LPCTSTR) ConfigFileExists(LPCTSTR filename, LPCTSTR pluginDir)
{
	if (false == FindConfigFile(tempBuf, 1024, filename, pluginDir)) // TODO:排他処理を追加する
		tempBuf[0] = 0;
	return tempBuf;
}

//===========================================================================

LPCTSTR bbPath(LPCTSTR other_name, LPTSTR path, size_t path_size, LPCTSTR name)
{
	if (other_name)
	{
		path[0] = 0;
		if (other_name[0])
			FindConfigFile(path, path_size, other_name, NULL);
	}
	else
		if (0 == path[0])
		{
			TCHAR file_dot_rc[MAX_PATH];
			_sntprintf_s(file_dot_rc, MAX_PATH, _TRUNCATE, _T("%s.rc"), name);
			if (false == FindConfigFile(path, path_size, file_dot_rc, NULL))
			{
				TCHAR file_rc[MAX_PATH];
				_sntprintf_s(file_rc, MAX_PATH, _TRUNCATE, _T("%src"), name);
				if (false == FindConfigFile(path, path_size, file_rc, NULL))
				{
					FindConfigFile(path, path_size, file_dot_rc, NULL);
				}
			}
		}
	//dbg_printf(_T("other <%s>  path <%s>  name <%s>"), other_name, path, name);
	return path;
}

//===========================================================================
// API: bbrcPath
// Purpose: Returns the handle to the blackboxrc file that is being used
// In: LPCTSTR = file to set as default bbrc file (defaults to NULL if not specified)
// Out: LPCTSTR = full path of file
//===========================================================================

API_EXPORT(LPCTSTR) bbrcPath(LPCTSTR other)
{
	return bbPath (other, blackboxrc_path, MAX_PATH, _T("blackbox"));
}

//===========================================================================
// API: extensionsrcPath
// Purpose: Returns the handle to the extensionsrc file that is being used
// In: LPCTSTR = file to set as default extensionsrc file (defaults to NULL if not specified)
// Out: LPCTSTR = full path of file
//===========================================================================

API_EXPORT(LPCTSTR) extensionsrcPath(LPCTSTR other)
{
	return bbPath (other, extensionsrc_path, MAX_PATH, _T("extensions"));
}

//===========================================================================
// API: plugrcPath
// In: LPCTSTR = file to set as default plugins rc file (defaults to NULL if not specified)
// Purpose: Returns the handle to the plugins rc file that is being used
// Out: LPCTSTR = full path of file
//===========================================================================

API_EXPORT(LPCTSTR) plugrcPath(LPCTSTR other)
{
	return bbPath (other, pluginrc_path, MAX_PATH, _T("plugins"));
}

//===========================================================================
// API: menuPath
// Purpose: Returns the handle to the menu file that is being used
// In: LPCTSTR = file to set as default menu file (defaults to NULL if not specified)
// Out: LPCTSTR = full path of file
//===========================================================================

API_EXPORT(LPCTSTR) menuPath(LPCTSTR other)
{
	return bbPath (other, menurc_path, MAX_PATH, _T("menu"));
}

//===========================================================================
// API: stylePath
// Purpose: Returns the handle to the style file that is being used
// In: LPCTSTR = file to set as default style file (defaults to NULL if not specified)
// Out: LPCTSTR = full path of file
//===========================================================================

API_EXPORT(LPCTSTR) stylePath(LPCTSTR other)
{
	return bbPath (other, stylerc_path, MAX_PATH, _T("style"));
}

//===========================================================================
// Function: ParseLiteralColor
// Purpose: Parses a given literal colour and returns the hex value
// In: LPCTSTR = color to parse (eg. _T("black"), _T("white"))
// Out: COLORREF (DWORD) of rgb value
// (old)Out: LPCTSTR = literal hex value
//===========================================================================

struct litcolor1 { TCHAR *cname; COLORREF cref; } litcolor1_ary[] = {

	{ _T("ghostwhite"), RGB(248,248,255) },
	{ _T("whitesmoke"), RGB(245,245,245) },
	{ _T("gainsboro"), RGB(220,220,220) },
	{ _T("floralwhite"), RGB(255,250,240) },
	{ _T("oldlace"), RGB(253,245,230) },
	{ _T("linen"), RGB(250,240,230) },
	{ _T("antiquewhite"), RGB(250,235,215) },
	{ _T("papayawhip"), RGB(255,239,213) },
	{ _T("blanchedalmond"), RGB(255,235,205) },
	{ _T("bisque"), RGB(255,228,196) },
	{ _T("peachpuff"), RGB(255,218,185) },
	{ _T("navajowhite"), RGB(255,222,173) },
	{ _T("moccasin"), RGB(255,228,181) },
	{ _T("cornsilk"), RGB(255,248,220) },
	{ _T("ivory"), RGB(255,255,240) },
	{ _T("lemonchiffon"), RGB(255,250,205) },
	{ _T("seashell"), RGB(255,245,238) },
	{ _T("honeydew"), RGB(240,255,240) },
	{ _T("mintcream"), RGB(245,255,250) },
	{ _T("azure"), RGB(240,255,255) },
	{ _T("aliceblue"), RGB(240,248,255) },
	{ _T("lavender"), RGB(230,230,250) },
	{ _T("lavenderblush"), RGB(255,240,245) },
	{ _T("mistyrose"), RGB(255,228,225) },
	{ _T("white"), RGB(255,255,255) },
	{ _T("black"), RGB(0,0,0) },
	{ _T("darkslategray"), RGB(47,79,79) },
	{ _T("dimgray"), RGB(105,105,105) },
	{ _T("slategray"), RGB(112,128,144) },
	{ _T("lightslategray"), RGB(119,136,153) },
	{ _T("gray"), RGB(190,190,190) },
	{ _T("lightgray"), RGB(211,211,211) },
	{ _T("midnightblue"), RGB(25,25,112) },
	{ _T("navy"), RGB(0,0,128) },
	{ _T("navyblue"), RGB(0,0,128) },
	{ _T("cornflowerblue"), RGB(100,149,237) },
	{ _T("darkslateblue"), RGB(72,61,139) },
	{ _T("slateblue"), RGB(106,90,205) },
	{ _T("mediumslateblue"), RGB(123,104,238) },
	{ _T("lightslateblue"), RGB(132,112,255) },
	{ _T("mediumblue"), RGB(0,0,205) },
	{ _T("royalblue"), RGB(65,105,225) },
	{ _T("blue"), RGB(0,0,255) },
	{ _T("dodgerblue"), RGB(30,144,255) },
	{ _T("deepskyblue"), RGB(0,191,255) },
	{ _T("skyblue"), RGB(135,206,235) },
	{ _T("lightskyblue"), RGB(135,206,250) },
	{ _T("steelblue"), RGB(70,130,180) },
	{ _T("lightsteelblue"), RGB(176,196,222) },
	{ _T("lightblue"), RGB(173,216,230) },
	{ _T("powderblue"), RGB(176,224,230) },
	{ _T("paleturquoise"), RGB(175,238,238) },
	{ _T("darkturquoise"), RGB(0,206,209) },
	{ _T("mediumturquoise"), RGB(72,209,204) },
	{ _T("turquoise"), RGB(64,224,208) },
	{ _T("cyan"), RGB(0,255,255) },
	{ _T("lightcyan"), RGB(224,255,255) },
	{ _T("cadetblue"), RGB(95,158,160) },
	{ _T("mediumaquamarine"), RGB(102,205,170) },
	{ _T("aquamarine"), RGB(127,255,212) },
	{ _T("darkgreen"), RGB(0,100,0) },
	{ _T("darkolivegreen"), RGB(85,107,47) },
	{ _T("darkseagreen"), RGB(143,188,143) },
	{ _T("seagreen"), RGB(46,139,87) },
	{ _T("mediumseagreen"), RGB(60,179,113) },
	{ _T("lightseagreen"), RGB(32,178,170) },
	{ _T("palegreen"), RGB(152,251,152) },
	{ _T("springgreen"), RGB(0,255,127) },
	{ _T("lawngreen"), RGB(124,252,0) },
	{ _T("green"), RGB(0,255,0) },
	{ _T("chartreuse"), RGB(127,255,0) },
	{ _T("mediumspringgreen"), RGB(0,250,154) },
	{ _T("greenyellow"), RGB(173,255,47) },
	{ _T("limegreen"), RGB(50,205,50) },
	{ _T("yellowgreen"), RGB(154,205,50) },
	{ _T("forestgreen"), RGB(34,139,34) },
	{ _T("olivedrab"), RGB(107,142,35) },
	{ _T("darkkhaki"), RGB(189,183,107) },
	{ _T("khaki"), RGB(240,230,140) },
	{ _T("palegoldenrod"), RGB(238,232,170) },
	{ _T("lightgoldenrodyellow"), RGB(250,250,210) },
	{ _T("lightyellow"), RGB(255,255,224) },
	{ _T("yellow"), RGB(255,255,0) },
	{ _T("gold"), RGB(255,215,0) },
	{ _T("lightgoldenrod"), RGB(238,221,130) },
	{ _T("goldenrod"), RGB(218,165,32) },
	{ _T("darkgoldenrod"), RGB(184,134,11) },
	{ _T("rosybrown"), RGB(188,143,143) },
	{ _T("indianred"), RGB(205,92,92) },
	{ _T("saddlebrown"), RGB(139,69,19) },
	{ _T("sienna"), RGB(160,82,45) },
	{ _T("peru"), RGB(205,133,63) },
	{ _T("burlywood"), RGB(222,184,135) },
	{ _T("beige"), RGB(245,245,220) },
	{ _T("wheat"), RGB(245,222,179) },
	{ _T("sandybrown"), RGB(244,164,96) },
	{ _T("tan"), RGB(210,180,140) },
	{ _T("chocolate"), RGB(210,105,30) },
	{ _T("firebrick"), RGB(178,34,34) },
	{ _T("brown"), RGB(165,42,42) },
	{ _T("darksalmon"), RGB(233,150,122) },
	{ _T("salmon"), RGB(250,128,114) },
	{ _T("lightsalmon"), RGB(255,160,122) },
	{ _T("orange"), RGB(255,165,0) },
	{ _T("darkorange"), RGB(255,140,0) },
	{ _T("coral"), RGB(255,127,80) },
	{ _T("lightcoral"), RGB(240,128,128) },
	{ _T("tomato"), RGB(255,99,71) },
	{ _T("orangered"), RGB(255,69,0) },
	{ _T("red"), RGB(255,0,0) },
	{ _T("hotpink"), RGB(255,105,180) },
	{ _T("deeppink"), RGB(255,20,147) },
	{ _T("pink"), RGB(255,192,203) },
	{ _T("lightpink"), RGB(255,182,193) },
	{ _T("palevioletred"), RGB(219,112,147) },
	{ _T("maroon"), RGB(176,48,96) },
	{ _T("mediumvioletred"), RGB(199,21,133) },
	{ _T("violetred"), RGB(208,32,144) },
	{ _T("magenta"), RGB(255,0,255) },
	{ _T("violet"), RGB(238,130,238) },
	{ _T("plum"), RGB(221,160,221) },
	{ _T("orchid"), RGB(218,112,214) },
	{ _T("mediumorchid"), RGB(186,85,211) },
	{ _T("darkorchid"), RGB(153,50,204) },
	{ _T("darkviolet"), RGB(148,0,211) },
	{ _T("blueviolet"), RGB(138,43,226) },
	{ _T("purple"), RGB(160,32,240) },
	{ _T("mediumpurple"), RGB(147,112,219) },
	{ _T("thistle"), RGB(216,191,216) },

	{ _T("darkgray"), RGB(169,169,169) },
	{ _T("darkblue"), RGB(0,0,139) },
	{ _T("darkcyan"), RGB(0,139,139) },
	{ _T("darkmagenta"), RGB(139,0,139) },
	{ _T("darkred"), RGB(139,0,0) },
	{ _T("lightgreen"), RGB(144,238,144) }
};

struct litcolor4 { TCHAR *cname; COLORREF cref[4]; } litcolor4_ary[] = {

	{ _T("snow"), { RGB(255,250,250), RGB(238,233,233), RGB(205,201,201), RGB(139,137,137) }},
	{ _T("seashell"), { RGB(255,245,238), RGB(238,229,222), RGB(205,197,191), RGB(139,134,130) }},
	{ _T("antiquewhite"), { RGB(255,239,219), RGB(238,223,204), RGB(205,192,176), RGB(139,131,120) }},
	{ _T("bisque"), { RGB(255,228,196), RGB(238,213,183), RGB(205,183,158), RGB(139,125,107) }},
	{ _T("peachpuff"), { RGB(255,218,185), RGB(238,203,173), RGB(205,175,149), RGB(139,119,101) }},
	{ _T("navajowhite"), { RGB(255,222,173), RGB(238,207,161), RGB(205,179,139), RGB(139,121,94) }},
	{ _T("lemonchiffon"), { RGB(255,250,205), RGB(238,233,191), RGB(205,201,165), RGB(139,137,112) }},
	{ _T("cornsilk"), { RGB(255,248,220), RGB(238,232,205), RGB(205,200,177), RGB(139,136,120) }},
	{ _T("ivory"), { RGB(255,255,240), RGB(238,238,224), RGB(205,205,193), RGB(139,139,131) }},
	{ _T("honeydew"), { RGB(240,255,240), RGB(224,238,224), RGB(193,205,193), RGB(131,139,131) }},
	{ _T("lavenderblush"), { RGB(255,240,245), RGB(238,224,229), RGB(205,193,197), RGB(139,131,134) }},
	{ _T("mistyrose"), { RGB(255,228,225), RGB(238,213,210), RGB(205,183,181), RGB(139,125,123) }},
	{ _T("azure"), { RGB(240,255,255), RGB(224,238,238), RGB(193,205,205), RGB(131,139,139) }},
	{ _T("slateblue"), { RGB(131,111,255), RGB(122,103,238), RGB(105,89,205), RGB(71,60,139) }},
	{ _T("royalblue"), { RGB(72,118,255), RGB(67,110,238), RGB(58,95,205), RGB(39,64,139) }},
	{ _T("blue"), { RGB(0,0,255), RGB(0,0,238), RGB(0,0,205), RGB(0,0,139) }},
	{ _T("dodgerblue"), { RGB(30,144,255), RGB(28,134,238), RGB(24,116,205), RGB(16,78,139) }},
	{ _T("steelblue"), { RGB(99,184,255), RGB(92,172,238), RGB(79,148,205), RGB(54,100,139) }},
	{ _T("deepskyblue"), { RGB(0,191,255), RGB(0,178,238), RGB(0,154,205), RGB(0,104,139) }},
	{ _T("skyblue"), { RGB(135,206,255), RGB(126,192,238), RGB(108,166,205), RGB(74,112,139) }},
	{ _T("lightskyblue"), { RGB(176,226,255), RGB(164,211,238), RGB(141,182,205), RGB(96,123,139) }},
	{ _T("slategray"), { RGB(198,226,255), RGB(185,211,238), RGB(159,182,205), RGB(108,123,139) }},
	{ _T("lightsteelblue"), { RGB(202,225,255), RGB(188,210,238), RGB(162,181,205), RGB(110,123,139) }},
	{ _T("lightblue"), { RGB(191,239,255), RGB(178,223,238), RGB(154,192,205), RGB(104,131,139) }},
	{ _T("lightcyan"), { RGB(224,255,255), RGB(209,238,238), RGB(180,205,205), RGB(122,139,139) }},
	{ _T("paleturquoise"), { RGB(187,255,255), RGB(174,238,238), RGB(150,205,205), RGB(102,139,139) }},
	{ _T("cadetblue"), { RGB(152,245,255), RGB(142,229,238), RGB(122,197,205), RGB(83,134,139) }},
	{ _T("turquoise"), { RGB(0,245,255), RGB(0,229,238), RGB(0,197,205), RGB(0,134,139) }},
	{ _T("cyan"), { RGB(0,255,255), RGB(0,238,238), RGB(0,205,205), RGB(0,139,139) }},
	{ _T("darkslategray"), { RGB(151,255,255), RGB(141,238,238), RGB(121,205,205), RGB(82,139,139) }},
	{ _T("aquamarine"), { RGB(127,255,212), RGB(118,238,198), RGB(102,205,170), RGB(69,139,116) }},
	{ _T("darkseagreen"), { RGB(193,255,193), RGB(180,238,180), RGB(155,205,155), RGB(105,139,105) }},
	{ _T("seagreen"), { RGB(84,255,159), RGB(78,238,148), RGB(67,205,128), RGB(46,139,87) }},
	{ _T("palegreen"), { RGB(154,255,154), RGB(144,238,144), RGB(124,205,124), RGB(84,139,84) }},
	{ _T("springgreen"), { RGB(0,255,127), RGB(0,238,118), RGB(0,205,102), RGB(0,139,69) }},
	{ _T("green"), { RGB(0,255,0), RGB(0,238,0), RGB(0,205,0), RGB(0,139,0) }},
	{ _T("chartreuse"), { RGB(127,255,0), RGB(118,238,0), RGB(102,205,0), RGB(69,139,0) }},
	{ _T("olivedrab"), { RGB(192,255,62), RGB(179,238,58), RGB(154,205,50), RGB(105,139,34) }},
	{ _T("darkolivegreen"), { RGB(202,255,112), RGB(188,238,104), RGB(162,205,90), RGB(110,139,61) }},
	{ _T("khaki"), { RGB(255,246,143), RGB(238,230,133), RGB(205,198,115), RGB(139,134,78) }},
	{ _T("lightgoldenrod"), { RGB(255,236,139), RGB(238,220,130), RGB(205,190,112), RGB(139,129,76) }},
	{ _T("lightyellow"), { RGB(255,255,224), RGB(238,238,209), RGB(205,205,180), RGB(139,139,122) }},
	{ _T("yellow"), { RGB(255,255,0), RGB(238,238,0), RGB(205,205,0), RGB(139,139,0) }},
	{ _T("gold"), { RGB(255,215,0), RGB(238,201,0), RGB(205,173,0), RGB(139,117,0) }},
	{ _T("goldenrod"), { RGB(255,193,37), RGB(238,180,34), RGB(205,155,29), RGB(139,105,20) }},
	{ _T("darkgoldenrod"), { RGB(255,185,15), RGB(238,173,14), RGB(205,149,12), RGB(139,101,8) }},
	{ _T("rosybrown"), { RGB(255,193,193), RGB(238,180,180), RGB(205,155,155), RGB(139,105,105) }},
	{ _T("indianred"), { RGB(255,106,106), RGB(238,99,99), RGB(205,85,85), RGB(139,58,58) }},
	{ _T("sienna"), { RGB(255,130,71), RGB(238,121,66), RGB(205,104,57), RGB(139,71,38) }},
	{ _T("burlywood"), { RGB(255,211,155), RGB(238,197,145), RGB(205,170,125), RGB(139,115,85) }},
	{ _T("wheat"), { RGB(255,231,186), RGB(238,216,174), RGB(205,186,150), RGB(139,126,102) }},
	{ _T("tan"), { RGB(255,165,79), RGB(238,154,73), RGB(205,133,63), RGB(139,90,43) }},
	{ _T("chocolate"), { RGB(255,127,36), RGB(238,118,33), RGB(205,102,29), RGB(139,69,19) }},
	{ _T("firebrick"), { RGB(255,48,48), RGB(238,44,44), RGB(205,38,38), RGB(139,26,26) }},
	{ _T("brown"), { RGB(255,64,64), RGB(238,59,59), RGB(205,51,51), RGB(139,35,35) }},
	{ _T("salmon"), { RGB(255,140,105), RGB(238,130,98), RGB(205,112,84), RGB(139,76,57) }},
	{ _T("lightsalmon"), { RGB(255,160,122), RGB(238,149,114), RGB(205,129,98), RGB(139,87,66) }},
	{ _T("orange"), { RGB(255,165,0), RGB(238,154,0), RGB(205,133,0), RGB(139,90,0) }},
	{ _T("darkorange"), { RGB(255,127,0), RGB(238,118,0), RGB(205,102,0), RGB(139,69,0) }},
	{ _T("coral"), { RGB(255,114,86), RGB(238,106,80), RGB(205,91,69), RGB(139,62,47) }},
	{ _T("tomato"), { RGB(255,99,71), RGB(238,92,66), RGB(205,79,57), RGB(139,54,38) }},
	{ _T("orangered"), { RGB(255,69,0), RGB(238,64,0), RGB(205,55,0), RGB(139,37,0) }},
	{ _T("red"), { RGB(255,0,0), RGB(238,0,0), RGB(205,0,0), RGB(139,0,0) }},
	{ _T("deeppink"), { RGB(255,20,147), RGB(238,18,137), RGB(205,16,118), RGB(139,10,80) }},
	{ _T("hotpink"), { RGB(255,110,180), RGB(238,106,167), RGB(205,96,144), RGB(139,58,98) }},
	{ _T("pink"), { RGB(255,181,197), RGB(238,169,184), RGB(205,145,158), RGB(139,99,108) }},
	{ _T("lightpink"), { RGB(255,174,185), RGB(238,162,173), RGB(205,140,149), RGB(139,95,101) }},
	{ _T("palevioletred"), { RGB(255,130,171), RGB(238,121,159), RGB(205,104,137), RGB(139,71,93) }},
	{ _T("maroon"), { RGB(255,52,179), RGB(238,48,167), RGB(205,41,144), RGB(139,28,98) }},
	{ _T("violetred"), { RGB(255,62,150), RGB(238,58,140), RGB(205,50,120), RGB(139,34,82) }},
	{ _T("magenta"), { RGB(255,0,255), RGB(238,0,238), RGB(205,0,205), RGB(139,0,139) }},
	{ _T("orchid"), { RGB(255,131,250), RGB(238,122,233), RGB(205,105,201), RGB(139,71,137) }},
	{ _T("plum"), { RGB(255,187,255), RGB(238,174,238), RGB(205,150,205), RGB(139,102,139) }},
	{ _T("mediumorchid"), { RGB(224,102,255), RGB(209,95,238), RGB(180,82,205), RGB(122,55,139) }},
	{ _T("darkorchid"), { RGB(191,62,255), RGB(178,58,238), RGB(154,50,205), RGB(104,34,139) }},
	{ _T("purple"), { RGB(155,48,255), RGB(145,44,238), RGB(125,38,205), RGB(85,26,139) }},
	{ _T("mediumpurple"), { RGB(171,130,255), RGB(159,121,238), RGB(137,104,205), RGB(93,71,139) }},
	{ _T("thistle"), { RGB(255,225,255), RGB(238,210,238), RGB(205,181,205), RGB(139,123,139) }}
};

COLORREF ParseLiteralColor(LPCTSTR colour)
{
	int i, n; size_t l; TCHAR *p, *pd, c, buf[32];
	l = _tcslen(colour) + 1;
	if (l > 2 && l < 32)
	{
		_tmemcpy(buf, colour, l); //_tcslwr(buf);
		while (NULL!=(p=_tcschr(buf,_T(' '))))
		{
			pd = _tcsdup(p+1);
			_tcsncpy_s(p, 32, pd, _TRUNCATE);
			free(pd);
		}
		if (NULL!=(p=_tcsstr(buf,_T("grey")))) p[2]=_T('a');
		if (0==_tmemcmp(buf,_T("gray"), 4) && (c=buf[4]) >= _T('0') && c <= _T('9'))
		{
			i = _ttoi(buf+4);
			if (i >= 0 && i <= 100)
			{
				i = (i * 255 + 50) / 100;
				return rgb(i,i,i);
			}
		}
		i = *(p = &buf[l-2]) - _T('1');
		if (i>=0 && i<4)
		{
			*p=0; --l;
			struct litcolor4 *cp4=litcolor4_ary;
			n = sizeof(litcolor4_ary) / sizeof(*cp4);
			do { if (0==_tmemcmp(buf, cp4->cname, l)) return cp4->cref[i]; cp4++; }
			while (--n);
		}
		else
		{
			struct litcolor1 *cp1=litcolor1_ary;
			n = sizeof(litcolor1_ary) / sizeof(*cp1);
			do { if (0==_tmemcmp(buf, cp1->cname, l)) return cp1->cref; cp1++; }
			while (--n);
		}
	}
	return (COLORREF)-1;
}

//===========================================================================
// Function: ReadColorFromString
// Purpose: parse a literal or hexadezimal color string


COLORREF ReadColorFromString(LPCTSTR string)
{
	if (NULL == string) return (COLORREF)-1;
	TCHAR stub[256], rgbstr[7], *s;
	s = unquote(stub, 256, string); _tcslwr_s(s, 256);
	if (_T('#')==*s) s++;
	for (;;)
	{
		COLORREF cr = 0; TCHAR *d, c;
		// check if its a valid hex number
		for (d = s; (c = *d) != 0; ++d)
		{
			cr <<= 4;
			if (c >= _T('0') && c <= _T('9')) cr |= c - _T('0');
			else
				if (c >= _T('a') && c <= _T('f')) cr |= c - (_T('a')-10);
				else goto check_rgb;
		}

		if (d - s == 3) // #AB4 short type colors
			cr = (cr&0xF00)<<12 | (cr&0xFF0)<<8 | (cr&0x0FF)<<4 | cr&0x00F;

		return switch_rgb(cr);

	check_rgb:
		// check if its an "rgb:12/ee/4c" type string
		s = stub;
		if (0 == _tmemcmp(s, _T("rgb:"), 4))
		{
			int j=3; s+=4; d = rgbstr;
			do {
				d[0] = *s && _T('/')!=*s ? *s++ : _T('0');
				d[1] = *s && _T('/')!=*s ? *s++ : d[0];
				d+=2; if (_T('/')==*s) ++s;
			} while (--j);
			*d=0; s = rgbstr;
			continue;
		}

		// must be one of the literal color names (or is invalid)
		return ParseLiteralColor(s);
	}
}

//===========================================================================
// API: Log
// Purpose: Appends given line to Blackbox.log file
//===========================================================================

API_EXPORT(void) Log(LPCTSTR Title, LPCTSTR Line)
{
	//log_printf(1, _T("%s: %s"), Title, Line);
}

//===========================================================================
// API: MBoxErrorFile
// Purpose: Gives a message box proclaming missing file
// In: LPCTSTR = missing file
// Out: int = return value of messagebox
//===========================================================================

API_EXPORT(int) MBoxErrorFile(LPCTSTR szFile)
{
	return BBMessageBox(MB_OK, NLS2(_T("$BBError_ReadFile$"),
									_T("Error: Unable to open file \"%s\".")
									_T("\nPlease check location and try again.")
									), szFile);
}

//===========================================================================
// API: MBoxErrorValue
// Purpose: Gives a message box proclaming a given value
// In: LPCTSTR = value
// Out: int = return value of messagebox
//===========================================================================

API_EXPORT(int) MBoxErrorValue(LPCTSTR szValue)
{
	return BBMessageBox(MB_OK, NLS2(_T("$BBError_MsgBox$"),
									_T("Error: %s")), szValue);
}

//===========================================================================
// API: BBExecute
// Purpose: A safe execute routine
// In: HWND = owner
// In: LPCTSTR = operation (eg. "open")
// In: LPCTSTR = command to run
// In: LPCTSTR = arguments
// In: LPCTSTR = directory to run from
// In: int = show status
// In: bool = suppress error messages
// //Out: HINSTANCE = instance of file running
//===========================================================================

API_EXPORT(BOOL) BBExecute(
    HWND Owner,
    LPCTSTR szOperation,
    LPCTSTR szCommand, LPCTSTR szArgs, LPCTSTR szDirectory,
    int nShowCmd, bool noErrorMsgs)
{
	if (szCommand[0])
	{
		SHELLEXECUTEINFO sei; ZeroMemory(&sei, sizeof(sei));
		sei.cbSize       = sizeof(sei);
		sei.hwnd         = Owner;
		sei.lpVerb       = szOperation;
		sei.lpFile       = szCommand;
		sei.lpParameters = szArgs;
		sei.lpDirectory  = szDirectory;
		sei.nShow        = nShowCmd;
		sei.fMask        = SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI;

		if (ShellExecuteEx(&sei))
			return TRUE;
	}
	if (!noErrorMsgs)
		BBMessageBox(MB_OK, NLS2(_T("$BBError_Execute$"),
								 _T("Error: Could not execute:")
								 _T("\nCommand:  \t%s")
								 _T("\nOperation:\t%s")
								 _T("\nArguments:\t%s")
								 _T("\nWorking Directory:\t%s")),
					 string_empty_or_null(szCommand),
					 string_empty_or_null(szOperation),
					 string_empty_or_null(szArgs),
					 string_empty_or_null(szDirectory)
					 );

	return FALSE;
}

//===========================================================================

BOOL BBExecute_command(const TCHAR *command, const TCHAR *arguments, bool no_errors)
{
	TCHAR parsed_command[MAX_PATH];
	replace_shellfolders(parsed_command, MAX_PATH, command, true);
	TCHAR workdir[MAX_PATH];
	get_directory(workdir, MAX_PATH, parsed_command);
	return BBExecute(NULL, NULL, parsed_command, arguments, workdir, SW_SHOWNORMAL, no_errors);
}

BOOL BBExecute_string(const TCHAR *command, bool no_errors)
{
	TCHAR path[MAX_PATH];
	NextToken(path, MAX_PATH, &command);
	return BBExecute_command(path, command, no_errors);
}

bool ShellCommand(const TCHAR *command, const TCHAR *work_dir, bool wait)
{
	SHELLEXECUTEINFO sei; TCHAR path[MAX_PATH]; BOOL r;
	NextToken(path, MAX_PATH, &command);
	ZeroMemory(&sei, sizeof sei);
	sei.cbSize = sizeof sei;
	sei.fMask = SEE_MASK_DOENVSUBST;
	if (wait) sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
	sei.lpFile = path;
	sei.lpParameters = command;
	sei.lpDirectory = work_dir;
	sei.nShow = SW_SHOWNORMAL;
	r = ShellExecuteEx(&sei);
	if (r && wait)
	{
		WaitForSingleObject(sei.hProcess, INFINITE);
		CloseHandle(sei.hProcess);
	}
	return (r != 0);
}

//===========================================================================
// API: IsAppWindow
// Purpose: checks given hwnd to see if it's an app
// In: HWND = hwnd to check
// Out: bool = if app or not
//===========================================================================
// This is used to populate the task list in case bb is started manually.

API_EXPORT(bool) IsAppWindow(HWND hwnd)
{
	LONG_PTR nStyle, nExStyle;
	HWND hOwner; HWND hParent;

	if (!IsWindow(hwnd))
		return false;

	if (CheckSticky(hwnd))
		return false;

	nStyle = GetWindowLongPtr(hwnd, GWL_STYLE);

	// if it is a WS_CHILD or not WS_VISIBLE, fail it
	if((nStyle & WS_CHILD) || !(nStyle & WS_VISIBLE) || (nStyle & WS_DISABLED))
		return false;

	nExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	// if the window is a WS_EX_TOOLWINDOW fail it
	if((nExStyle & WS_EX_TOOLWINDOW) && !(nExStyle & WS_EX_APPWINDOW))
		return false;

	// If this has a parent, then only accept this window
	// if the parent is not accepted
	hParent = GetParent(hwnd);
	if (hParent && IsAppWindow(hParent))
		return false;

	// If this has an owner, then only accept this window
	// if the parent is not accepted
	hOwner = GetWindow(hwnd, GW_OWNER);
	if(hOwner && IsAppWindow(hOwner))
		return false;

	return true;
}

//===========================================================================
// API: MakeStyleGradient
// Purpose:  Make a gradient from style Item
//===========================================================================

API_EXPORT(void) MakeStyleGradient(HDC hdc, RECT *rp, StyleItem * pSI, bool withBorder)
{
	COLORREF borderColor; int borderWidth;
	if (withBorder)
	{
		if (pSI->bordered)
		{
			borderColor = pSI->borderColor;
			borderWidth = pSI->borderWidth;
		}
		else
		{
			borderColor = mStyle.borderColor;
			borderWidth = mStyle.borderWidth;
		}
	}
	else
	{
		borderColor = 0;
		borderWidth = 0;
	}

	MakeGradientEx(hdc, *rp,
				   pSI->parentRelative ? -1 : pSI->type,
				   pSI->Color,
				   pSI->ColorTo,
				   pSI->ColorSplitTo,
				   pSI->ColorToSplitTo,
				   pSI->interlaced,
				   pSI->bevelstyle,
				   pSI->bevelposition,
				   0,
				   borderColor,
				   borderWidth
				   );
}

//===========================================================================
// API: ParseItem
// Purpose: parses a given string and assigns settings to a StyleItem class
// In: LPCTSTR = item to parse out
// In: StyleItem* = class to assign values to
// Out: void = None
//===========================================================================

struct styleprop { TCHAR *key; int  val; };

struct styleprop styleprop_1[] = {
	{_T("flat")        ,BEVEL_FLAT           },
	{_T("raised")      ,BEVEL_RAISED         },
	{_T("sunken")      ,BEVEL_SUNKEN         },
	{NULL          ,BEVEL_RAISED         }
};

struct styleprop styleprop_2[] = {
	{_T("bevel1")      ,BEVEL1         },
	{_T("bevel2")      ,BEVEL2         },
	{_T("bevel3")      ,3              },
	{NULL          ,BEVEL1         }
};

struct styleprop styleprop_3[] = {
	{_T("solid")           ,B_SOLID            },
	{_T("splithorizontal") ,B_SPLITHORIZONTAL  }, // _T("horizontal") is match .*horizontal
	{_T("mirrorhorizontal"),B_MIRRORHORIZONTAL },
	{_T("horizontal")      ,B_HORIZONTAL       },
	{_T("splitvertical")   ,B_SPLITVERTICAL    }, // _T("vertical") is match .*vertical
	{_T("mirrorvertical")  ,B_MIRRORVERTICAL   },
	{_T("vertical")        ,B_VERTICAL         },
	{_T("crossdiagonal")   ,B_CROSSDIAGONAL    },
	{_T("diagonal")        ,B_DIAGONAL         },
	{_T("pipecross")       ,B_PIPECROSS        },
	{_T("elliptic")        ,B_ELLIPTIC         },
	{_T("rectangle")       ,B_RECTANGLE        },
	{_T("pyramid")         ,B_PYRAMID          },
	{NULL              ,-1                 }
};

int check_item(const TCHAR *p, struct styleprop *s)
{
	do if (_tcsstr(p, s->key)) break; while ((++s)->key);
	return s->val;
}

int ParseType(TCHAR *buf)
{
	return check_item(buf, styleprop_3);
}

API_EXPORT(void) ParseItem(LPCTSTR szItem, StyleItem *item)
{
	TCHAR buf[256]; int t;
	_tcsncpy_s(buf, 256, szItem, _TRUNCATE); _tcslwr_s(buf, 256);
	item->bevelstyle = check_item(buf, styleprop_1);
	item->bevelposition = BEVEL_FLAT == item->bevelstyle ? 0 : check_item(buf, styleprop_2);
	t = check_item(buf, styleprop_3);
	item->type = (-1 != t) ? t : B_SOLID;
	item->interlaced = NULL!=_tcsstr(buf, _T("interlaced"));
	item->parentRelative = NULL!=_tcsstr(buf, _T("parentrelative"));
	//if (item->nVersion >= 2) item->bordered = NULL!=_tcsstr(buf, _T("border"));
}

//===========================================================================
// API: MakeSticky
// API: RemoveSticky
// API: CheckSticky
// Purpose:  make a plugin-window appear on all workspaces
//===========================================================================

struct hwnd_list * stickyhwl;

API_EXPORT(void) MakeSticky(HWND window)
{
	if (window && NULL == assoc(stickyhwl, window))
		cons_node(&stickyhwl, new_node(window));
	//dbg_window(window, _T("makesticky"));
}

API_EXPORT(void) RemoveSticky(HWND window)
{
	delete_assoc(&stickyhwl, window);
	//dbg_window(window, _T("removesticky"));
}

API_EXPORT(bool) CheckSticky(HWND window)
{
	if (assoc(stickyhwl, window))
		return true;

	// for bbPager
	if (GetWindowLongPtr(window, GWLP_USERDATA) == 0x49474541 /*magicDWord*/)
		return true;

	return false;
}

//================================
// non api
void ClearSticky()
{
	freeall(&stickyhwl);
}

//===========================================================================
// API: GetUnderExplorer
//===========================================================================

API_EXPORT(bool) GetUnderExplorer()
{
	return underExplorer;
}

//===========================================================================
// API: SetTransparency
// Purpose: Wrapper, win9x conpatible
// In:      HWND, alpha
// Out:     bool
//===========================================================================

BOOL (WINAPI *pSetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);

API_EXPORT(bool) SetTransparency(HWND hwnd, BYTE alpha)
{
	//dbg_window(hwnd, _T("alpha %d"), alpha);
	if (NULL == pSetLayeredWindowAttributes) return false;

	LONG_PTR wStyle1, wStyle2;
	wStyle1 = wStyle2 = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

	if (alpha < 255) wStyle2 |= WS_EX_LAYERED;
	else wStyle2 &= ~WS_EX_LAYERED;

	if (wStyle2 != wStyle1)
		SetWindowLongPtr(hwnd, GWL_EXSTYLE, wStyle2);

	if (wStyle2 & WS_EX_LAYERED)
		return 0 != pSetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);

	return true;
}

//*****************************************************************************

//*****************************************************************************
// multimon api, win 9x compatible

HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
HMONITOR (WINAPI *pMonitorFromPoint)(POINT pt, DWORD dwFlags);
BOOL     (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);
BOOL     (WINAPI* pEnumDisplayMonitors)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);

void get_mon_rect(HMONITOR hMon, RECT *s, RECT *w)
{
	if (hMon)
	{
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		if (pGetMonitorInfoA(hMon, &mi))
		{
			if (w) *w = mi.rcWork;
			if (s) *s = mi.rcMonitor;
			return;
		}
	}

	if (w)
	{
		SystemParametersInfo(SPI_GETWORKAREA, 0, w, 0);
	}

	if (s)
	{
		s->top = s->left = 0;
		s->right = GetSystemMetrics(SM_CXSCREEN);
		s->bottom = GetSystemMetrics(SM_CYSCREEN);
	}
}

//===========================================================================
// API: GetMonitorRect
//===========================================================================

API_EXPORT(HMONITOR) GetMonitorRect(void *from, RECT *r, int flags)
{
	HMONITOR hMon = NULL;
	if (multimon && from)
	{
		if (flags & GETMON_FROM_WINDOW)
			hMon = pMonitorFromWindow((HWND)from, MONITOR_DEFAULTTONEAREST);
		else
			if (flags & GETMON_FROM_POINT)
				hMon = pMonitorFromPoint(*(POINT*)from, MONITOR_DEFAULTTONEAREST);
			else
				if (flags & GETMON_FROM_MONITOR)
					hMon = (HMONITOR)from;
	}

	if (flags & GETMON_WORKAREA)
		get_mon_rect(hMon, NULL, r);
	else
		get_mon_rect(hMon, r, NULL);

	return hMon;
}

//===========================================================================
// API: SetDesktopMargin
// Purpose:  Set a margin for e.g. toolbar, bbsystembar, etc
// In:       hwnd to associate the margin with, location, margin-width
// Out:      void
//===========================================================================

struct dt_margins
{
	struct dt_margins *next;
	HWND hwnd;
	HMONITOR hmon;
	int location;
	int margin;
};

void update_screen_areas(struct dt_margins *dt_margins);

API_EXPORT(void) SetDesktopMargin(HWND hwnd, int location, int margin)
{
	//dbg_printf(_T("SDTM: %08x %d %d"), hwnd, location, margin);

	static struct dt_margins *margin_list;

	if (BB_DM_RESET == location)
		freeall(&margin_list); // reset everything
	else
		if (BB_DM_REFRESH == location)
			; // do nothing
		else
			if (hwnd)
			{
				// search for hwnd:
				struct dt_margins *p = (struct dt_margins *)assoc(margin_list, hwnd);
				if (margin)
				{
					if (NULL == p) // insert a _new structure
					{
						p = (struct dt_margins *)c_alloc(sizeof(struct dt_margins));
						cons_node (&margin_list, p);
						p->hwnd = hwnd;
					}
					p->location = location;
					p->margin = margin;
					if (multimon) p->hmon = pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
				}
				else
					if (p)
					{
						remove_item(&margin_list, p);
					}
			}
	update_screen_areas(margin_list);
}

//===========================================================================

struct _screen
{
	struct _screen *next;
	HMONITOR hMon;
	int index;
	RECT screen_rect;
	RECT work_rect;
	RECT new_rect;
	RECT custom_margins;
};

struct _screen_list
{
	struct _screen *pScr;
	struct _screen **ppScr;
	int index;
};

void get_custom_margin(RECT *pcm, int screen)
{
	TCHAR key[80]; int i = 0, x;
	static const TCHAR *edges[4] = { _T("Left:"), _T("Top:"), _T("Right:"), _T("Bottom:") };
	if (0 == screen) x = _sntprintf_s(key, 80, _TRUNCATE, _T("blackbox.desktop.margin"));
	else x = _sntprintf_s(key, 80, _TRUNCATE, _T("blackbox.desktop.%d.margin"), 1+screen);
	do _tcsncpy_s(key+x, 80-x, edges[i], _TRUNCATE), (&pcm->left)[i] = ReadInt(extensionsrcPath(), key, -1);
	while (++i < 4);
}

BOOL CALLBACK fnEnumMonProc(HMONITOR hMon, HDC hdcOptional, RECT *prcLimit, LPARAM dwData)
{
	//dbg_printf(_T("EnumProc %08x"), hMon);
	struct _screen * s = (struct _screen *)c_alloc (sizeof *s);
	s->hMon = hMon;
	get_mon_rect(hMon, &s->screen_rect, &s->work_rect);
	s->new_rect = s->screen_rect;

	struct _screen_list *i = (struct _screen_list *)dwData;
	*i->ppScr = s;
	i->ppScr = &s->next;

	int screen = s->index = i->index++;
	if (0 == screen) s->custom_margins = Settings_desktopMargin;
	else get_custom_margin(&s->custom_margins, screen);

	return TRUE;
}

void update_screen_areas(struct dt_margins *dt_margins)
{
	struct _screen_list si = { NULL, &si.pScr, 0 };

	if (multimon)
		pEnumDisplayMonitors(NULL, NULL, fnEnumMonProc, (LPARAM)&si);
	else
		fnEnumMonProc(NULL, NULL, NULL, (LPARAM)&si);

	//dbg_printf(_T("list: %d %d"), listlen(si.pScr), si.index);

	struct _screen *pS;

	if (false == Settings_fullMaximization)
	{
		struct dt_margins *p;
		dolist (p, dt_margins) // loop through margins
		{
			pS = (struct _screen *)assoc(si.pScr, p->hmon); // get screen for this window
			//dbg_printf(_T("assoc: %x"), ppScr);
			if (pS)
			{
				RECT *n = &pS->new_rect;
				RECT *s = &pS->screen_rect;
				switch (p->location)
				{
				case BB_DM_LEFT   : n->left     = (LONG)imax(n->left  , s->left   + p->margin); break;
				case BB_DM_TOP    : n->top      = (LONG)imax(n->top   , s->top    + p->margin); break;
				case BB_DM_RIGHT  : n->right    = (LONG)imin(n->right , s->right  - p->margin); break;
				case BB_DM_BOTTOM : n->bottom   = (LONG)imin(n->bottom, s->bottom - p->margin); break;
				}
			}
		}

		dolist (pS, si.pScr)
		{
			RECT *n = &pS->new_rect;
			RECT *s = &pS->screen_rect;
			RECT *m = &pS->custom_margins;
			if (-1 != m->left)     n->left     = s->left     + m->left    ;
			if (-1 != m->top)      n->top      = s->top      + m->top     ;
			if (-1 != m->right)    n->right    = s->right    - m->right   ;
			if (-1 != m->bottom)   n->bottom   = s->bottom   - m->bottom  ;
		}
	}

	dolist (pS, si.pScr)
	{
		RECT *n = &pS->new_rect;
		if (0 != memcmp(&pS->work_rect, n, sizeof(RECT)))
		{
			//dbg_printf(_T("HW = %08x  WA = %d %d %d %d"), hwnd, n->left, n->top, n->right, n->bottom);
			SystemParametersInfo(SPI_SETWORKAREA, 0, (PVOID)n, SPIF_SENDCHANGE);
		}
	}

	freeall(&si.pScr);
}

//===========================================================================

//===========================================================================
// API: SnapWindowToEdge
// Purpose:Snaps a given windowpos at a specified distance
// In: WINDOWPOS* = WINDOWPOS recieved from WM_WINDOWPOSCHANGING
// In: int = distance to snap to
// In: bool = use screensize of workspace
// Out: void = none
//===========================================================================

// Structures
struct edges { int from1, from2, to1, to2, dmin, omin, d, o, def; };

struct snap_info { struct edges *h; struct edges *v;
    bool sizing; bool same_level; int pad; HWND self; HWND parent; };

// Local fuctions
//void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad);
void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad);
BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam);


API_EXPORT(void) SnapWindowToEdge(WINDOWPOS* wp, LPARAM nDist_or_pContent, UINT Flags)
{
	int snapdist    = Settings_edgeSnapThreshold;
	int padding     = Settings_edgeSnapPadding;
	//int grid        = 0;

	if (snapdist < 1) return;

	HWND self = wp->hwnd;

	// well, why is this here? Because some plugins call this even if
	// they reposition themselves rather than being moved by the user.
	static DWORD snap_tick; DWORD ticknow = GetTickCount();
	if (GetCapture() != self)
	{
		if (snap_tick < ticknow) return;
	}
	else
	{
		snap_tick = ticknow + 100;
	}

	HWND parent = (WS_CHILD & GetWindowLongPtr(self, GWL_STYLE)) ? GetParent(self) : NULL;

	//RECT r; GetWindowRect(self, &r);
	//bool sizing         = wp->cx != r.right-r.left || wp->cy != r.bottom-r.top;

	bool sizing         = 0 != (Flags & SNAP_SIZING);
	bool snap_workarea  = 0 == (Flags & SNAP_FULLSCREEN);
	bool snap_plugins   = 0 == (Flags & SNAP_NOPLUGINS) && Settings_edgeSnapPlugins;
	//dbg_printf(_T("%x %x %d %d %d %d Flags: %x siz %d"), self, parent, wp->x, wp->y, wp->cx, wp->cy, Flags, sizing);

	// ------------------------------------------------------
	struct edges h;
	struct edges v;
	struct snap_info si = { &h, &v, sizing, true, padding, self, parent };

	h.dmin = v.dmin = h.def = v.def = snapdist;

	h.from1 = wp->x;
	h.from2 = h.from1 + wp->cx;
	v.from1 = wp->y;
	v.from2 = v.from1 + wp->cy;

	// ------------------------------------------------------
	// snap to grid

	/*if (grid > 1 && (parent || sizing))
    {
        snap_to_grid(&h, &v, sizing, grid, padding);
    }*/
	//else
	{
		// -----------------------------------------
		if (parent) // snap to siblings
		{
			EnumChildWindows(parent, SnapEnumProc, (LPARAM)&si);
			if (0 == (Flags&SNAP_NOPARENT))
			{
				// snap to frame edges
				RECT r; GetClientRect(parent, &r);
				h.to1 = r.left;
				h.to2 = r.right;
				v.to1 = r.top;
				v.to2 = r.bottom;
				snap_to_edge(&h, &v, sizing, false, padding);
			}
		}
		else // snap to top level windows
		{
			if (snap_plugins)
				EnumThreadWindows(GetCurrentThreadId(), SnapEnumProc, (LPARAM)&si);

			// snap to screen edges
			RECT r; GetMonitorRect(self, &r, snap_workarea ?  GETMON_WORKAREA|GETMON_FROM_WINDOW : GETMON_FROM_WINDOW);
			h.to1 = r.left;
			h.to2 = r.right;
			v.to1 = r.top;
			v.to2 = r.bottom;
			snap_to_edge(&h, &v, sizing, false, 0);
		}
		// -----------------------------------------
		if (sizing)
		{
			if ((Flags & SNAP_CONTENT) && nDist_or_pContent)// snap to button icons
			{
				// images have to be double padded, since they are centered
				h.to2 = (h.to1 = h.from1) + ((SIZE*)nDist_or_pContent)->cx;
				v.to2 = (v.to1 = v.from1) + ((SIZE*)nDist_or_pContent)->cy;
				snap_to_edge(&h, &v, sizing, false, 0);
			}
			// snap frame to childs
			si.same_level = false;
			si.pad = -padding;
			si.self = NULL;
			si.parent = self;
			EnumChildWindows(self, SnapEnumProc, (LPARAM)&si);
		}
	}

	// -----------------------------------------
	// adjust the window-pos

	if (h.dmin < snapdist)
		if (sizing) wp->cx += h.omin; else wp->x += h.omin;

	if (v.dmin < snapdist)
		if (sizing) wp->cy += v.omin; else wp->y += v.omin;
}

//*****************************************************************************

BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam)
{
	struct snap_info *si = (struct snap_info *)lParam;
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

	if (hwnd != si->self && (style & WS_VISIBLE))
	{
		HWND pw = (style & WS_CHILD) ? GetParent(hwnd) : NULL;
		if (pw == si->parent && false == IsMenu(hwnd))
		{
			RECT r; GetWindowRect(hwnd, &r);
			r.right -= r.left;
			r.bottom -= r.top;
			if (pw) ScreenToClient(pw, (POINT*)&r.left);
			if (false == si->same_level)
			{
				r.left += si->h->from1;
				r.top  += si->v->from1;
			}
			si->h->to2 = (si->h->to1 = r.left) + r.right;
			si->v->to2 = (si->v->to1 = r.top)  + r.bottom;
			snap_to_edge(si->h, si->v, si->sizing, si->same_level, si->pad);
		}
	}
	return TRUE;
}   

//*****************************************************************************
/*
void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad)
{
	for (struct edges *g = h;;g = v)
	{
		int o, d;
		if (sizing) o = g->from2 - g->from1 + pad; // relative to topleft
		else        o = g->from1 - pad; // absolute coords

		o = o % grid;
		if (o < 0) o += grid;

		if (o >= grid / 2)
			d = o = grid-o;
		else
			d = o, o = -o;

		if (d < g->dmin) g->dmin = d, g->omin = o;

		if (g == v) break;
	}
}
*/
//*****************************************************************************
void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad)
{
	int o, d, n; struct edges *t;
	h->d = h->def; v->d = v->def;
	for (n = 2;;) // v- and h-edge
	{
		// see if there is any common edge, i.e if the lower top is above the upper bottom.
		if ((v->to2 < v->from2 ? v->to2 : v->from2) >= (v->to1 > v->from1 ? v->to1 : v->from1))
		{
			if (same_level) // child to child
			{
				//snap to the opposite edge, with some padding between
				bool f = false;

				d = o = (h->to2 + pad) - h->from1;  // left edge
				if (d < 0) d = -d;
				if (d <= h->d)
				{
					if (false == sizing)
						if (d < h->d) h->d = d, h->o = o;
					if (d < h->def) f = true;
				}

				d = o = h->to1 - (h->from2 + pad); // right edge
				if (d < 0) d = -d;
				if (d <= h->d)
				{
					if (d < h->d) h->d = d, h->o = o;
					if (d < h->def) f = true;
				}

				if (f)
				{
					// if it's near, snap to the corner
					if (false == sizing)
					{
						d = o = v->to1 - v->from1;  // top corner
						if (d < 0) d = -d;
						if (d < v->d) v->d = d, v->o = o;
					}
					d = o = v->to2 - v->from2;  // bottom corner
					if (d < 0) d = -d;
					if (d < v->d) v->d = d, v->o = o;
				}
			}
			else // child to frame
			{
				//snap to the same edge, with some bevel between
				if (false == sizing)
				{
					d = o = h->to1 - (h->from1 - pad); // left edge
					if (d < 0) d = -d;
					if (d < h->d) h->d = d, h->o = o;
				}
				d = o = h->to2 - (h->from2 + pad); // right edge
				if (d < 0) d = -d;
				if (d < h->d) h->d = d, h->o = o;
			}
		}
		if (0 == --n) break;
		t = h; h = v, v = t;
	}

	if (false == sizing && false == same_level)
	{
		// snap to center
		for (n = 2;;) // v- and h-edge
		{
			if (v->d < v->dmin)
			{
				d = o = (h->to1 + h->to2)/2 - (h->from1 + h->from2)/2;
				if (d < 0) d = -d;
				if (d < h->d) h->d = d, h->o = o;
			}
			if (0 == --n) break;
			t = h; h = v, v = t;
		}
	}

	if (h->d < h->dmin) h->dmin = h->d, h->omin = h->o;
	if (v->d < v->dmin) v->dmin = v->d, v->omin = v->o;
}

//===========================================================================

API_EXPORT(void) DisableInputMethod(HWND window)
{
	HIMC context;

	context = ImmAssociateContext(window,NULL);
	if(context)
		ImmDestroyContext(context);
}
