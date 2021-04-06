/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Traffic.hpp"
#include "NMEA/Validity.hpp"
#include "util/TrivialArray.hxx"

#include <type_traits>

#ifdef _AUG_MSC
# define MCS_WORKAROUND   1
#else
# define MCS_WORKAROUND   0
#endif

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

  constexpr void Clear() noexcept {
    modified.Clear();
    new_traffic.Clear();
    list.clear();
  }

  constexpr bool IsEmpty() const noexcept {
    return list.empty();
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  constexpr void Complement(const TrafficList &add) noexcept {
    if (IsEmpty() && !add.IsEmpty())
      *this = add;
    // Add unique traffic from 'add' list
    for (auto &traffic : add.list) {
      if (FindTraffic(traffic.id) == nullptr) {
        FlarmTraffic *new_traffic = AllocateTraffic();
        if (new_traffic == nullptr)
          return;
        *new_traffic = traffic;
      }
    }
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    modified.Expire(clock, std::chrono::minutes(5));
    new_traffic.Expire(clock, std::chrono::minutes(1));

    for (size_t i = list.size(); i-- > 0;)
      if (!list[i].Refresh(clock))
        list.quick_remove(i);
  }

  constexpr unsigned GetActiveTrafficCount() const noexcept {
    return list.size();
  }

  /**
   * Looks up an item in the traffic list.
   *
   * @param id FLARM id
   * @return the FLARM_TRAFFIC pointer, NULL if not found
   */
  constexpr FlarmTraffic *FindTraffic(FlarmId id) noexcept {
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
  constexpr const FlarmTraffic *FindTraffic(FlarmId id) const noexcept {
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
  constexpr FlarmTraffic *FindTraffic(const TCHAR *name) noexcept {
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
  constexpr const FlarmTraffic *FindTraffic(const TCHAR *name) const noexcept {
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
  constexpr FlarmTraffic *AllocateTraffic() noexcept {
    return list.full()
      ? NULL
      : &list.append();
  }

  /**
   * Search for the previous traffic in the ordered list.
   */
#if MCS_WORKAROUND
  constexpr const FlarmTraffic *
  PreviousTraffic(const FlarmTraffic *t) const noexcept {
    // August ???? FlarmTraffic *x = t - 1;
//    return (t == &list.front()) ?  nullptr : t - 1;
#if 0
    auto iter = std::find(list.begin(), list.end(), t);
    if (iter > list.begin())
      return &(*(iter - 1));
    else
#endif
      return nullptr;
#else
  constexpr const FlarmTraffic *
  PreviousTraffic(const FlarmTraffic *t) const noexcept {
    return t > list.begin() ? t - 1 : NULL;
#endif
  }

  /**
   * Search for the next traffic in the ordered list.
   */
#if MCS_WORKAROUND
  constexpr const FlarmTraffic *
  NextTraffic(const FlarmTraffic *t) const noexcept {
#if 0
//    return (t == &list.back()) ? t + 1 : nullptr;
    auto iter = std::find(list.begin(), list.end(), t);
    iter++;
    if (iter < list.end())
      return &(*iter);
    else
#endif
      return nullptr;
#else
  constexpr const FlarmTraffic *
  NextTraffic(const FlarmTraffic *t) const noexcept {
    return t + 1 < list.end() ? t + 1 : NULL;
#endif
  }

  /**
   * Search for the first traffic in the ordered list.
   */
  constexpr const FlarmTraffic *FirstTraffic() const noexcept {
#if MCS_WORKAROUND
    return list.empty() ? nullptr : &list.front();
#else
    return list.empty() ? NULL : list.begin();
#endif
  }

  /**
   * Search for the last traffic in the ordered list.
   */
  constexpr const FlarmTraffic *LastTraffic() const noexcept {
#if MCS_WORKAROUND
    return list.empty() ? nullptr : &list.back();
#else
    return list.empty() ? NULL : list.end() - 1;
#endif
  }

  /**
   * Finds the most critical alert.  Returns NULL if there is no
   * alert.
   */
  [[gnu::pure]]
  const FlarmTraffic *FindMaximumAlert() const noexcept;

#if MCS_WORKAROUND
  constexpr unsigned TrafficIndex(const FlarmTraffic *t) const noexcept {
#if 1
    unsigned int i = 0;
    for (auto traffic : list) {
      if (traffic.name == t->name)
        return i;
      i++;
    }
    return 0; // TODO(August2111): This is wrong!!!!
#else
    auto iter = std::find(list.begin(), list.end(), t);
    return iter - list.begin();
#endif
#else
  constexpr unsigned TrafficIndex(const FlarmTraffic *t) const noexcept {
    return t - list.begin();
#endif
  }
};

static_assert(std::is_trivial<TrafficList>::value, "type is not trivial");
