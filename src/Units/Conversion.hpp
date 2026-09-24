// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * SI conversion factors shared by Units tables and Engine.
 *
 * Engine must not include Units/Units.hpp (user settings).  These
 * constants match unit_descriptors[] and stay header-only.
 */
namespace Units {

/** metres → feet (also the Unit::FEET factor_to_user) */
constexpr double METERS_TO_FEET = 3.2808399;

constexpr double FEET_TO_METERS = 1.0 / METERS_TO_FEET;

/** one flight level is 100 feet */
constexpr double FEET_PER_FLIGHT_LEVEL = 100;

/** metres → FL (also the Unit::FLIGHT_LEVEL factor_to_user) */
constexpr double METERS_TO_FLIGHT_LEVEL =
  METERS_TO_FEET / FEET_PER_FLIGHT_LEVEL;

constexpr double FLIGHT_LEVEL_TO_METERS =
  FEET_PER_FLIGHT_LEVEL * FEET_TO_METERS;

} // namespace Units
