// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "thread/Mutex.hxx"
#include "util/tstring.hpp"

#include <map>
#include <list>
#include <chrono>

#include <cstdint>

namespace SkyLinesTracking {

struct Data {
  using Time = std::chrono::duration<uint_least32_t, std::chrono::milliseconds::period>;

  struct Traffic {
    /**
     * Millisecond of day.
     *
     * @see SkyLinesTracking::TrafficResponsePacket::Traffic::time
     */
    Time time_of_day;

    GeoPoint location;
    int altitude;

    Traffic() = default;
    constexpr Traffic(Time _time, GeoPoint _location,
                      int _altitude) noexcept
      :time_of_day(_time),
       location(_location), altitude(_altitude) {}
  };

  struct Wave {
    /**
     * Millisecond of day.
     *
     * @see SkyLinesTracking::Wave::time
     */
    Time time_of_day;

    /**
     * Two points describing the wave line.  This is the same as the
     * corresponding attributes in #WaveInfo.
     */
    GeoPoint a, b;

    Wave() = default;
    constexpr Wave(Time _time, GeoPoint _a, GeoPoint _b) noexcept
      :time_of_day(_time), a(_a), b(_b) {}
  };

  struct Thermal {
    std::chrono::steady_clock::time_point received_time;

    AGeoPoint bottom_location, top_location;

    double lift;

    Thermal() = default;
    Thermal(const AGeoPoint &_bottom, const AGeoPoint &_top, double _lift)
      :received_time(std::chrono::steady_clock::now()),
       bottom_location(_bottom), top_location(_top), lift(_lift) {}
  };

  mutable Mutex mutex;

  std::map<uint32_t, Traffic> traffic;

  /**
   * A database of user-id to display-name.  An empty string means
   * the server has failed/refused to supply a name.
   */
  std::map<uint32_t, tstring> user_names;

  std::list<Wave> waves;

  std::list<Thermal> thermals;

  [[gnu::pure]]
  bool IsUserKnown(uint32_t id) const {
    return user_names.find(id) != user_names.end();
  }
};

} /* namespace SkyLinesTracking */
