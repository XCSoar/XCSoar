// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Unit.hpp"

namespace Units {

/**
 * Converts a value from the system unit to the user-specified unit
 * @param value The value in system unit
 * @param Unit The destination unit
 * @return The value in user-specified unit
 */
[[gnu::const]]
double
ToUserUnit(double value, Unit unit) noexcept;

/**
 * Converts a value from the user-specified unit to the system unit
 * @param value The value in user-specified unit
 * @param Unit The source unit
 * @return The value in system unit
 */
[[gnu::const]]
double
ToSysUnit(double value, Unit unit) noexcept;

} // namespace Units
