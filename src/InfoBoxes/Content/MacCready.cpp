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

#include "InfoBoxes/Content/MacCready.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/MacCreadyEdit.hpp"
#include "InfoBoxes/Panel/MacCreadySetup.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"

#include <tchar.h>

static void
SetVSpeed(InfoBoxData &data, double value)
{
  TCHAR buffer[32];
  FormatUserVerticalSpeed(value, buffer, false);
  data.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  data.SetValueUnit(Units::current.vertical_speed_unit);
}

/*
 * Subpart callback function pointers
 */

static constexpr InfoBoxPanel panels[] = {
  { N_("Edit"), LoadMacCreadyEditPanel },
  { N_("Setup"), LoadMacCreadySetupPanel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentMacCready::GetDialogContent() {
  return panels;
}

/*
 * Subpart normal operations
 */

void
InfoBoxContentMacCready::Update(InfoBoxData &data)
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  data.SetTitle(settings_computer.task.auto_mc ? _("MC AUTO") : _("MC MANUAL"));

  SetVSpeed(data, settings_computer.polar.glide_polar_task.GetMC());

  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  data.SetCommentFromSpeed(common_stats.V_block, false);
}
