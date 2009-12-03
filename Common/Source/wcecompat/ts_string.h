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
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#ifdef _UNICODE

void ascii2unicode(const char* ascii, WCHAR* unicode);
void unicode2ascii(const WCHAR* unicode, char* ascii);
void ascii2unicode(const char* ascii, WCHAR* unicode, int maxChars);
void unicode2ascii(const WCHAR* unicode, char* ascii, int maxChars);

#else

void unicode2ascii(const char *unicode, char *ascii, int maxChars);
void unicode2ascii(const char *unicode, char *ascii);
void ascii2unicode(const char *ascii, char *unicode);

#endif

#endif /* __wcecompat__ts_string_h__ */
