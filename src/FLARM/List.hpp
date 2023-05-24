// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Traffic.hpp"
#include "NMEA/Validity.hpp"
#include "util/TrivialArray.hxx"

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
    if (add.modified.Modified(modified))
      modified = add.modified;

    if (add.new_traffic.Modified(new_traffic))
      new_traffic = add.new_traffic;

    if (list.empty() && !add.list.empty()) {
      /* don't bother merging the two lists, we can simply memcpy()
         it */
      list = add.list;
      return;
    }

    // Add unique traffic from 'add' list
    for (auto &traffic : add.list) {
      if (FindTraffic(traffic.id) == nullptr) {
        FlarmTraffic * new_traffic = AllocateTraffic();
        if (new_traffic == nullptr)
          return;
        *new_traffic = traffic;
      }
    }
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    modified.Expire(clock, std::chrono::minutes(5));
    new_traffic.Expire(clock, std::chrono::minutes(1));

    for (unsigned i = list.size(); i-- > 0;)
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
  constexpr const FlarmTraffic *PreviousTraffic(const FlarmTraffic *t) const noexcept {
    return t > list.begin()
      ? t - 1
      : NULL;
  }

  /**
   * Search for the next traffic in the ordered list.
   */
  constexpr const FlarmTraffic *NextTraffic(const FlarmTraffic *t) const noexcept {
    return t + 1 < list.end()
      ? t + 1
      : NULL;
  }

  /**
   * Search for the first traffic in the ordered list.
   */
  constexpr const FlarmTraffic *FirstTraffic() const noexcept {
    return list.empty() ? NULL : list.begin();
  }

  /**
   * Search for the last traffic in the ordered list.
   */
  constexpr const FlarmTraffic *LastTraffic() const noexcept {
    return list.empty() ? NULL : list.end() - 1;
  }

  /**
   * Finds the most critical alert.  Returns NULL if there is no
   * alert.
   */
  [[gnu::pure]]
  const FlarmTraffic *FindMaximumAlert() const noexcept;

  constexpr unsigned TrafficIndex(const FlarmTraffic *t) const noexcept {
    return t - list.begin();
  }

  /**
   * Is set if traffic is present and closer than 4Km.
   */
  bool InCloseRange() const noexcept;
};

static_assert(std::is_trivial<TrafficList>::value, "type is not trivial");
