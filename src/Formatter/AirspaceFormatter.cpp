// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceFormatter.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "util/Macros.hpp"

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
  return GetClass(airspace.GetClass());
}

const TCHAR *
AirspaceFormatter::GetClassShort(const AbstractAirspace &airspace)
{
  return GetClassShort(airspace.GetClass());
}

const TCHAR *
AirspaceFormatter::GetType(const AbstractAirspace &airspace)
{
  return airspace.GetType();
}
