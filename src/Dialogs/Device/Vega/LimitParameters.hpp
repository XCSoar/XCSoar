// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Language/Language.hpp"

static constexpr
VegaParametersWidget::StaticParameter limit_parameters[] = {
  { DataField::Type::INTEGER, "VelocityNeverExceed", N_("VNE"), NULL,
    NULL, 0, 2000, 10, "%d (0.1 m/s)" },
  { DataField::Type::INTEGER, "VelocitySafeTerrain", N_("V terrain"), NULL,
    NULL, 0, 2000, 10, "%d (0.1 m/s)" },
  { DataField::Type::INTEGER, "VelocitySafeTerrain", N_("Height terrain"),
    NULL, NULL, 0, 2000, 10, "%d m" },
  { DataField::Type::INTEGER, "VelocityManoeuvering", N_("V manoeuvering"),
    NULL, NULL, 0, 2000, 10, "%d (0.1 m/s)" },
  { DataField::Type::INTEGER, "VelocityAirBrake", N_("V airbrake"),
    NULL, NULL, 0, 2000, 10, "%d (0.1 m/s)" },
  { DataField::Type::INTEGER, "VelocityFlap", N_("V flap"),
    NULL, NULL, 0, 2000, 10, "%d (0.1 m/s)" },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
