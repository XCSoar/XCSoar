// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Language/Language.hpp"

static constexpr
VegaParametersWidget::StaticParameter calibration_parameters[] = {
  { DataField::Type::INTEGER, "TotalEnergyMixingRatio",
    N_("TE mixing"),
    N_("Proportion of TE probe value used in total energy mixing with pitot/static total energy."),
    NULL, 0, 8, 1, "%d/8",
  },
  { DataField::Type::INTEGER, "CalibrationAirSpeed",
    N_("ASI cal."),
    N_("Calibration factor applied to measured airspeed to obtain indicated airspeed."),
    NULL, 0, 200, 1, "%d %%",
  },
  { DataField::Type::INTEGER, "CalibrationTEStatic",
    N_("TE static cal."),
    N_("Calibration factor applied to static pressure used in total energy calculation."),
    NULL, 0, 200, 1, "%d %%",
  },
  { DataField::Type::INTEGER, "CalibrationTEDynamic",
    N_("TE dynamic cal."),
    N_("Calibration factor applied to dynamic pressure used in total energy calculation."),
    NULL, 0, 200, 1, "%d %%",
  },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
