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

#include "UnitsStore.hpp"
#include "Units.hpp"
#include "Language.hpp"

struct UnitStoreItem
{
  const TCHAR* Name;
  UnitSetting Units;
};

static const UnitStoreItem Presets[] =
{
  { N_("European"), {
    unKiloMeter,
    unMeter,
    unGradCelcius,
    unKiloMeterPerHour,
    unMeterPerSecond,
    unKiloMeterPerHour,
    unKiloMeterPerHour
  } },
  { N_("British"), {
    unKiloMeter,
    unFeet,
    unGradCelcius,
    unKnots,
    unKnots,
    unKnots,
    unKiloMeterPerHour
  } },
  { N_("American"), {
    unStatuteMiles,
    unFeet,
    unGradFahrenheit,
    unKnots,
    unKnots,
    unKnots,
    unStatuteMilesPerHour
  } },
  { N_("Australian"), {
    unKiloMeter,
    unFeet,
    unGradCelcius,
    unKnots,
    unKnots,
    unKnots,
    unKiloMeterPerHour
  } }
};

const TCHAR*
Units::Store::GetName(unsigned i)
{
  assert(i < Count());
  return gettext(Presets[i].Name);
}

const UnitSetting&
Units::Store::Read(unsigned i)
{
  assert(i < Count());
  return Presets[i].Units;
}

unsigned
Units::Store::Count()
{
  return sizeof(Presets) / sizeof(Presets[0]);
}
