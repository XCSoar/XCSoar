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

#include "Util/StaticArray.hpp"
#include "METAR.hpp"
#include "TAF.hpp"

#include <tchar.h>

struct METAR;
struct ParsedMETAR;
struct TAF;
class JobRunner;

class NOAAStore
{
  struct Item
  {
    char code[5];

    bool metar_available;
    bool taf_available;

    METAR metar;
    TAF taf;
  };

  typedef StaticArray<Item, 20> StationContainer;

  StationContainer stations;

  gcc_pure
  int GetStationIndex(const char *code) const;

public:
  bool LoadFromString(const TCHAR *string);
  bool LoadFromProfile();
  void SaveToProfile();

  /**
   * Add a station to the set of stations for which
   * weather information should be downloaded
   * @param code Four letter code of the station/airport (upper case)
   */
  void AddStation(const char *code);
#ifdef _UNICODE
  void AddStation(const TCHAR *code);
#endif

  /**
   * Removes a station from the set of stations for which
   * weather information should be downloaded
   * @param index Index of the station in the array
   */
  void RemoveStation(unsigned index);
  /**
   * Removes a station from the set of stations for which
   * weather information should be downloaded
   * @param code Four letter code of the station/airport (upper case)
   */
  void RemoveStation(const char *code);

  /**
   * Returns the four letter code of the station given by the array index
   * @param index Index of the station in the array
   * @return Four letter code of the station/airport
   */
  gcc_pure
  const char *GetCode(unsigned index) const;

  /**
   * Returns the four letter code as a TCHAR string.  This may return
   * a pointer to a static buffer, and consecutive calls (even with
   * different objects) may invalidate the previous return value.  May
   * be called only from the main thread.
   */
  const TCHAR *GetCodeT(unsigned index) const;

  /**
   * Transfers the downloaded METAR into the given reference if available
   * @param index Index of the station in the array
   * @param metar Destination METAR struct
   * @return True if the data was available,
   * False if no METAR data was available
   */
  bool GetMETAR(unsigned index, METAR &metar) const;

  /**
   * Transfers the downloaded METAR into the given reference if available
   * @param code Four letter code of the station/airport (upper case)
   * @param metar Destination METAR struct
   * @return True if the data was available,
   * False if no METAR data was available
   */
  bool GetMETAR(const char *code, METAR &metar) const;

  /**
   * Transfers the downloaded TAF into the given reference if available
   * @param index Index of the station in the array
   * @param metar Destination TAF struct
   * @return True if the data was available,
   * False if no TAF data was available
   */
  bool GetTAF(unsigned index, TAF &taf) const;

  /**
   * Transfers the downloaded TAF into the given reference if available
   * @param code Four letter code of the station/airport (upper case)
   * @param metar Destination TAF struct
   * @return True if the data was available,
   * False if no TAF data was available
   */
  bool GetTAF(const char *code, TAF &taf) const;

  /**
   * Attempts to download new data for the given station
   * @param index Index of the station in the array
   * @return True if the data was downloaded successfully
   */
  bool UpdateStation(unsigned index, JobRunner &runner);

  /**
   * Attempts to download new data for the given station
   * @param code Four letter code of the station/airport (upper case)
   * @return True if the data was downloaded successfully
   */
  bool UpdateStation(const char *code, JobRunner &runner);
#ifdef _UNICODE
  bool UpdateStation(const TCHAR *code, JobRunner &runner);
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
  unsigned Count() const;

  /**
   * Returns whether the station array is already full
   */
  gcc_pure
  bool Full() const;
};

#endif
