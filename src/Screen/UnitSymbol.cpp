/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Screen/UnitSymbol.hpp"
#include "resource.h"

#include <assert.h>

const POINT
UnitSymbol::get_origin(enum kind kind) const
{
  assert(kind >= 0 && kind < 4);

  POINT origin;
  origin.x = size.cx * kind;
  origin.y = 0;
  return origin;
}

static UnitSymbol unit_symbols[unCount];

void
LoadUnitSymbols()
{
  unit_symbols[unKiloMeter].load(IDB_UNIT_KM, 5, 11);
  unit_symbols[unNauticalMiles].load(IDB_UNIT_NM, 5, 11);
  unit_symbols[unStatuteMiles].load(IDB_UNIT_SM, 5, 11);
  unit_symbols[unKiloMeterPerHour].load(IDB_UNIT_KMH, 10, 11);
  unit_symbols[unKnots].load(IDB_UNIT_KT, 5, 11);
  unit_symbols[unStatuteMilesPerHour].load(IDB_UNIT_MPH, 10, 11);
  unit_symbols[unMeterPerSecond].load(IDB_UNIT_MS, 5, 11);
  unit_symbols[unFeetPerMinutes].load(IDB_UNIT_FPM, 5, 11);
  unit_symbols[unMeter].load(IDB_UNIT_M, 5, 11);
  unit_symbols[unFeet].load(IDB_UNIT_FT, 5, 11);
  unit_symbols[unFligthLevel].load(IDB_UNIT_FL, 5, 11);
  unit_symbols[unKelvin].load(IDB_UNIT_DegK, 5, 11);
  unit_symbols[unGradCelcius].load(IDB_UNIT_DegC, 5, 11);
  unit_symbols[unGradFahrenheit].load(IDB_UNIT_DegF, 5, 11);
}

void
UnloadUnitSymbols()
{
  for (unsigned i = 0; i < unCount; ++i)
    unit_symbols[i].reset();
}

const UnitSymbol *
GetUnitSymbol(Units_t unit)
{
  assert(unit <= unCount);

  return unit_symbols[unit].defined() ? &unit_symbols[unit] : NULL;
}
