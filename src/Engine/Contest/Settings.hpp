// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>
#include <cstdint>

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

  /**
   * Deutsche Meisterschaft im Streckensegelflug (Germany).
   */
  DMST,

  WEGLIDE_FREE,
  WEGLIDE_DISTANCE,
  WEGLIDE_FAI,
  WEGLIDE_OR,

  CHARRON,

  NONE,
};

struct ContestSettings {
  /** Whether to do online contest optimisation */
  bool enable;

  /**
   * For the contest score, predict that the aircraft will reach the
   * next turn point?
   */
  bool predict;

  /** Rule set to scan for in contest */
  Contest contest;

  /** Handicap factor */
  unsigned handicap;

  void SetDefaults() noexcept;
};

static_assert(std::is_trivial<ContestSettings>::value, "type is not trivial");
