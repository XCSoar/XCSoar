/*
 Copyright_License {

 XCSoar Glide Computer - http://www.xcsoar.org/
 Copyright (C) 2000-2018 The XCSoar Project
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

#ifndef XCSOAR_GLIDER_LINK_TRAFFIC_LIST_HPP
#define XCSOAR_GLIDER_LINK_TRAFFIC_LIST_HPP

#include "Traffic.hpp"
#include "NMEA/Validity.hpp"
#include "Util/TrivialArray.hxx"

#include <type_traits>

/**
 * This class keeps track of the traffic objects received from a
 * GLIDER_LINK.
 */
struct GliderLinkTrafficList {
  static constexpr size_t MAX_COUNT = 25;

  /**
   * When was the last new traffic received?
   */
  Validity new_traffic;

  /** GliderLink traffic information */
  TrivialArray<GliderLinkTraffic, MAX_COUNT> list;

  void Clear() {
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
  void Complement(const GliderLinkTrafficList &add) {
    if (IsEmpty() && !add.IsEmpty())
      *this = add;
  }

  /**
   * Replaces data in the list
   */
  void Replace(const GliderLinkTrafficList &add) {
    *this = add;
  }

  void Expire(double clock) {
    new_traffic.Expire(clock, double(5*60));

    for (unsigned i = list.size(); i-- > 0;)
      if (!list[i].Refresh(clock))
        list.quick_remove(i);
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id GLIDER_LINK id
   * @return the GLIDER_LINK_TRAFFIC pointer, NULL if not found
   */
  GliderLinkTraffic *FindTraffic(GliderLinkId id) {
    for (auto &traffic : list)
      if (traffic.id == id)
        return &traffic;

    return NULL;
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id GLIDER_LINK id
   * @return the GLIDER_LINK_TRAFFIC pointer, NULL if not found
   */
  const GliderLinkTraffic *FindTraffic(GliderLinkId id) const {
    for (const auto &traffic : list)
      if (traffic.id == id)
        return &traffic;

    return NULL;
  }

  /**
   * Allocates a new GLIDER_LINK_TRAFFIC object from the array.
   *
   * @return the GLIDER_LINK_TRAFFIC pointer, NULL if the array is full
   */
  GliderLinkTraffic *AllocateTraffic() {
    return list.full()
      ? NULL
      : &list.append();
  }
};

static_assert(std::is_trivial<GliderLinkTrafficList>::value, "type is not trivial");

#endif
