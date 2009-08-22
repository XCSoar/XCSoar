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

#ifndef RASTERWEATHER_H
#define RASTERWEATHER_H

#include "Sizes.h"
#include "RasterMap.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class RasterWeather {
public:
  RasterWeather() {
    int i;
    bsratio = false;
    for (i=0; i<MAX_WEATHER_MAP; i++) {
      weather_map[i]= 0;
    }
    for (i=0; i<MAX_WEATHER_TIMES; i++) {
      weather_available[i]= false;
    }
    weather_time = 0;
  }
  ~RasterWeather() {
    Close();
  }
 public:
  void Close();
  void Reload(double lat, double lon);
  int weather_time;
  RasterMap* weather_map[MAX_WEATHER_MAP];
  void RASP_filename(char* rasp_filename, const TCHAR* name);
  bool LoadItem(int item, const TCHAR* name);
  void SetViewCenter(double lat, double lon);
  void ServiceFullReload(double lat, double lon);
  void ValueToText(TCHAR* Buffer, short val);
  void ItemLabel(int i, TCHAR* Buffer);
  void Scan(double lat, double lon);
  bool weather_available[MAX_WEATHER_TIMES];
  int IndexToTime(int x);
 private:
  bool bsratio;
};

extern RasterWeather RASP;

#endif
