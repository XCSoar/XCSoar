// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceClass.hpp"

#include <tchar.h>

class AbstractAirspace;
struct AirspaceAltitude;

namespace AirspaceFormatter {

/** Returns the airspace class as text. */
[[gnu::const]]
const TCHAR *GetClass(AirspaceClass airspace_class);

/** Returns the airspace class as short text. */
[[gnu::const]]
const TCHAR *GetClassShort(AirspaceClass airspace_class);

/** Returns the class of the airspace as text. */
[[gnu::pure]]
const TCHAR *GetClass(const AbstractAirspace &airspace);

/** Returns the class of the airspace as short text. */
[[gnu::pure]]
const TCHAR *GetClassShort(const AbstractAirspace &airspace);

  /** Returns the airspace altitude limit as text with unit. */
  void FormatAltitude(TCHAR *buffer, const AirspaceAltitude &altitude);

  /** Returns the airspace altitude limit as short text with unit. */
  void FormatAltitudeShort(TCHAR *buffer, const AirspaceAltitude &altitude,
                           bool include_unit = true);

  /** Returns the type of the airspace as text. */
[[gnu::pure]]
const TCHAR *GetType(const AbstractAirspace &airspace);
}
