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

#ifndef XCSOAR_AIRSPACE_FORMATTER_HPP
#define XCSOAR_AIRSPACE_FORMATTER_HPP

#include "Engine/Airspace/AirspaceClass.hpp"
#include "Compiler.h"

#include <tchar.h>

class AbstractAirspace;
struct AirspaceAltitude;

namespace AirspaceFormatter {

/** Returns the airspace class as text. */
gcc_const
const TCHAR *GetClass(AirspaceClass airspace_class);

/** Returns the airspace class as short text. */
gcc_const
const TCHAR *GetClassShort(AirspaceClass airspace_class);

/** Returns the class of the airspace as text. */
gcc_pure
const TCHAR *GetClass(const AbstractAirspace &airspace);

/** Returns the class of the airspace as short text. */
gcc_pure
const TCHAR *GetClassShort(const AbstractAirspace &airspace);

  /** Returns the airspace altitude limit as text with unit. */
  void FormatAltitude(TCHAR *buffer, const AirspaceAltitude &altitude);

  /** Returns the airspace altitude limit as short text with unit. */
  void FormatAltitudeShort(TCHAR *buffer, const AirspaceAltitude &altitude,
                           bool include_unit = true);
}

#endif
