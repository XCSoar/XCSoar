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

#include "AirspaceFormatter.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Util/Macros.hpp"

static const TCHAR *const airspace_class_names[] = {
  _T("Unknown"),
  _T("Restricted"),
  _T("Prohibited"),
  _T("Danger Area"),
  _T("Class A"),
  _T("Class B"),
  _T("Class C"),
  _T("Class D"),
  _T("No Gliders"),
  _T("CTR"),
  _T("Wave"),
  _T("Task Area"),
  _T("Class E"),
  _T("Class F"),
  _T("Transponder Mandatory Zone"),
  _T("Class G"),
  _T("Military Aerodrome Traffic Zone"),
  _T("Radio Mandatory Zone"),
};

static_assert(ARRAY_SIZE(airspace_class_names) ==
              (size_t)AirspaceClass::AIRSPACECLASSCOUNT,
              "number of airspace class names does not match number of "
              "airspace classes");

static const TCHAR *const airspace_class_short_names[] = {
  _T("?"),
  _T("R"),
  _T("P"),
  _T("Q"),
  _T("A"),
  _T("B"),
  _T("C"),
  _T("D"),
  _T("GP"),
  _T("CTR"),
  _T("W"),
  _T("AAT"),
  _T("E"),
  _T("F"),
  _T("TMZ"),
  _T("G"),
  _T("MATZ"),
  _T("RMZ"),
};

static_assert(ARRAY_SIZE(airspace_class_short_names) ==
              (size_t)AirspaceClass::AIRSPACECLASSCOUNT,
              "number of airspace class short names does not match number of "
              "airspace classes");

const TCHAR *
AirspaceFormatter::GetClass(AirspaceClass airspace_class)
{
  unsigned i = (unsigned)airspace_class;

  return i < ARRAY_SIZE(airspace_class_names) ?
         airspace_class_names[i] : NULL;
}

const TCHAR *
AirspaceFormatter::GetClassShort(AirspaceClass airspace_class)
{
  unsigned i = (unsigned)airspace_class;

  return i < ARRAY_SIZE(airspace_class_short_names) ?
         airspace_class_short_names[i] : NULL;
}

const TCHAR *
AirspaceFormatter::GetClass(const AbstractAirspace &airspace)
{
  return GetClass(airspace.GetType());
}

const TCHAR *
AirspaceFormatter::GetClassShort(const AbstractAirspace &airspace)
{
  return GetClassShort(airspace.GetType());
}
