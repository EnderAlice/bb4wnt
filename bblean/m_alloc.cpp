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
*/ /* m_alloc.cpp - memory block tracker */

#ifndef __BBAPI_H_
#include "BB.h"
#else
#include "m_alloc.h"
#endif

#ifdef BBOPT_MEMCHECK
#undef new
/*
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
*/
#include <malloc.h>

struct alloc_block
{
	struct alloc_block *prev;
	struct alloc_block *next;
	int line;
	TCHAR file[40];
	int size;
	unsigned long check;
	unsigned long content;
};

#define HASH_SIZE 256
struct alloc_block *SA[HASH_SIZE];
unsigned alloc_size;
unsigned alloc_size_max;
unsigned alloc_count;
unsigned alloc_count_max;

//===========================================================================
void mem_error_msg(TCHAR *msg, struct alloc_block *ab)
{
	TCHAR buf[1024];
	_stprintf(buf, _T("Error: %s:%d: %s (%d bytes)\n"), ab->file, ab->line, msg, ab->size);
	if (IDCANCEL == MessageBox(NULL, buf,_T("message from mem-check"), MB_OKCANCEL|MB_TOPMOST))
	{
		//DebugBreak();
		ExitProcess(0);
	}
}

//===========================================================================
void link_sa(struct alloc_block *ab)
{
	unsigned int i = ((unsigned int)ab >> 4) & (HASH_SIZE - 1);
	if (SA[i]) SA[i]->prev = ab;
	ab->next = SA[i];
	ab->prev = NULL;
	SA[i] = ab;
}

void free_sa(struct alloc_block *ab)
{
	unsigned int i = ((unsigned int)ab >> 4) & (HASH_SIZE - 1);
	if (SA[i] == ab)
	{
		SA[i] = ab->next;
	}
	else
	{
		if (ab->prev) ab->prev->next = ab->next;
		if (ab->next) ab->next->prev = ab->prev;
	}
}

int check_block(const TCHAR *msg, struct alloc_block *ab)
{
	TCHAR buffer[256];
	int n = ab->size;
	unsigned long x = n^0x12345678;
	if (x != ab->check)
		_stprintf(buffer, _T("%s: block is unknown"), msg);
	else
		if (x != ((struct alloc_block *)((char *)ab + n))->content)
			_stprintf(buffer, _T("%s: block is corrupted"), msg);
		else
			return 1;

	mem_error_msg(buffer, ab);
	return 0;
}

unsigned m_alloc_usable_size(void *v)
{
	struct alloc_block *ab;
	if (v==NULL) return 0;
	ab = (struct alloc_block *)((char *)v - (unsigned)&((struct alloc_block*)NULL)->content);
	if (!check_block(_T("Check"), ab))
		return 0;
	return ab->size;
}

//===========================================================================
FILE *alloc_fp;

void m_alloc_printf(const TCHAR *fmt, ...)
{
	TCHAR buffer[4096];
	if (NULL == alloc_fp)
	{
		GetModuleFileName(NULL, buffer, MAX_PATH);
		_tcscpy(_tcsrchr(buffer, _T('\\'))+1, _T("m_alloc_dump.log"));
		alloc_fp = fopen(buffer, _T("wt, ccs=UTF-8"));
	}
	va_list arg;
	va_start(arg, fmt);
	_vstprintf (buffer, fmt, arg);
	fputts (buffer, alloc_fp);
	//OutputDebugString(buffer);
}

void m_alloc_dump_memory(void)
{
	int i;
	m_alloc_printf(
		_T("memory allocation:\n")
		_T("count\t%d\t(max: %d)\n")
		_T("size \t%d\t(max: %d)\n\n"),
		alloc_count,  alloc_count_max,
		alloc_size, alloc_size_max
		);
	for (i = 0; i < HASH_SIZE; i++)
	{
		struct alloc_block *ab = SA[i];
		while (ab)
		{
			int n = ab->size;
			int x = ((struct alloc_block *)((char *)ab + n))->content;
			if ((x^n)!=0x12345678)
			{
				m_alloc_printf(_T("Error: %s:%d: %s (%d bytes)\n\n"),
							   ab->file,
							   ab->line,
							   _T("block corrupted"),
							   ab->size
							   );
				break;
			}
			else
			{
				m_alloc_printf(_T("Error: %s:%d: %s (%d bytes)\n"),
							   ab->file,
							   ab->line,
							   _T("block is ok"),
							   ab->size
							   );
				m_alloc_printf(_T("content: <%s>\n\n"), &ab->content);
			}
			ab = ab->next;
		}
	}
	fclose(alloc_fp); alloc_fp = NULL;
}

void m_alloc_check_memory(void)
{
	int i;
	for (i = 0; i < HASH_SIZE; i++)
	{
		struct alloc_block *ab = SA[i];
		while (ab)
		{
			check_block(_T("check"), ab);
			ab = ab->next;
		}
	}
}

void *_m_alloc_raw(unsigned n, const TCHAR *file, int line)
{
	struct alloc_block *ab; unsigned l; const char *s;

	if (n==0) return NULL;

	ab=(struct alloc_block*)malloc(sizeof(*ab) + n);
	//ab=(struct alloc_block*)GlobalAlloc(GMEM_FIXED, sizeof(*ab) + n);

	if (NULL==ab) {
		mem_error_msg(_T("m_alloc failed"), ab);
		return ab;
	}

	memset(ab, 0x22, sizeof *ab + n);

	ab->size = n;
	ab->line = line;
	s = file;

#if 1
	s = _tcsrchr(file, _T('/'));
	if (NULL == s) s = _tcsrchr(file, _T('\\'));
	if (NULL == s) s = file; else s++;
#endif

	l = _tcslen(s);
	if (l >= sizeof ab->file)
		_tmemcpy(ab->file, s + l - (sizeof ab->file - 1), sizeof ab->file -1), l = sizeof ab->file - 1;
	else
		_tmemcpy(ab->file, s, l);
	ab->file[l] = 0;

	((struct alloc_block *)((char *)ab + n))->content =
		ab->check = 0x12345678^n;

	alloc_size+=n;
	++alloc_count;
	if (alloc_size > alloc_size_max) alloc_size_max = alloc_size;
	if (alloc_count > alloc_count_max) alloc_count_max = alloc_count;

	link_sa(ab);
	return &ab->content;
}


void *_m_alloc (unsigned n, const char *file, int line)
{
	void *v = _m_alloc_raw(n, file, line);
	if (v) memset(v, 0x55, n);
	return v;
}

void *_c_alloc (unsigned n, const char *file, int line)
{
	void *v = _m_alloc_raw(n, file, line);
	if (v) memset(v, 0, n);
	return v;
}

void _m_free(void *v)
{
	struct alloc_block *ab;

	if (v==NULL) return;

	ab = (struct alloc_block *)((char *)v - (unsigned)&((struct alloc_block*)NULL)->content);
	if (!check_block(_T("Free"), ab))
		return;

	free_sa(ab);
	alloc_size-=ab->size;
	--alloc_count;
	memset(ab, 0x33, sizeof *ab + ab->size);
	free(ab);
	//GlobalFree(ab);
}

void *_m_realloc (void *v, unsigned s, const char *file, int line)
{
	struct alloc_block *ab; void *v2;

	if (v==NULL) {
		return _m_alloc(s,file,line);
	}
	if (s==0) {
		_m_free(v); return NULL;
	}

	ab = (struct alloc_block *)((char *)v - (unsigned)&((struct alloc_block*)NULL)->content);
	if (!check_block(_T("Realloc"), ab))
		return NULL;

	v2 = _m_alloc(s, file, line);
	if (v2) memcpy(v2, v, ab->size);
	_m_free(v);
	return v2;
}

//===========================================================================

//===========================================================================

const TCHAR *m_file;
int m_line;

void _m_setinfo(const TCHAR *file, int line)
{
	m_file = file;
	m_line = line;
}

void * operator new (unsigned n)
{
	return _m_alloc(n, m_file, m_line);
}

void operator delete (void *v)
{
	_m_free(v);
}

void * operator new[] (unsigned n)
{
	return _m_alloc(n, m_file, m_line);
}

void operator delete[] (void *v)
{
	_m_free(v);
}

//===========================================================================
#else
#ifdef __GNUC__  // get rid of bloated libstdc++ implementation
#include <malloc.h>
void * operator new (unsigned n)
{
	return malloc(n);
}
void operator delete (void *v)
{
	free(v);
}
void * operator new[] (unsigned n)
{
	return malloc(n);
}
void operator delete[] (void *v)
{
	free(v);
}
#endif
#endif

#ifdef __GNUC__  // get rid of bloated libstdc++ implementation
extern "C" void __cxa_pure_virtual(void) { }
#endif

#if defined __BORLANDC__ && defined CRTSTRING
extern "C" void _pure_error_(void) { }
#endif

