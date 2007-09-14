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


#include <string.h>
#include <stdlib.h>


_CRTIMP int __cdecl _stricmp(const char* s1, const char* s2)
{
	int	c1, c2;
	do
	{
		c1 = (unsigned char)*s1++;
		if (c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';
		c2 = (unsigned char)*s2++;
		if (c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';
	} while (c1 && (c1 == c2));
	return c1 - c2;
}

_CRTIMP int __cdecl _strnicmp(const char* s1, const char* s2, size_t n)
{
	int	c1, c2;
	do
	{
		c1 = (unsigned char)*s1++;
		if (c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';
		c2 = (unsigned char)*s2++;
		if (c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';
	} while ((--n > 0) && c1 && (c1 == c2));
	return c1 - c2;
}

_CRTIMP char* __cdecl strrchr(const char* s, int c)
{
	const char*	p = s + strlen(s) - 1;
	while (p >= s)
	{
		if (*p == c)
			return (char*)p;
		p--; // JMW bug fix!
	}
	return NULL;
}

_CRTIMP char* __cdecl _strdup(const char* s)
{
	char* new_s = (char*)malloc(strlen(s)+1);
	if (new_s == NULL)
		return NULL;
	strcpy(new_s, s);
	return new_s;
}
