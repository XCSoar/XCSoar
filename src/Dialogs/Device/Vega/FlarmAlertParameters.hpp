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

#ifndef XCSOAR_VEGA_FLARM_ALERT_PARAMETERS_HPP
#define XCSOAR_VEGA_FLARM_ALERT_PARAMETERS_HPP

#include "VegaParametersWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"

static constexpr StaticEnumChoice flarm_user_interfaces[] = {
  { 0, _T("LED+Buzzer") },
  { 1, _T("None") },
  { 2, _T("Buzzer") },
  { 3, _T("LED") },
  { 0 }
};

static constexpr
VegaParametersWidget::StaticParameter flarm_alert_parameters[] = {
  { DataField::Type::INTEGER, "FlarmMaxObjectsReported",
    N_("Max. objects reported"), NULL, NULL, 0, 15, 1, _T("%d") },
  { DataField::Type::INTEGER, "FlarmMaxObjectsReportedOnCircling",
    N_("Max. reported"), NULL, NULL, 0, 4, 1, _T("%d") },
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

#endif
