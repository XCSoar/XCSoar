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

#ifndef XCSOAR_VEGA_AUDIO_DEADBAND_PARAMETERS
#define XCSOAR_VEGA_AUDIO_DEADBAND_PARAMETERS

#include "VegaParametersWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice deadband_types[] = {
  { 0, N_("Step") },
  { 1, N_("Ramp") },
  { 0 }
};

static constexpr
VegaParametersWidget::StaticParameter audio_deadband_parameters[] = {
  { DataField::Type::ENUM, "ToneDeadbandCirclingType",
    N_("Circling deadband type"),
    N_("Type of dead band used in circling mode."),
    deadband_types,
  },
  { DataField::Type::INTEGER, "ToneDeadbandCirclingHigh",
    N_("Circling hi cutoff"),
    N_("High limit of circling dead band"),
    NULL, 0, 100, 1, _T("%d %%"),
  },
  { DataField::Type::INTEGER, "ToneDeadbandCirclingLow",
    N_("Circling low cutoff"),
    N_("Low limit of circling dead band"),
    NULL, 0, 100, 1, _T("%d %%"),
  },

  { DataField::Type::ENUM, "ToneDeadbandCruiseType",
    N_("Cruise deadband type"),
    N_("Type of dead band used in cruise mode."),
    deadband_types,
  },
  { DataField::Type::INTEGER, "ToneDeadbandCruiseHigh",
    N_("Cruise hi cutoff"),
    N_("High limit of cruise dead band"),
    NULL, 0, 100, 1, _T("%d %%"),
  },
  { DataField::Type::INTEGER, "ToneDeadbandCruiseLow",
    N_("Cruise low cutoff"),
    N_("Low limit of cruise dead band"),
    NULL, 0, 100, 1, _T("%d %%"),
  },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};

#endif
