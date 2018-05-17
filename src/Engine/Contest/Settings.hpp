/* Copyright_License {

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

#ifndef XCSOAR_CONTEST_SETTINGS_HPP
#define XCSOAR_CONTEST_SETTINGS_HPP

#include <type_traits>

#include <stdint.h>

enum class Contest : uint8_t {
  /**
   * Deprecated.  Use #OLC_LEAGUE instead.
   */
  OLC_SPRINT = 0,

  OLC_FAI,
  OLC_CLASSIC,
  OLC_LEAGUE,
  OLC_PLUS,
  XCONTEST,
  DHV_XC,
  SIS_AT,
  NET_COUPE,

  /**
   * Deutsche Meisterschaft im Streckensegelflug (Germany).
   */
  DMST,

  NONE,
};

struct ContestSettings {
  /** Whether to do online OLC optimisation */
  bool enable;

  /**
   * For the contest score, predict that the aircraft will reach the
   * next turn point?
   */
  bool predict;

  /** Rule set to scan for in OLC */
  Contest contest;

  /** Handicap factor */
  unsigned handicap;

  void SetDefaults();
};

static_assert(std::is_trivial<ContestSettings>::value, "type is not trivial");

#endif
