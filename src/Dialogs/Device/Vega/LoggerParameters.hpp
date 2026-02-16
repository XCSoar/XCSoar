// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Language/Language.hpp"

static constexpr
VegaParametersWidget::StaticParameter logger_parameters[] = {
  { DataField::Type::INTEGER, "UTCOffset",
    N_("UTC offset"), NULL,
    NULL, -13, 13, 1, "%d",
  },
  { DataField::Type::BOOLEAN, "IGCLoging",
    N_("IGC logging"),
  },
  { DataField::Type::INTEGER, "IGCLoggerInterval",
    N_("Logger interval"), NULL,
    NULL, 1, 12, 1, "%d",
  },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
