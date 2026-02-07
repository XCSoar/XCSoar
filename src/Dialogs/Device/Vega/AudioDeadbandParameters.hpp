// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
    NULL, 0, 100, 1, "%d %%",
  },
  { DataField::Type::INTEGER, "ToneDeadbandCirclingLow",
    N_("Circling low cutoff"),
    N_("Low limit of circling dead band"),
    NULL, 0, 100, 1, "%d %%",
  },

  { DataField::Type::ENUM, "ToneDeadbandCruiseType",
    N_("Cruise deadband type"),
    N_("Type of dead band used in cruise mode."),
    deadband_types,
  },
  { DataField::Type::INTEGER, "ToneDeadbandCruiseHigh",
    N_("Cruise hi cutoff"),
    N_("High limit of cruise dead band"),
    NULL, 0, 100, 1, "%d %%",
  },
  { DataField::Type::INTEGER, "ToneDeadbandCruiseLow",
    N_("Cruise low cutoff"),
    N_("Low limit of cruise dead band"),
    NULL, 0, 100, 1, "%d %%",
  },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
