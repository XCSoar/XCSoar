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

#ifndef RASTERWEATHER_H
#define RASTERWEATHER_H

#include "Poco/RWLock.h"

#include <tchar.h>

class RasterMap;
struct GEOPOINT;

class RasterWeather {
public:
  static const unsigned MAX_WEATHER_MAP = 16;
  static const unsigned MAX_WEATHER_TIMES = 48;

public:
  RasterWeather() {
    unsigned i;
    bsratio = false;
    for (i=0; i<MAX_WEATHER_MAP; i++) {
      weather_map[i]= 0;
    }
    for (i=0; i<MAX_WEATHER_TIMES; i++) {
      weather_available[i]= false;
    }
    _weather_time = 0;
    _parameter = 0;
  }
  ~RasterWeather() {
    Close();
  }
  void Close();

  void ValueToText(TCHAR* Buffer, short val);
  void SetViewCenter(const GEOPOINT &location);
  void ItemLabel(int i, TCHAR* Buffer);
  RasterMap* GetMap();
  unsigned GetParameter();
  void SetParameter(unsigned i);

  /**
   * @param location Location of observer
   * @param day_time the UTC time, in seconds since midnight
   */
  void Reload(const GEOPOINT &location, int day_time);

  void ScanAll(const GEOPOINT &location);
  bool isWeatherAvailable(unsigned t);
  unsigned GetTime();
  void SetTime(unsigned i);
  int IndexToTime(int x);
 private:
  unsigned _parameter; // was terrain.render_weather
  unsigned _weather_time;
  RasterMap* weather_map[MAX_WEATHER_MAP];
  void RASP_filename(char* rasp_filename, const TCHAR* name);
  bool LoadItem(int item, const TCHAR* name);
  void ServiceFullReload(const GEOPOINT &location);
  bool weather_available[MAX_WEATHER_TIMES];
 private:
  bool bsratio;
  Poco::RWLock lock;
  void _Close();
};

#endif
