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


#include <ctype.h>
#include <stdlib.h>

_CRTIMP const unsigned short _ctype[] =
{
	// _UPPER  : 41-5a
	// _LOWER  : 61-7a
	// _DIGIT  : 30-39
	// _SPACE  : 09-0d 20
	// _PUNCT  : 21-2f 3a-40 5b-60 7b-7e
	// _CONTROL: 00-1f 7f
	// _BLANK  : 20
	// _HEX    : 30-39 41-46 61-66
	// _ALPHA  : 41-5a 61-7a

	// don't know what _ctype[0] is for
	0,
	// 00-08: _CONTROL
	_CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL,
	// 09-0D: _CONTROL|_SPACE
	_CONTROL|_SPACE, _CONTROL|_SPACE, _CONTROL|_SPACE, _CONTROL|_SPACE, _CONTROL|_SPACE,
	// 0E-1F: _CONTROL
	_CONTROL, _CONTROL,
	_CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL,
	_CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL, _CONTROL,
	// 20: _BLANK|_SPACE
	_BLANK|_SPACE,
	// 21-2F: _PUNCT
	_PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT,
	_PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT,
	// 30-39: _HEX|_DIGIT
	_HEX|_DIGIT, _HEX|_DIGIT, _HEX|_DIGIT, _HEX|_DIGIT, _HEX|_DIGIT,
	_HEX|_DIGIT, _HEX|_DIGIT, _HEX|_DIGIT, _HEX|_DIGIT, _HEX|_DIGIT,
	// 3A-40: _PUNCT
	_PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT,
	// 41-46: _HEX|_UPPER
	_HEX|_UPPER, _HEX|_UPPER, _HEX|_UPPER, _HEX|_UPPER, _HEX|_UPPER, _HEX|_UPPER,
	// 47-5A: _UPPER
	_UPPER,
	_UPPER, _UPPER, _UPPER, _UPPER, _UPPER, _UPPER, _UPPER, _UPPER,
	_UPPER, _UPPER, _UPPER, _UPPER, _UPPER, _UPPER, _UPPER, _UPPER,
	_UPPER, _UPPER, _UPPER,
	// 5B-60: _PUNCT
	_PUNCT, _PUNCT, _PUNCT, _PUNCT, _PUNCT,	_PUNCT,
	// 61-66: _HEX|_LOWER
	_HEX|_LOWER, _HEX|_LOWER, _HEX|_LOWER, _HEX|_LOWER, _HEX|_LOWER, _HEX|_LOWER,
	// 67-7A: _LOWER
	_LOWER,
	_LOWER, _LOWER, _LOWER, _LOWER, _LOWER, _LOWER, _LOWER, _LOWER,
	_LOWER, _LOWER, _LOWER, _LOWER, _LOWER, _LOWER, _LOWER, _LOWER,
	_LOWER, _LOWER, _LOWER,
	// 7B-7E: _PUNCT
	_PUNCT, _PUNCT, _PUNCT, _PUNCT,
	// 7F: _CONTROL
	_CONTROL,
	// 80-FF: 0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

_CRTIMP const unsigned short *_pctype = &_ctype[1];
_CRTIMP const wctype_t *_pwctype = &_ctype[1];


_CRTIMP int __cdecl _isctype(int c, int mask)
{
	return (int)(_pctype[c] & mask);
}

#ifdef _WIN32_WCE_EMULATION
_CRTIMP int __cdecl iswctype(wint_t c, wctype_t mask)
#else
_CRTIMP int iswctype(wint_t c, wctype_t mask)
#endif
{
	return (int)(_pctype[c] & mask);
}

#ifdef _WIN32_WCE_EMULATION
_CRTIMP wchar_t __cdecl towupper(wchar_t c)
#else
_CRTIMP wchar_t towupper(wchar_t c)
#endif
{
	return (wchar_t)toupper(c & 0xff);
}

#ifdef _WIN32_WCE_EMULATION
_CRTIMP wchar_t __cdecl towlower(wchar_t c)
#else
_CRTIMP wchar_t towlower(wchar_t c)
#endif
{
	return (wchar_t)tolower(c & 0xff);
}
