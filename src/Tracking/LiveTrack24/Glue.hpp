/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_LIVETRACK24_GLUE_HPP
#define XCSOAR_LIVETRACK24_GLUE_HPP

#include "Settings.hpp"
#include "Client.hpp"
#include "time/PeriodClock.hpp"
#include "Geo/GeoPoint.hpp"
#include "co/InjectTask.hxx"
#include "time/BrokenDateTime.hpp"

struct MoreData;
struct DerivedInfo;
class CurlGlobal;

namespace LiveTrack24 {

class Glue final {
  struct State
  {
    LiveTrack24::SessionID session_id;
    unsigned packet_id;

    void ResetSession() {
      session_id = 0;
    }

    bool HasSession() {
      return session_id != 0;
    }
  };

  PeriodClock clock;

  Settings settings;

  State state;

  Client client;

  /**
   * The Unix UTC time stamp that was last submitted to the tracking
   * server.  This attribute is used to detect time warps.
   */
  std::chrono::system_clock::time_point last_timestamp{};

  BrokenDateTime date_time;
  GeoPoint location;
  unsigned altitude;
  unsigned ground_speed;
  Angle track;
  bool flying = false, last_flying;

  Co::InjectTask inject_task;

public:
  explicit Glue(CurlGlobal &curl) noexcept;

  void SetSettings(const Settings &_settings);
  void OnTimer(const MoreData &basic, const DerivedInfo &calculated);

protected:
  Co::InvokeTask Tick(Settings settings);
  void OnCompletion(std::exception_ptr error) noexcept;
};

} // namespace Livetrack24

#endif
