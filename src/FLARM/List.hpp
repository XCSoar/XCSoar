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

#ifndef XCSOAR_FLARM_TRAFFIC_LIST_HPP
#define XCSOAR_FLARM_TRAFFIC_LIST_HPP

#include "Traffic.hpp"
#include "NMEA/Validity.hpp"
#include "Util/TrivialArray.hxx"

#include <type_traits>

/**
 * This class keeps track of the traffic objects received from a
 * FLARM.
 */
struct TrafficList {
  static constexpr size_t MAX_COUNT = 25;

  /**
   * Time stamp of the latest modification to this object.
   */
  Validity modified;

  /**
   * When was the last new traffic received?
   */
  Validity new_traffic;

  /** Flarm traffic information */
  TrivialArray<FlarmTraffic, MAX_COUNT> list;

  void Clear() {
    modified.Clear();
    new_traffic.Clear();
    list.clear();
  }

  bool IsEmpty() const {
    return list.empty();
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const TrafficList &add) {
    if (IsEmpty() && !add.IsEmpty())
      *this = add;
  }

  void Expire(double clock) {
    modified.Expire(clock, 300);
    new_traffic.Expire(clock, 60);

    for (unsigned i = list.size(); i-- > 0;)
      if (!list[i].Refresh(clock))
        list.quick_remove(i);
  }

  unsigned GetActiveTrafficCount() const {
    return list.size();
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id FLARM id
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  FlarmTraffic *FindTraffic(FlarmId id) {
    for (auto &traffic : list)
      if (traffic.id == id)
        return &traffic;

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id FLARM id
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  const FlarmTraffic *FindTraffic(FlarmId id) const {
    for (const auto &traffic : list)
      if (traffic.id == id)
        return &traffic;

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param name the name or call sign
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  FlarmTraffic *FindTraffic(const TCHAR *name) {
    for (auto &traffic : list)
      if (traffic.name.equals(name))
        return &traffic;

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param name the name or call sign
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  const FlarmTraffic *FindTraffic(const TCHAR *name) const {
    for (const auto &traffic : list)
      if (traffic.name.equals(name))
        return &traffic;

    return NULL;
  }

  /**
   * Allocates a new FLARM_TRAFFIC object from the array.
   *
   * @return the FLARM_TRAFFIC pointer, NULL if the array is full
   */
  FlarmTraffic *AllocateTraffic() {
    return list.full()
      ? NULL
      : &list.append();
  }

  /**
   * Search for the previous traffic in the ordered list.
   */
  const FlarmTraffic *PreviousTraffic(const FlarmTraffic *t) const {
    return t > list.begin()
      ? t - 1
      : NULL;
  }

  /**
   * Search for the next traffic in the ordered list.
   */
  const FlarmTraffic *NextTraffic(const FlarmTraffic *t) const {
    return t + 1 < list.end()
      ? t + 1
      : NULL;
  }

  /**
   * Search for the first traffic in the ordered list.
   */
  const FlarmTraffic *FirstTraffic() const {
    return list.empty() ? NULL : list.begin();
  }

  /**
   * Search for the last traffic in the ordered list.
   */
  const FlarmTraffic *LastTraffic() const {
    return list.empty() ? NULL : list.end() - 1;
  }

  /**
   * Finds the most critical alert.  Returns NULL if there is no
   * alert.
   */
  const FlarmTraffic *FindMaximumAlert() const;

  unsigned TrafficIndex(const FlarmTraffic *t) const {
    return t - list.begin();
  }
};

static_assert(std::is_trivial<TrafficList>::value, "type is not trivial");

#endif
