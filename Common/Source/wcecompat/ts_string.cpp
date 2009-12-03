/*  wcecompat: Windows CE C Runtime Library "compatibility" library.
 *
 *  Copyright (C) 2001-2002 Essemer Pty Ltd.  All rights reserved.
 *  http://www.essemer.com.au/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ts_string.h"

#include <string.h>
#include <stdlib.h>

#ifdef _UNICODE

static bool
check_wchar_align(const void *p)
{
  return ((long)p & (sizeof(WCHAR) - 1)) == 0;
}

void ascii2unicode(const char* ascii, WCHAR* unicode)
{
  if (strlen(ascii)==0) {
    unicode[0]=0;
    unicode[1]=0;
    return;
  }

  if (check_wchar_align(unicode))
	{	// word-aligned
		while (*ascii != '\0')
			*unicode++ = *ascii++;
		*unicode = '\0';
	}
	else
	{	// not word-aligned
		while (*ascii != '\0')
		{
			*(char*)unicode = *ascii++;
			*(((char*)unicode)+1) = 0;
			unicode++;
		}
		*(char*)unicode = 0;
		*(((char*)unicode)+1) = 0;
	}
}

void unicode2ascii(const WCHAR* unicode, char* ascii)
{
  if (wcslen(unicode)==0) {
    ascii[0] = 0;
    return;
  }

  if (check_wchar_align(unicode))
	{	// word-aligned
		while (*unicode != '\0')
			*ascii++ = (char)*unicode++;
		*ascii = '\0';
	}
	else
	{	// not word-aligned
		while (*(char*)unicode != 0 || *(((char*)unicode)+1) != 0)
			*ascii++ = *(char*)unicode++;
		*ascii = '\0';
	}
}

void ascii2unicode(const char* ascii, WCHAR* unicode, int maxChars)
{
  if (strlen(ascii)==0) {
    unicode[0]=0;
    unicode[1]=0;
    return;
  }

  if (check_wchar_align(unicode))
	{	// word-aligned
    int i;
		for (i=0; ascii[i] != 0 && i<maxChars; i++)
			unicode[i] = ascii[i];
		unicode[i] = 0;
	}
	else
	{	// not word-aligned
    int i;
		for (i=0; ascii[i] != 0 && i<maxChars; i++)
		{
			*(char*)&unicode[i] = ascii[i];
			*(((char*)&unicode[i])+1) = 0;
			unicode++;
		}
		*(char*)&unicode[i] = 0;
		*(((char*)&unicode[i])+1) = 0;
	}
}

void unicode2ascii(const WCHAR* unicode, char* ascii, int maxChars)
{
  if (wcslen(unicode)==0) {
    ascii[0] = 0;
    return;
  }

  if (check_wchar_align(unicode))
	{	// word-aligned
    int i;
		for (i=0; unicode[i] != 0 && i<maxChars; i++)
			ascii[i] = (char)unicode[i];
		ascii[i] = 0;
	}
	else
	{	// not word-aligned
    int i;
		for (i=0; (*(char*)&unicode[i] != 0 || *(((char*)&unicode[i])+1) != 0) && i<maxChars; i++)
			ascii[i] = *(char*)&unicode[i];
		ascii[i] = 0;
	}
}

#else /* !_UNICODE */

void unicode2ascii(const char *unicode, char *ascii, int maxChars)
{
  strncpy(ascii, unicode, maxChars - 1);
  ascii[maxChars - 1] = 0;
}

void unicode2ascii(const char *unicode, char *ascii)
{
  strcpy(ascii, unicode);
}

void ascii2unicode(const char *ascii, char *unicode)
{
  strcpy(unicode, ascii);
}

#endif /* _UNICODE */
