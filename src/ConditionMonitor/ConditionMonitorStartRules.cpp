/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "ConditionMonitorStartRules.hpp"
#include "Computer/GlideComputer.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"

bool
ConditionMonitorStartRules::CheckCondition(const GlideComputer& cmp)
{
#ifdef OLD_TASK // start condition warnings
  if (!task.Valid()
      || !cmp.Basic().flying
      || (task.getActiveIndex() > 0)
      || !task.ValidTaskPoint(task.getActiveIndex() + 1))
    return false;

  if (cmp.Calculated().LegDistanceToGo > task.getSettings().StartRadius)
    return false;

  if (cmp.ValidStartSpeed(task.getSettings().StartMaxSpeedMargin)
      && cmp.InsideStartHeight(task.getSettings().StartMaxHeightMargin))
    withinMargin = true;
  else
    withinMargin = false;
  }
  return !(cmp.ValidStartSpeed() && cmp.InsideStartHeight());
#else
  return false;
#endif
}

void
ConditionMonitorStartRules::Notify()
{
  if (withinMargin)
    Message::AddMessage(_("Start rules slightly violated\nbut within margin"));
  else
    Message::AddMessage(_("Start rules violated"));
}
