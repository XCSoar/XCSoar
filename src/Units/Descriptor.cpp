// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Units/Descriptor.hpp"
#include "Units/Units.hpp"
#include "Atmosphere/Temperature.hpp"
#include "util/Macros.hpp"

#include <cassert>
#include <cstddef>

//SI to Local Units

const UnitDescriptor Units::unit_descriptors[] = {
  { nullptr, 1, 0 },
  { "km", 0.001, 0 },
  { "NM", 0.000539956803, 0 },
  { "mi", 0.000621371192, 0 },
  { "km/h", 3.6, 0 },
  { "kt", 1.94384449, 0 },
  { "mph", 2.23693629, 0 },
  { "m/s", 1, 0 },
  { "fpm", 196.850394, 0 },
  { "m", 1, 0 },
  { "ft", 3.2808399, 0 },
  { "FL", 0.032808399, 0 },
  { "K", 1, 0 },
  { DEG "C", 1, -CELSIUS_OFFSET },
  { DEG "F", 1.8, -459.67 },
  { "hPa", 1, 0 },
  { "mb", 1, 0 },
  { "mmHg", 0.7500616827041698, 0 },
  { "inHg", 0.0295287441401431, 0 },
  { "kg/m²", 1, 0 },
  { "lb/ft²", 0.204816144, 0 },
  { "kg", 1, 0 },
  { "lb", 2.20462, 0 },
  { "%", 1, 0 },
  { ":1", 1, 0 },
  { "V", 1, 0 },
  { "Hz", 1, 0 },
  { "rpm", 60, 0 },
};

static_assert(ARRAY_SIZE(Units::unit_descriptors) == (size_t)Unit::COUNT,
              "number of unit descriptions does not match number of units");

const char *
Units::GetUnitName(Unit unit) noexcept
{
  const unsigned i = (unsigned)unit;
  assert(i < ARRAY_SIZE(unit_descriptors));
  assert(unit_descriptors[i].name != nullptr);

  return unit_descriptors[i].name;
}
