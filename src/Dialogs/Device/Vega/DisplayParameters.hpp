// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
