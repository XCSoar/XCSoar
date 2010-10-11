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

#include "UtilsFont.hpp"

#include <assert.h>
#include <stdlib.h> /* for strtol() */

bool
GetFontFromString(const TCHAR *Buffer1, LOGFONT* lplf)
{
  // FontDescription of format:
  // typical font entry
  // 26,0,0,0,700,1,0,0,0,0,0,4,2,<fontname>

  LOGFONT lfTmp;

  assert(lplf != NULL);
  memset((void *)&lfTmp, 0, sizeof(LOGFONT));

  TCHAR *p;
  lfTmp.lfHeight = _tcstol(Buffer1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfWidth = _tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfEscapement = _tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfOrientation = _tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  //FW_THIN   100
  //FW_NORMAL 400
  //FW_MEDIUM 500
  //FW_BOLD   700
  //FW_HEAVY  900

  lfTmp.lfWeight = _tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfItalic = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfUnderline = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfStrikeOut = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfCharSet = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfOutPrecision = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfClipPrecision = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  // DEFAULT_QUALITY			   0
  // RASTER_FONTTYPE			   0x0001
  // DRAFT_QUALITY			     1
  // NONANTIALIASED_QUALITY  3
  // ANTIALIASED_QUALITY     4
  // CLEARTYPE_QUALITY       5
  // CLEARTYPE_COMPAT_QUALITY 6

  lfTmp.lfQuality = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfPitchAndFamily = (unsigned char)_tcstol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  _tcscpy(lfTmp.lfFaceName, p + 1);

  *lplf = lfTmp;
  return true;
}
