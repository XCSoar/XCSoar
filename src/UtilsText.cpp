/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "UtilsText.hpp"
#include "Compatibility/string.h"
#include "Sizes.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <windef.h>

/**
 * Parses the special characters (cr, lf, back slash) in the old_string and
 * returns the parsed new_string
 * @param old_string The old string with (or without) special characters
 * @return The new parsed string
 */
TCHAR *
StringMallocParse(const TCHAR* old_string)
{
  TCHAR buffer[2048]; // Note - max size of any string we cope with here !

  unsigned int used = 0;

  for (unsigned int i = 0; i < _tcslen(old_string); i++) {
    if (used < 2045) {
      if (old_string[i] == '\\') {
        if (old_string[i + 1] == 'r') {
          buffer[used++] = '\r';
          i++;
        } else if (old_string[i + 1] == 'n') {
          buffer[used++] = '\n';
          i++;
        } else if (old_string[i + 1] == '\\') {
          buffer[used++] = '\\';
          i++;
        } else {
          buffer[used++] = old_string[i];
        }
      } else {
        buffer[used++] = old_string[i];
      }
    }
  }

  buffer[used++] = _T('\0');

  return _tcsdup(buffer);
}

/**
 * Converts a char array to a TCHAR array
 * @param pszDest TCHAR array (Output)
 * @param pszSrc char array (Input)
 */
void
ConvertCToT(TCHAR *pszDest, const char *pszSrc)
{
  do {
    *pszDest++ = (TCHAR)*pszSrc;
  } while (*pszSrc++ != '\0');
}

int
TextToLineOffsets(const TCHAR *text, int *LineOffsets, int maxLines)
{
  assert(LineOffsets != NULL);
  assert(maxLines > 0);

  if (text == NULL || text[0] == 0)
    return 0;

  int nTextLines = 0;
  const TCHAR *p = text;

  do {
    const TCHAR *newline = _tcschr(p, _T('\n'));
    if (newline == NULL)
      break;

    LineOffsets[nTextLines++] = p - text;
    p = newline + 1;
  } while (nTextLines < maxLines);

  return nTextLines;
}
