/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_TERRAIN_RASTER_WEATHER_HPP
#define XCSOAR_TERRAIN_RASTER_WEATHER_HPP

#include "Engine/Navigation/GeoPoint.hpp"
#include "Poco/RWLock.h"
#include "Compiler.h"

#include <tchar.h>

class RasterMap;
class OperationEnvironment;
struct zzip_dir;

/**
 * Class to manage raster weather data
 */
class RasterWeather {
public:
  static const unsigned MAX_WEATHER_MAP = 16; /**< Max number of items stored */
  static const unsigned MAX_WEATHER_TIMES = 48; /**< Max time segments of each item */

private:
  GeoPoint center;

  unsigned _parameter;
  unsigned _weather_time;
  bool reload;
  RasterMap *weather_map;

  mutable Poco::RWLock lock;

  bool weather_available[MAX_WEATHER_TIMES];

public:
  /** 
   * Default constructor
   */
  RasterWeather();

  ~RasterWeather();
  
  /** Close loaded data */
  void Close();

  void ValueToText(TCHAR* Buffer, short val) const;

  void SetViewCenter(const GeoPoint &location, fixed radius);

  gcc_pure
  bool IsDirty() const;

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
  void Reload(int day_time, OperationEnvironment &operation);

  void ScanAll(const GeoPoint &location, OperationEnvironment &operation);
  bool isWeatherAvailable(unsigned t) const;

  gcc_pure
  unsigned GetTime() const;

  void SetTime(unsigned i);

  gcc_const
  static int IndexToTime(int index);

private:
  static void NarrowWeatherFilename(char *filename, const TCHAR *name,
                                    unsigned time_index);

  static void GetFilename(TCHAR *rasp_filename, const TCHAR *name,
                          unsigned time_index);

  bool LoadItem(const TCHAR* name, unsigned time_index,
                OperationEnvironment &operation);

  gcc_pure
  bool ExistsItem(struct zzip_dir *dir, const TCHAR* name,
                  unsigned time_index) const;

  void _Close();
};

#endif
