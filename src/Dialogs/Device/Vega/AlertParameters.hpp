/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_VEGA_ALERT_PARAMETERS_HPP
#define XCSOAR_VEGA_ALERT_PARAMETERS_HPP

#include "VegaParametersWidget.hpp"
#include "Language/Language.hpp"

static constexpr
VegaParametersWidget::StaticParameter alert_parameters[] = {
  { DataField::Type::INTEGER, "GearOnDelay",
    N_("Gear on delay"), NULL,
    NULL, 1, 2000, 50, _T("%d ms") },
  { DataField::Type::INTEGER, "GearOffDelay",
    N_("Gear off delay"), NULL,
    NULL, 1, 2000, 50, _T("%d ms") },
  { DataField::Type::INTEGER, "GearRepeatTime",
    N_("Interval; gear"), NULL,
    NULL, 1, 100, 1, _T("%d s") },
  { DataField::Type::INTEGER, "PlyMaxComDelay",
    N_("Radio com. max. delay"), NULL,
    NULL, 0, 100, 1, _T("%d s") },
  { DataField::Type::INTEGER, "BatLowDelay",
    N_("Battery low delay"), NULL,
    NULL, 0, 100, 1, _T("%d s") },
  { DataField::Type::INTEGER, "BatEmptyDelay",
    N_("Battery empty delay"), NULL,
    NULL, 0, 100, 1, _T("%d s") },
  { DataField::Type::INTEGER, "BatRepeatTime",
    N_("Interval; battery"), NULL,
    NULL, 0, 100, 1, _T("%d s") },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};

#endif
