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

#ifndef XCSOAR_TERRAIN_RASTER_WEATHER_HPP
#define XCSOAR_TERRAIN_RASTER_WEATHER_HPP

#include "Engine/Navigation/GeoPoint.hpp"
#include "Poco/RWLock.h"
#include "Compiler.h"

#include <tchar.h>

class RasterMap;

/**
 * Class to manage raster weather data
 */
class RasterWeather {
public:
  static const unsigned MAX_WEATHER_MAP = 16; /**< Max number of items stored */
  static const unsigned MAX_WEATHER_TIMES = 48; /**< Max time segments of each item */

private:
  GEOPOINT center;

public:
  /** 
   * Default constructor
   */
  RasterWeather();

  ~RasterWeather();
  
  /** Close loaded data */
  void Close();

  void ValueToText(TCHAR* Buffer, short val) const;
  void SetViewCenter(const GEOPOINT &location);
  gcc_const static const TCHAR *ItemLabel(unsigned i);
  gcc_const static const TCHAR *ItemHelp(unsigned i);

  gcc_pure
  const RasterMap *GetMap() const;

  gcc_pure
  unsigned GetParameter() const;

  void SetParameter(unsigned i);

  /**
   * @param day_time the UTC time, in seconds since midnight
   */
  void Reload(int day_time);

  void ScanAll(const GEOPOINT &location);
  bool isWeatherAvailable(unsigned t) const;

  gcc_pure
  unsigned GetTime() const;

  void SetTime(unsigned i);

  gcc_const
  static int IndexToTime(int index);

private:
  unsigned _parameter;
  unsigned _weather_time;
  bool reload;
  RasterMap *weather_map;

  static void GetFilename(TCHAR *rasp_filename, const TCHAR *name,
                          unsigned time_index);

  bool LoadItem(const TCHAR* name, unsigned time_index);

  gcc_pure
  bool ExistsItem(const TCHAR* name, unsigned time_index) const;

  bool weather_available[MAX_WEATHER_TIMES];

  mutable Poco::RWLock lock;
  void _Close();
};

#endif
