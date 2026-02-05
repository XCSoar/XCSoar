// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Units/UnitsStore.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "util/Macros.hpp"

#include <cassert>

struct UnitStoreItem
{
  const char* Name;
  UnitSetting Units;
};

static constexpr UnitStoreItem Presets[] =
{
  { N_("European"), {
    Unit::KILOMETER,
    Unit::METER,
    Unit::DEGREES_CELCIUS,
    Unit::KILOMETER_PER_HOUR,
    Unit::METER_PER_SECOND,
    Unit::KILOMETER_PER_HOUR,
    Unit::KILOMETER_PER_HOUR,
    Unit::HECTOPASCAL,
    Unit::KG_PER_M2,
    Unit::KG,
    Unit::RPM,
  } },
  { N_("British"), {
    Unit::KILOMETER,
    Unit::FEET,
    Unit::DEGREES_CELCIUS,
    Unit::KNOTS,
    Unit::KNOTS,
    Unit::KNOTS,
    Unit::KILOMETER_PER_HOUR,
    Unit::MILLIBAR,
    Unit::KG_PER_M2,
    Unit::KG,
    Unit::RPM,
  } },
  { N_("American"), {
    Unit::STATUTE_MILES,
    Unit::FEET,
    Unit::DEGREES_FAHRENHEIT,
    Unit::KNOTS,
    Unit::KNOTS,
    Unit::KNOTS,
    Unit::STATUTE_MILES_PER_HOUR,
    Unit::INCH_MERCURY,
    Unit::LB_PER_FT2,
    Unit::LB,
    Unit::RPM,
  } },
  { N_("Australian"), {
    Unit::KILOMETER,
    Unit::FEET,
    Unit::DEGREES_CELCIUS,
    Unit::KNOTS,
    Unit::KNOTS,
    Unit::KNOTS,
    Unit::KILOMETER_PER_HOUR,
    Unit::HECTOPASCAL,
    Unit::KG_PER_M2,
    Unit::KG,
    Unit::RPM,
  } }
};

const char *
Units::Store::GetName(unsigned i) noexcept
{
  assert(i < Count());
  return gettext(Presets[i].Name);
}

const UnitSetting &
Units::Store::Read(unsigned i) noexcept
{
  assert(i < Count());
  return Presets[i].Units;
}

unsigned
Units::Store::Count() noexcept
{
  return ARRAY_SIZE(Presets);
}

unsigned
Units::Store::EqualsPresetUnits(const UnitSetting &config) noexcept
{
  unsigned len = Count();
  for (unsigned i = 0; i < len; i++) {
    // Search for the units, but ignore the coord.format
    if (config == Presets[i].Units) {
      return i+1;
    }
  }
  return 0;
}
