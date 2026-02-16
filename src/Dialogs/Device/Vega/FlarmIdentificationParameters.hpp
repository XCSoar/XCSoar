// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice flarm_aircraft_types[] = {
  { 0, "Undefined" },
  { 1, "Glider" },
  { 2, "Tow plane" },
  { 3, "Helicopter" },
  { 4, "Parachute" },
  { 5, "Drop plane" },
  { 6, "Fixed hangglider" },
  { 7, "Soft paraglider" },
  { 8, "Powered aircraft" },
  { 9, "Jet aircraft" },
  { 10, "UFO" },
  { 11, "Baloon" },
  { 12, "Blimp, Zeppelin" },
  { 13, "UAV (Drone)" },
  { 14, "Static" },
  { 0 }
};

static constexpr
VegaParametersWidget::StaticParameter flarm_id_parameters[] = {
  { DataField::Type::BOOLEAN, "FlarmPrivacyFlag", N_("Privacy") },
  { DataField::Type::ENUM, "FlarmAircraftType",
    N_("Aircraft type"), NULL, flarm_aircraft_types },

  /* sentinel */
  { DataField::Type::BOOLEAN }
};
