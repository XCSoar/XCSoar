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

#ifndef NOAA_STORE_HPP
#define NOAA_STORE_HPP

#include "METAR.hpp"
#include "ParsedMETAR.hpp"
#include "TAF.hpp"

#include <list>

#ifdef _UNICODE
#include <tchar.h>
#endif

class NOAAStore
{
public:
  struct Item
  {
    char code[5];

    bool metar_available;
    bool parsed_metar_available;
    bool taf_available;

    METAR metar;
    ParsedMETAR parsed_metar;
    TAF taf;

    /**
     * Returns the four letter code as a TCHAR string.  This may
     * return a pointer to a static buffer, and consecutive calls
     * (even with different objects) may Invalidate the previous
     * return value.  May be called only from the main thread.
     */
#ifdef _UNICODE
    gcc_pure
    const TCHAR *GetCodeT() const;
#else
    const char *GetCodeT() const {
      return code;
    }
#endif
  };

  typedef std::list<Item> StationContainer;

  StationContainer stations;

public:
  bool LoadFromString(const char *string);
  bool LoadFromProfile();
  void SaveToProfile();

  typedef StationContainer::iterator iterator;
  typedef StationContainer::const_iterator const_iterator;

  iterator begin() {
    return stations.begin();
  }

  iterator end() {
    return stations.end();
  }

  const_iterator begin() const {
    return stations.begin();
  }

  const_iterator end() const {
    return stations.end();
  }

  void erase(iterator i) {
    stations.erase(i);
  }

  /**
   * Check if the four letter code is valid
   * @param code Four letter code of the station/airport (upper case)
   */
  static bool IsValidCode(const char *code);
#ifdef _UNICODE
  static bool IsValidCode(const TCHAR *code);
#endif

  /**
   * Add a station to the set of stations for which
   * weather information should be downloaded
   * @param code Four letter code of the station/airport (upper case)
   */
  iterator AddStation(const char *code);
#ifdef _UNICODE
  iterator AddStation(const TCHAR *code);
#endif

  /**
   * Returns the amount of stations in the array
   * @return The amount of stations in the array
   */
  gcc_pure
  unsigned Count() const {
    return std::distance(begin(), end());
  }
};

#endif
