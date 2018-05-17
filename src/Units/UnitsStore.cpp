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

#include "Units/UnitsStore.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

struct UnitStoreItem
{
  const TCHAR* Name;
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
  } }
};

const TCHAR*
Units::Store::GetName(unsigned i)
{
  assert(i < Count());
  return gettext(Presets[i].Name);
}

const UnitSetting&
Units::Store::Read(unsigned i)
{
  assert(i < Count());
  return Presets[i].Units;
}

unsigned
Units::Store::Count()
{
  return ARRAY_SIZE(Presets);
}

unsigned
Units::Store::EqualsPresetUnits(const UnitSetting &config)
{
  unsigned len = Count();
  for (unsigned i = 0; i < len; i++) {
    // Search for the units, but ignore the coord.format
    if (config == Presets[i].Units) {
      return i+1;
      break;
    }
  }
  return 0;
}
