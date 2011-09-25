/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/UnitSymbol.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Point.hpp"
#include "resource.h"

#include <assert.h>

const RasterPoint
UnitSymbol::get_origin(enum kind kind) const
{
  assert(kind >= 0 && kind < 4);

  RasterPoint origin;
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
  unit_symbols[unFeetPerMinute].load(IDB_UNIT_FPM, 5, 11);
  unit_symbols[unMeter].load(IDB_UNIT_M, 5, 11);
  unit_symbols[unFeet].load(IDB_UNIT_FT, 5, 11);
  unit_symbols[unFlightLevel].load(IDB_UNIT_FL, 5, 11);
  unit_symbols[unKelvin].load(IDB_UNIT_DegK, 5, 11);
  unit_symbols[unGradCelcius].load(IDB_UNIT_DegC, 5, 11);
  unit_symbols[unGradFahrenheit].load(IDB_UNIT_DegF, 5, 11);
}

void
DeinitialiseUnitSymbols()
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


void 
UnitSymbol::draw(Canvas& canvas, PixelScalar x, PixelScalar y) const
{
  const RasterPoint BmpPos = get_origin(UnitSymbol::NORMAL);
  const PixelSize size = get_size();
  canvas.scale_copy(x, y, bitmap,
		    BmpPos.x, BmpPos.y,
		    size.cx, size.cy);
}
