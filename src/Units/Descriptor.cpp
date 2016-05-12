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

#include "Units/Descriptor.hpp"
#include "Units/Units.hpp"
#include "Atmosphere/Temperature.hpp"
#include "Util/Macros.hpp"

#include <assert.h>
#include <stddef.h>

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
};

static_assert(ARRAY_SIZE(Units::unit_descriptors) == (size_t)Unit::COUNT,
              "number of unit descriptions does not match number of units");

const TCHAR *
Units::GetUnitName(Unit unit)
{
  const unsigned i = (unsigned)unit;
  assert(i < ARRAY_SIZE(unit_descriptors));
  assert(unit_descriptors[i].name != nullptr);

  return unit_descriptors[i].name;
}
