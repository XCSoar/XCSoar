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
  { _T("km"), 0.001, 0 },
  { _T("NM"), 0.000539956803, 0 },
  { _T("mi"), 0.000621371192, 0 },
  { _T("km/h"), 3.6, 0 },
  { _T("kt"), 1.94384449, 0 },
  { _T("mph"), 2.23693629, 0 },
  { _T("m/s"), 1, 0 },
  { _T("fpm"), 196.850394, 0 },
  { _T("m"), 1, 0 },
  { _T("ft"), 3.2808399, 0 },
  { _T("FL"), 0.032808399, 0 },
  { _T("K"), 1, 0 },
  { _T(DEG) _T("C"), 1, -CELSIUS_OFFSET },
  { _T(DEG) _T("F"), 1.8, -459.67 },
  { _T("hPa"), 1, 0 },
  { _T("mb"), 1, 0 },
  { _T("mmHg"), 0.7500616827041698, 0 },
  { _T("inHg"), 0.0295287441401431, 0 },
  { _T("kg/m²"), 1, 0 },
  { _T("lb/ft²"), 0.204816144, 0 },
  { _T("kg"), 1, 0 },
  { _T("lb"), 2.20462, 0 },
  { _T("%"), 1, 0 },
  { _T(":1"), 1, 0 },
  { _T("V"), 1, 0 },
  { _T("Hz"), 1, 0 },
  { _T("rpm"), 60, 0 },
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
