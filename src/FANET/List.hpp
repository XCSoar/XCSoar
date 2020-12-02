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

#ifndef XCSOAR_FANET_LIST_HPP
#define XCSOAR_FANET_LIST_HPP

#include "FanetStation.hpp"
#include "NMEA/Validity.hpp"
#include "util/TrivialArray.hxx"

#include <type_traits>

/**
 * This list keeps track of the stations objects received from FANET.
 */
struct StationsList {
  static constexpr size_t MAX_COUNT = 25;

  /** Fanet Stations information */
  TrivialArray<FanetStation, MAX_COUNT> list;

  void Clear() {
    list.clear();
  }

  bool IsEmpty() const {
    return list.empty();
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const StationsList &add) {
    // Add unique station from 'add' list
    for (auto &station : add.list) {
      if (FindStations(station.address) == nullptr) {
        FanetStation * new_station = AllocateStation();
        if (new_station == nullptr)
          return;
        *new_station = station;
      }
    }
  }

  /**
   * Goes trough all the station in the list and removes expired ones
   *
   * @param clock current time
   */
  void RemoveExpired(double clock) {
    for (unsigned i = list.size(); i-- > 0;)
      if ( ! list[i].IsValid(clock))
        list.remove(i);
  }

  /**
   * Looks up an item in the stations list.
   *
   * @param address FANET address
   * @return the FANET_STATIONS pointer, NULL if not found
   */
  FanetStation *FindStations(FanetAddress address) {
    for (auto &stations : list)
      if (stations.address == address)
        return &stations;

    return nullptr;
  }

  /**
   * Allocates a new FANET_STATIONS object from the array.
   *
   * @return the FANET_STATIONS pointer, NULL if the array is full
   */
  FanetStation *AllocateStation() {
    return list.full()
           ? nullptr
           : &list.append();
  }

};

static_assert(std::is_trivial<StationsList>::value, "type is not trivial");

#endif //XCSOAR_FANET_LIST_HPP
