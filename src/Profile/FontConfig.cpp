/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Profile/FontConfig.hpp"
#include "Profile/Profile.hpp"
#include "Util/Macros.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* for strtol() */

#ifdef _UNICODE
#include <windows.h>
#endif

static bool
GetFontFromString(const char *Buffer1, LOGFONT* lplf)
{
  // FontDescription of format:
  // typical font entry
  // 26,0,0,0,700,1,0,0,0,0,0,4,2,<fontname>

  LOGFONT lfTmp;

  assert(lplf != NULL);
  memset((void *)&lfTmp, 0, sizeof(LOGFONT));

  char *p;
  lfTmp.lfHeight = strtol(Buffer1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfWidth = strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfEscapement = strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfOrientation = strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  //FW_THIN   100
  //FW_NORMAL 400
  //FW_MEDIUM 500
  //FW_BOLD   700
  //FW_HEAVY  900

  lfTmp.lfWeight = strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfItalic = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfUnderline = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfStrikeOut = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfCharSet = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfOutPrecision = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfClipPrecision = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  // DEFAULT_QUALITY			   0
  // RASTER_FONTTYPE			   0x0001
  // DRAFT_QUALITY			     1
  // NONANTIALIASED_QUALITY  3
  // ANTIALIASED_QUALITY     4
  // CLEARTYPE_QUALITY       5
  // CLEARTYPE_COMPAT_QUALITY 6

  lfTmp.lfQuality = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

  lfTmp.lfPitchAndFamily = (unsigned char)strtol(p + 1, &p, 10);
  if (*p != _T(','))
    return false;

#ifdef _UNICODE
  MultiByteToWideChar(CP_UTF8, 0, p + 1, -1,
                      lfTmp.lfFaceName, ARRAY_SIZE(lfTmp.lfFaceName));
#else
  strcpy(lfTmp.lfFaceName, p + 1);
#endif

  *lplf = lfTmp;
  return true;
}

bool
Profile::GetFont(const char *key, LOGFONT* lplf)
{
  assert(key != NULL);
  assert(key[0] != '\0');
  assert(lplf != NULL);

  const char *value = Get(key);
  return value != NULL && GetFontFromString(value, lplf);
}

void
Profile::SetFont(const char *key, LOGFONT &logfont)
{
  assert(key != NULL);
  assert(key[0] != '\0');

#ifdef _UNICODE
  char face[256];
  WideCharToMultiByte(CP_UTF8, 0, logfont.lfFaceName, -1,
                      face, ARRAY_SIZE(face),
                      nullptr, nullptr);
#else
  const char *face = logfont.lfFaceName;
#endif

  NarrowString<256> buffer;
  buffer.Format("%d,%d,0,0,%d,%d,0,0,0,0,0,%d,%d,%s", logfont.lfHeight,
                logfont.lfWidth, logfont.lfWeight, logfont.lfItalic,
                logfont.lfQuality, logfont.lfPitchAndFamily,
                face);

  Profile::Set(key, buffer);
}
