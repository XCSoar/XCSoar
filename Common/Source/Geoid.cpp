/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

#include "StdAfx.h"
#include "Utils.h"

#include <stdlib.h>

#define EGM96SIZE 16200

unsigned char* egm96data= NULL;

extern HINSTANCE hInst;

void OpenGeoid(void) {
  LPTSTR lpRes;
  HRSRC hResInfo;
  HGLOBAL hRes;
  int len;
  hResInfo = FindResource (hInst, TEXT("IDR_RASTER_EGM96S"), TEXT("RASTERDATA"));

  if (hResInfo == NULL) {
    // unable to find the resource
    egm96data = NULL;
    return;
  }
  // Load the wave resource.
  hRes = LoadResource (hInst, hResInfo);
  if (hRes == NULL) {
    // unable to load the resource
    egm96data = NULL;
    return;
  }

  // Lock the wave resource and do something with it.
  lpRes = (LPTSTR)LockResource (hRes);

  if (lpRes) {
    len = SizeofResource(hInst,hResInfo);
    if (len==EGM96SIZE) {
      egm96data = (unsigned char*)malloc(len);
      strncpy((char*)egm96data,(char*)lpRes,len);
    } else {
      egm96data = NULL;
      return;
    }
  }
  return;
}


void CloseGeoid(void) {
  if (egm96data) {
    free(egm96data);
    egm96data = NULL;
  }
}


double LookupGeoidSeparation(double lat, double lon) {
  if (!egm96data) return 0.0;

  int ilat, ilon;
  ilat = iround((90.0-lat)/2.0);
  if (lon<0) {
    lon+= 360.0;
  }
  ilon = iround(lon/2.0);

  int offset = ilat*180+ilon;
  if (offset>=EGM96SIZE)
    return 0.0;
  if (offset<0)
    return 0.0;

  double val = (double)(egm96data[offset])-127;
  return val;
}

