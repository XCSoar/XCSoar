// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Language/Language.hpp"

static constexpr
VegaParametersWidget::StaticParameter flarm_repeat_parameters[] = {
  { DataField::Type::INTEGER, "FlarmInfoRepeatTime",
    N_("Interval; info"), NULL,
    NULL, 1, 2000, 100, "%d ms" },
  { DataField::Type::INTEGER, "FlarmCautionRepeatTime",
    N_("Interval; caution"), NULL,
    NULL, 1, 2000, 100, "%d ms" },
  { DataField::Type::INTEGER, "FlarmWarningRepeatTime",
    N_("Interval; warning"), NULL,
    NULL, 1, 2000, 100, "%d ms" },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
