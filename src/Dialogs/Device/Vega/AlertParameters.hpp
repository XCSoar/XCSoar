// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Language/Language.hpp"

static constexpr
VegaParametersWidget::StaticParameter alert_parameters[] = {
  { DataField::Type::INTEGER, "GearOnDelay",
    N_("Gear on delay"), NULL,
    NULL, 1, 2000, 50, "%d ms" },
  { DataField::Type::INTEGER, "GearOffDelay",
    N_("Gear off delay"), NULL,
    NULL, 1, 2000, 50, "%d ms" },
  { DataField::Type::INTEGER, "GearRepeatTime",
    N_("Interval; gear"), NULL,
    NULL, 1, 100, 1, "%d s" },
  { DataField::Type::INTEGER, "PlyMaxComDelay",
    N_("Radio com. max. delay"), NULL,
    NULL, 0, 100, 1, "%d s" },
  { DataField::Type::INTEGER, "BatLowDelay",
    N_("Battery low delay"), NULL,
    NULL, 0, 100, 1, "%d s" },
  { DataField::Type::INTEGER, "BatEmptyDelay",
    N_("Battery empty delay"), NULL,
    NULL, 0, 100, 1, "%d s" },
  { DataField::Type::INTEGER, "BatRepeatTime",
    N_("Interval; battery"), NULL,
    NULL, 0, 100, 1, "%d s" },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
