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

#include "TaskConstraintsMonitor.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"
#include "Formatter/UserUnits.hpp"
#include "Util/StaticString.hxx"

gcc_pure
static bool
InStartSector()
{
  const auto &calculated = CommonInterface::Calculated();

  return calculated.flight.flying &&
    calculated.ordered_task_stats.task_valid &&
    calculated.ordered_task_stats.active_index == 0 &&
    calculated.ordered_task_stats.inside_oz;
}

void
TaskConstraintsMonitor::Check()
{
  const auto &basic = CommonInterface::Basic();

  if (InStartSector()) {
    const auto settings = protected_task_manager->GetOrderedTaskSettings();

    if (basic.ground_speed_available &&
        !settings.start_constraints.CheckSpeed(basic.ground_speed) &&
        max_start_speed_clock.CheckUpdate(30000)) {
      StaticString<256> msg;
      msg.Format(_T("%s (%s > %s)"), _("Maximum start speed exceeded"),
                 FormatUserSpeed(basic.ground_speed).c_str(),
                 FormatUserSpeed(settings.start_constraints.max_speed).c_str());
      Message::AddMessage(msg);
    }
  } else {
    max_start_speed_clock.Reset();
  }
}
