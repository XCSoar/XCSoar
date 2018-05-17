/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_WEATHER_RASP_CACHE_HPP
#define XCSOAR_WEATHER_RASP_CACHE_HPP

#include "Compiler.h"

#include <tchar.h>

struct BrokenTime;
struct GeoPoint;
class RaspStore;
class RasterMap;
class OperationEnvironment;

/**
 * Class to manage the raster weather map, to be loaded/selected from
 * a #RaspStore instance.
 */
class RaspCache {
  const RaspStore &store;

  const unsigned parameter;

  unsigned time = 0;
  unsigned last_time = 0;

  RasterMap *map = nullptr;

public:
  /** 
   * Default constructor
   */
  RaspCache(const RaspStore &_store, unsigned _parameter)
    :store(_store), parameter(_parameter) {}

  ~RaspCache() {
    Close();
  }

  const RaspStore &GetStore() const {
    return store;
  }

  gcc_pure
  const RasterMap *GetMap() const {
    return map;
  }

  /**
   * Returns the current map's name.
   */
  gcc_pure
  const TCHAR *GetMapName() const;

  /**
   * Returns the human-readable name for the current RASP map, or
   * nullptr if no RASP map is enabled.
   */
  gcc_pure
  const TCHAR *GetMapLabel() const;

  /**
   * Returns the index of the weather map being displayed.
   */
  gcc_pure
  unsigned GetParameter() const {
    return parameter;
  }

  gcc_pure
  bool IsInside(GeoPoint p) const;

  /**
   * @param day_time the local time, in seconds since midnight
   */
  void Reload(BrokenTime time_local, OperationEnvironment &operation);

  /**
   * Returns the current time index.
   */
  gcc_pure
  BrokenTime GetTime() const;

  /**
   * Sets the current time index.
   */
  void SetTime(BrokenTime t);

private:
  void Close();
};

#endif
