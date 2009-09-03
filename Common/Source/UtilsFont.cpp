/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include <assert.h>
#include "Registry.hpp"

#include <stdlib.h> /* for strtol() */

void propGetFontSettingsFromString(const TCHAR *Buffer1, LOGFONT* lplf)
{
#define propGetFontSettingsMAX_SIZE 128
  TCHAR Buffer[propGetFontSettingsMAX_SIZE]; // RLD need a buffer (not sz) for strtok_r w/ gcc optimized ARM920

  TCHAR *pWClast, *pToken;
  LOGFONT lfTmp;
  _tcsncpy(Buffer, Buffer1, propGetFontSettingsMAX_SIZE);
    // FontDescription of format:
    // typical font entry
    // 26,0,0,0,700,1,0,0,0,0,0,4,2,<fontname>

    //FW_THIN   100
    //FW_NORMAL 400
    //FW_MEDIUM 500
    //FW_BOLD   700
    //FW_HEAVY  900

  assert(lplf != NULL);
  memset ((void *)&lfTmp, 0, sizeof (LOGFONT));

  if ((pToken = strtok_r(Buffer, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfHeight = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfWidth = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfEscapement = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfOrientation = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfWeight = _tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfItalic = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfUnderline = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfStrikeOut = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfCharSet = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfOutPrecision = (unsigned char)_tcstol(pToken, NULL, 10);
  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfClipPrecision = (unsigned char)_tcstol(pToken, NULL, 10);

  // DEFAULT_QUALITY			   0
  // RASTER_FONTTYPE			   0x0001
  // DRAFT_QUALITY			     1
  // NONANTIALIASED_QUALITY  3
  // ANTIALIASED_QUALITY     4
  // CLEARTYPE_QUALITY       5
  // CLEARTYPE_COMPAT_QUALITY 6

  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfQuality = (unsigned char)_tcstol(pToken, NULL, 10);

  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;
  lfTmp.lfPitchAndFamily = (unsigned char)_tcstol(pToken, NULL, 10);

  if ((pToken = strtok_r(NULL, TEXT(","), &pWClast)) == NULL) return;

  _tcscpy(lfTmp.lfFaceName, pToken);

  memcpy((void *)lplf, (void *)&lfTmp, sizeof (LOGFONT));

  return;
}


void propGetFontSettings(const TCHAR *Name, LOGFONT* lplf) {

  TCHAR Buffer[128];

  assert(Name != NULL);
  assert(Name[0] != '\0');
  assert(lplf != NULL);

#if (WINDOWSPC>0) && !defined(PCGNAV)
  // Don't load font settings from registry values for windows version
  return;
#endif

  if (GetRegistryString(Name, Buffer, sizeof(Buffer)/sizeof(TCHAR)) == 0) {
    propGetFontSettingsFromString(Buffer, lplf);
  }
}

