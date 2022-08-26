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

#include "FLARM/Error.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/List.hpp"

#include <type_traits>

/**
 * A container for all data received by a FLARM.
 */
struct FlarmData {
  FlarmError error;

  FlarmVersion version;

  FlarmStatus status;

  TrafficList traffic;

  constexpr bool IsDetected() const noexcept {
    return status.available || !traffic.IsEmpty();
  }

  constexpr void Clear() noexcept {
    error.Clear();
    version.Clear();
    status.Clear();
    traffic.Clear();
  }

  constexpr void Complement(const FlarmData &add) noexcept {
    error.Complement(add.error);
    version.Complement(add.version);
    status.Complement(add.status);
    traffic.Complement(add.traffic);
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    error.Expire(clock);
    version.Expire(clock);
    status.Expire(clock);
    traffic.Expire(clock);
  }
};

static_assert(std::is_trivial<FlarmData>::value, "type is not trivial");
