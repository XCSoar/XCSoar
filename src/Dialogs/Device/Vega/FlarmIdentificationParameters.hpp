// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "VegaParametersWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice flarm_aircraft_types[] = {
  { 0, _T("Undefined") },
  { 1, _T("Glider") },
  { 2, _T("Tow plane") },
  { 3, _T("Helicopter") },
  { 4, _T("Parachute") },
  { 5, _T("Drop plane") },
  { 6, _T("Fixed hangglider") },
  { 7, _T("Soft paraglider") },
  { 8, _T("Powered aircraft") },
  { 9, _T("Jet aircraft") },
  { 10, _T("UFO") },
  { 11, _T("Baloon") },
  { 12, _T("Blimp, Zeppelin") },
  { 13, _T("UAV (Drone)") },
  { 14, _T("Static") },
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
