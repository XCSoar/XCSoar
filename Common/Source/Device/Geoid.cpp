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

/**
 * This file handles the geoid separation
 * @file Geoid.cpp
 * @see http://en.wikipedia.org/wiki/EGM96
 */

#include "Device/Geoid.h"
#include "Interface.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <tchar.h>

#define EGM96SIZE 16200

unsigned char* egm96data= NULL;

/**
 * Load the EGM96 geoid resource into egm96data.
 */
void
OpenGeoid(void)
{
  const TCHAR *lpRes;
  HRSRC hResInfo;
  HGLOBAL hRes;
  int len;

  hResInfo = FindResource(XCSoarInterface::hInst,
                          _T("IDR_RASTER_EGM96S"),
                          _T("RASTERDATA"));

  if (hResInfo == NULL) {
    // unable to find the resource
    egm96data = NULL;
    return;
  }

  // Load the wave resource.
  hRes = LoadResource(XCSoarInterface::hInst, hResInfo);
  if (hRes == NULL) {
    // unable to load the resource
    egm96data = NULL;
    return;
  }

  // Lock the wave resource and do something with it.
  lpRes = (const TCHAR *)LockResource(hRes);

  if (lpRes) {
    len = SizeofResource(XCSoarInterface::hInst,hResInfo);

    if (len == EGM96SIZE) {
      egm96data = (unsigned char*)malloc(len);
      strncpy((char*)egm96data,(const char*)lpRes,len);
    } else {
      egm96data = NULL;
      return;
    }
  }

  return;
}

/**
 * Clear the EGM96 from the memory
 */
void
CloseGeoid(void)
{
  if (egm96data) {
    free(egm96data);
    egm96data = NULL;
  }
}

/**
 * Returns the geoid separation between the EGS96
 * and the WGS84 at the given latitude and longitude
 * @param lat Latitude
 * @param lon Longitude
 * @return The geoid separation
 */
double
LookupGeoidSeparation(double lat, double lon)
{
  if (!egm96data)
    return 0.0;

  int ilat, ilon;
  ilat = iround((90.0-lat)/2.0);
  // TODO: TB: use limit180
  if (lon < 0) {
    lon += 360.0;
  }
  ilon = iround(lon/2.0);

  int offset = ilat*180+ilon;
  if (offset >= EGM96SIZE)
    return 0.0;
  if (offset < 0)
    return 0.0;

  double val = (double)(egm96data[offset]) - 127;
  return val;
}
