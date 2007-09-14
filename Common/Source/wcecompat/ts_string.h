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


#ifndef __wcecompat__ts_string_h__
#define __wcecompat__ts_string_h__


#include <string.h>
#include <windows.h>


void ascii2unicode(const char* ascii, WCHAR* unicode);
void unicode2ascii(const WCHAR* unicode, char* ascii);
void ascii2unicode(const char* ascii, WCHAR* unicode, int maxChars);
void unicode2ascii(const WCHAR* unicode, char* ascii, int maxChars);


//
// ascii/unicode typesafe versions of strlen
//

inline size_t ts_strlen(const char* str)
{
	return strlen(str);
}

inline size_t ts_strlen(const unsigned short* str)
{
	return wcslen(str);
}


//
// ascii/unicode typesafe versions of strsize
// (returns size in bytes of str, not num characters as strlen does.  Does not include zero termination)
//

inline size_t ts_strsize(const char* str)
{
	return strlen(str);
}

inline size_t ts_strsize(const unsigned short* str)
{
	return wcslen(str)*2;
}


//
// ascii/unicode typesafe versions of strsize
// (returns size in bytes of str, not num characters as strlen does.  Includes zero termination)
//

inline size_t ts_strsizez(const char* str)
{
	return strlen(str)+1;
}

inline size_t ts_strsizez(const unsigned short* str)
{
	return (wcslen(str)+1)*2;
}


//
// ascii/unicode typesafe versions of strcpy
//

inline char* ts_strcpy(char* dest, const char* src)
{
	return strcpy(dest, src);
}

inline char* ts_strcpy(char* dest, const unsigned short* src)
{
	unicode2ascii(src, dest);
	return dest;
}

inline unsigned short* ts_strcpy(unsigned short* dest, const char* src)
{
	ascii2unicode(src, dest);
	return dest;
}

inline unsigned short* ts_strcpy(unsigned short* dest, const unsigned short* src)
{
	return wcscpy(dest, src);
}


//
// ascii/unicode typesafe versions of strncpy
//

inline char* ts_strncpy(char* dest, const char* src, int n)
{
	return strncpy(dest, src, n);
}

inline char* ts_strncpy(char* dest, const unsigned short* src, int n)
{
	unicode2ascii(src, dest, n);
	return dest;
}

inline unsigned short* ts_strncpy(unsigned short* dest, const char* src, int n)
{
	ascii2unicode(src, dest, n);
	return dest;
}

inline unsigned short* ts_strncpy(unsigned short* dest, const unsigned short* src, int n)
{
	return wcsncpy(dest, src, n);
}


//
// ascii/unicode typesafe versions of strcat
//

inline char* ts_strcat(char* dest, const char* src)
{
	return strcat(dest, src);
}

char* ts_strcat(char* dest, const unsigned short* src);
unsigned short* ts_strcat(unsigned short* dest, const char* src);

inline unsigned short* ts_strcat(unsigned short* dest, const unsigned short* src)
{
	return wcscat(dest, src);
}


//
// ascii/unicode typesafe versions of strdup
//

inline char* ts_strdup(const char* str)
{
	return _strdup(str);
}

char* ts_strdup_unicode_to_ascii(const unsigned short* str);

inline unsigned short* ts_strdup(const unsigned short* str)
{
	return wcsdup(str);
}

unsigned short* ts_strdup_ascii_to_unicode(const char* str);


#endif /* __wcecompat__ts_string_h__ */
