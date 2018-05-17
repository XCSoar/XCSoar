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

#ifndef XCSOAR_VEGA_DISPLAY_PARAMETERS_HPP
#define XCSOAR_VEGA_DISPLAY_PARAMETERS_HPP

#include "VegaParametersWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice needle_gauge_types[] = {
  { 0, _T("None") },
  { 1, _T("LX") },
  { 2, _T("Analog") },
  { 0 },
};

static constexpr
VegaParametersWidget::StaticParameter display_parameters[] = {
  { DataField::Type::ENUM, "NeedleGaugeType", N_("Needle gauge type"),
    NULL, needle_gauge_types },
  { DataField::Type::INTEGER, "LedBrightness", N_("LED bright"), NULL,
    NULL, 1, 15, 1, _T("%d/15") },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};

#endif
