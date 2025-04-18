// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Traffic.hpp"
#include "NMEA/Validity.hpp"
#include "util/TrivialArray.hxx"

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

  void Expire(TimeStamp clock) noexcept {
    new_traffic.Expire(clock, std::chrono::minutes(5));

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
