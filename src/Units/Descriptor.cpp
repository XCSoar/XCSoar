/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

//SI to Local Units

const UnitDescriptor Units::unit_descriptors[] = {
  { NULL, fixed(1), fixed(0) },
  { _T("km"), fixed(0.001), fixed(0) },
  { _T("NM"), fixed(0.000539956803), fixed(0) },
  { _T("mi"), fixed(0.000621371192), fixed(0) },
  { _T("km/h"), fixed(3.6), fixed(0) },
  { _T("kt"), fixed(1.94384449), fixed(0) },
  { _T("mph"), fixed(2.23693629), fixed(0) },
  { _T("m/s"), fixed(1), fixed(0) },
  { _T("fpm"), fixed(196.850394), fixed(0) },
  { _T("m"), fixed(1), fixed(0) },
  { _T("ft"), fixed(3.2808399), fixed(0) },
  { _T("FL"), fixed(0.032808399), fixed(0) },
  { _T("K"), fixed(1), fixed(0) },
  { _T(DEG)_T("C"), fixed(1), -CELSIUS_OFFSET },
  { _T(DEG)_T("F"), fixed(1.8), fixed(-459.67) },
  { _T("hPa"), fixed(1), fixed(0) },
  { _T("mb"), fixed(1), fixed(0) },
  { _T("mmHg"), fixed(0.7500616827041698), fixed(0) },
  { _T("inHg"), fixed(0.0295287441401431), fixed(0) },
};

static_assert(ARRAY_SIZE(Units::unit_descriptors) == (size_t)Unit::COUNT,
              "number of unit descriptions does not match number of units");

const TCHAR *
Units::GetUnitName(Unit unit)
{
  const unsigned i = (unsigned)unit;
  assert(i < ARRAY_SIZE(unit_descriptors));
  assert(unit_descriptors[i].name != NULL);

  return unit_descriptors[i].name;
}
