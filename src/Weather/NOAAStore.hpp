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

#ifndef NOAA_STORE_HPP
#define NOAA_STORE_HPP

#include "METAR.hpp"
#include "ParsedMETAR.hpp"
#include "TAF.hpp"

#include <list>
#include <tchar.h>

class JobRunner;

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
     * (even with different objects) may invalidate the previous
     * return value.  May be called only from the main thread.
     */
#ifdef _UNICODE
    gcc_pure
    const TCHAR *GetCodeT() const;
#else
    const TCHAR *GetCodeT() const {
      return code;
    }
#endif

    /**
     * Transfers the downloaded METAR into the given reference if available
     *
     * @param metar Destination METAR struct
     * @return True if the data was available,
     * False if no METAR data was available
     */
    bool GetMETAR(METAR &metar) const;

    /**
     * Transfers the downloaded TAF into the given reference if available
     * @param index Index of the station in the array
     * @param metar Destination TAF struct
     * @return True if the data was available,
     * False if no TAF data was available
     */
    bool GetTAF(TAF &taf) const;

    /**
     * Attempts to download new data.
     * @return True if the data was downloaded successfully
     */
    bool Update(JobRunner &runner);
  };

  typedef std::list<Item> StationContainer;

  StationContainer stations;

public:
  bool LoadFromString(const TCHAR *string);
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
   * Add a station to the set of stations for which
   * weather information should be downloaded
   * @param code Four letter code of the station/airport (upper case)
   */
  iterator AddStation(const char *code);
#ifdef _UNICODE
  iterator AddStation(const TCHAR *code);
#endif

  /**
   * Attempts to download new data for all stations
   * @return True if the data for all stations was downloaded successfully
   */
  bool Update(JobRunner &runner);

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
