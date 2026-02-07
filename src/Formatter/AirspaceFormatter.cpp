// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceFormatter.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "util/Macros.hpp"

static const char *const airspace_class_names[] = {
  "Unknown",
  "Restricted",
  "Prohibited",
  "Danger Area",
  "Class A",
  "Class B",
  "Class C",
  "Class D",
  "No Gliders",
  "CTR",
  "Wave",
  "Task Area",
  "Class E",
  "Class F",
  "Transponder Mandatory Zone",
  "Class G",
  "Military Aerodrome Traffic Zone",
  "Radio Mandatory Zone",
  "Unclassified",
  "TMA",
  "Temporary Reserved Airspace",
  "Temporary Segregated Area",
  "Flight Information Region",
  "Upper Flight Information Region",
  "Air Defense Identification Zone",
  "Aerodrome Traffic Zone",
  "Airway",
  "Military Training Route",
  "Alert Area",
  "Warning Area",
  "Protected Area",
  "Hazardous Area",
  "Gliding Sector",
  "Temporary Reserved Prohibited Area",
  "Terminal Information Zone",
  "Terminal Instrument Approach Procedure Area",
  "Military Training Area",
  "Control Area",
  "Area Control Center Sector",
  "Aerial Sporting Recreational",
  "Overflight Restriction",
  "Military Restricted Area",
  "Temporary Flight Restriction",
  "Visual Flight Rules Sector",
  "Flight Information Sector",
  "Lower Traffic Area",
  "Upper Traffic Area",
  "Aerial Sporting Or Recreational Activity",
  "NOTAM Affected Area",
  "Airspace without type",
  "TRA/TSA Feeding Route",
  "Transponder Recommended Zone",
  "Designated Route for VFR",
};

static_assert(ARRAY_SIZE(airspace_class_names) ==
              (size_t)AirspaceClass::AIRSPACECLASSCOUNT,
              "number of airspace class names does not match number of "
              "airspace classes");

static const char *const airspace_class_short_names[] = {
  "?",
  "R",
  "P",
  "Q",
  "A",
  "B",
  "C",
  "D",
  "GP",
  "CTR",
  "W",
  "AAT",
  "E",
  "F",
  "TMZ",
  "G",
  "MATZ",
  "RMZ",
  "Unclassified",
  "TMA",
  "TTRA",
  "TSA",
  "FIR",
  "UIR",
  "ADIZ",
  "AATZ",
  "AWY",
  "MTR",
  "Alert",
  "Warning",
  "Protected",
  "HTZ",
  "Gld_Sec",
  "TRP",
  "TIZ",
  "TIA",
  "MTA",
  "CTA",
  "ACC_Sec",
  "ASR",
  "OverFl_Restr",
  "MRT",
  "TFR",
  "VFR_Sec",
  "FIS_Sec",
  "LTA",
  "UTA",
  "ASRA",
  "NOTAM",
  "NOTYPE",
  "TRA/TSA",
  "TRZ",
  "VFRROUTE",
};

static_assert(ARRAY_SIZE(airspace_class_short_names) ==
              (size_t)AirspaceClass::AIRSPACECLASSCOUNT,
              "number of airspace class short names does not match number of "
              "airspace classes");

const char *
AirspaceFormatter::GetClass(AirspaceClass airspace_class)
{
  unsigned i = (unsigned)airspace_class;

  return i < ARRAY_SIZE(airspace_class_names) ?
         airspace_class_names[i] : NULL;
}

const char *
AirspaceFormatter::GetClassShort(AirspaceClass airspace_class)
{
  unsigned i = (unsigned)airspace_class;

  return i < ARRAY_SIZE(airspace_class_short_names) ?
         airspace_class_short_names[i] : NULL;
}

const char *
AirspaceFormatter::GetClass(const AbstractAirspace &airspace)
{
  return GetClass(airspace.GetClass());
}

const char *
AirspaceFormatter::GetClassShort(const AbstractAirspace &airspace)
{
  return GetClassShort(airspace.GetClass());
}

const char *
AirspaceFormatter::GetType(const AbstractAirspace &airspace)
{
  return GetClass(airspace.GetType());
}

const char *
AirspaceFormatter::GetClassOrType(const AbstractAirspace &airspace)
{
  return GetClass(airspace.GetClassOrType());
}
