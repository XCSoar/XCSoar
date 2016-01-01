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

#ifndef XCSOAR_FLARM_VERSION_HPP
#define XCSOAR_FLARM_VERSION_HPP

#include "NMEA/Validity.hpp"
#include "Util/StaticString.hxx"

#include <type_traits>

/**
 * The FLARM version read from the PFLAV sentence.
 */
struct FlarmVersion {
  Validity available;

  NarrowString<7> hardware_version, software_version;
  NarrowString<19> obstacle_version;

  void Clear() {
    available.Clear();
  }

  void Complement(const FlarmVersion &add) {
    if (available.Complement(add.available)) {
      hardware_version = add.hardware_version;
      software_version = add.software_version;
      obstacle_version = add.obstacle_version;
    }
  }

  void Expire(gcc_unused double clock) {
    /* no expiry; this object will be cleared only when the device
       connection is lost */
  }
};

static_assert(std::is_trivial<FlarmVersion>::value, "type is not trivial");

#endif
