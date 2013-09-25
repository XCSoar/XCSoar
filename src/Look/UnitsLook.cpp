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

#include "UnitsLook.hpp"
#include "Resources.hpp"

void
UnitsLook::Initialise()
{
  symbols[(unsigned)Unit::KILOMETER].Load(IDB_UNIT_KM);
  symbols[(unsigned)Unit::NAUTICAL_MILES].Load(IDB_UNIT_NM);
  symbols[(unsigned)Unit::STATUTE_MILES].Load(IDB_UNIT_SM);
  symbols[(unsigned)Unit::KILOMETER_PER_HOUR].Load(IDB_UNIT_KMH);
  symbols[(unsigned)Unit::KNOTS].Load(IDB_UNIT_KT);
  symbols[(unsigned)Unit::STATUTE_MILES_PER_HOUR].Load(IDB_UNIT_MPH);
  symbols[(unsigned)Unit::METER_PER_SECOND].Load(IDB_UNIT_MS);
  symbols[(unsigned)Unit::FEET_PER_MINUTE].Load(IDB_UNIT_FPM);
  symbols[(unsigned)Unit::METER].Load(IDB_UNIT_M);
  symbols[(unsigned)Unit::FEET].Load(IDB_UNIT_FT);
  symbols[(unsigned)Unit::FLIGHT_LEVEL].Load(IDB_UNIT_FL);
  symbols[(unsigned)Unit::KELVIN].Load(IDB_UNIT_DegK);
  symbols[(unsigned)Unit::DEGREES_CELCIUS].Load(IDB_UNIT_DegC);
  symbols[(unsigned)Unit::DEGREES_FAHRENHEIT].Load(IDB_UNIT_DegF);
}
