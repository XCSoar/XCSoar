// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Unit.hpp"

#include <tchar.h>

struct UnitDescriptor
{
  const char *name;
  double factor_to_user;
  double offset_to_user;
};

/**
 * Namespace to manage unit conversions.
 * internal system units are (metric SI).
 */
namespace Units
{

extern const UnitDescriptor unit_descriptors[];

/**
 * Returns the name of the given Unit
 * @return The name of the given Unit (e.g. "km" or "ft")
 */
[[gnu::const]]
const char *
GetUnitName(Unit unit) noexcept;

};
