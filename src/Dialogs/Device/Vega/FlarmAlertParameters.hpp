// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice flarm_user_interfaces[] = {
  { 0, "LED+Buzzer" },
  { 1, "None" },
  { 2, "Buzzer" },
  { 3, "LED" },
  { 0 }
};

static constexpr
VegaParametersWidget::StaticParameter flarm_alert_parameters[] = {
  { DataField::Type::INTEGER, "FlarmMaxObjectsReported",
    N_("Max. objects reported"), NULL, NULL, 0, 15, 1, "%d" },
  { DataField::Type::INTEGER, "FlarmMaxObjectsReportedOnCircling",
    N_("Max. reported"), NULL, NULL, 0, 4, 1, "%d" },
  { DataField::Type::ENUM, "FlarmUserInterface",
    N_("Flarm interface"), NULL, flarm_user_interfaces },
  { DataField::Type::BOOLEAN, "KeepOnStraightFlightMode",
    N_("Disable circling") },
  { DataField::Type::BOOLEAN, "DontReportTraficModeChanges",
    N_("No mode reports") },
  { DataField::Type::BOOLEAN, "DontReportGliderType",
    N_("No aircraft type") },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
