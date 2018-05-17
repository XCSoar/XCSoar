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

#include "InfoBoxes/Content/Glide.hpp"
#include "Engine/Util/Gradient.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"

#include <tchar.h>

void
UpdateInfoBoxGRInstant(InfoBoxData &data)
{
  const auto gr = CommonInterface::Calculated().gr;

  if (!::GradientValid(gr)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromGlideRatio(gr);
}

void
UpdateInfoBoxGRCruise(InfoBoxData &data)
{
  const auto &basic = CommonInterface::Basic();
  const auto &calculated = CommonInterface::Calculated();
  const auto cruise_gr = calculated.cruise_gr;

  if (!::GradientValid(cruise_gr)) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromGlideRatio(cruise_gr);

  if (basic.location_available)
    data.SetCommentFromDistance(basic.location.DistanceS(calculated.cruise_start_location));
  else
    data.SetCommentInvalid();
}

void
UpdateInfoBoxGRAvg(InfoBoxData &data)
{
  const auto average_gr = CommonInterface::Calculated().average_gr;

  if (average_gr == 0) {
    data.SetInvalid();
    return;
  }

  // Set Value
  if (average_gr < 0)
    data.SetValue(_T("^^^"));
  else if (!::GradientValid(average_gr))
    data.SetValue(_T("+++"));
  else
    data.SetValueFromGlideRatio(average_gr);
}

void
UpdateInfoBoxLDVario(InfoBoxData &data)
{
  const auto ld_vario = CommonInterface::Calculated().ld_vario;

  if (!::GradientValid(ld_vario) ||
      !CommonInterface::Basic().total_energy_vario_available ||
      !CommonInterface::Basic().airspeed_available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromGlideRatio(ld_vario);
}
