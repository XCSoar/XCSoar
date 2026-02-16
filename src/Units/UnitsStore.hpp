// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct UnitSetting;

/**
 * Namespace to manage units presets.
 */
namespace Units::Store {

[[gnu::const]]
const char *
GetName(unsigned i) noexcept;

[[gnu::const]]
const UnitSetting &
Read(unsigned i) noexcept;

[[gnu::const]]
unsigned
Count() noexcept;

/**
 * Only the units part of the structure is addressed.
 * @return Index + 1 if an equivalent set is found, else 0.
 */
[[gnu::pure]]
unsigned
EqualsPresetUnits(const UnitSetting &config) noexcept;

} // namespace Units::Store
