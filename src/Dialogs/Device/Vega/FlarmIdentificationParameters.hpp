/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_VEGA_FLARM_ID_PARAMETERS_HPP
#define XCSOAR_VEGA_FLARM_ID_PARAMETERS_HPP

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

#endif
