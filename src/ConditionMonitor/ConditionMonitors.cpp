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

#include "ConditionMonitors.hpp"
#include "ConditionMonitor.hpp"
#include "ConditionMonitorFinalGlide.hpp"
#include "ConditionMonitorWind.hpp"
#include "Message.hpp"
#include "Device/device.hpp"
#include "Protection.hpp"
#include "Math/SunEphemeris.hpp"
#include "LocalTime.hpp"
#include "InputEvents.hpp"
#include "Computer/GlideComputer.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"

#include <math.h>

class ConditionMonitorSunset: public ConditionMonitor
{
  SunEphemeris sun;

public:
  ConditionMonitorSunset():ConditionMonitor(60 * 30, 60)
  {
  }

protected:
  bool
  CheckCondition(const GlideComputer& cmp)
  {
    if (!cmp.Basic().location_available ||
        !cmp.Calculated().flight.flying || HaveCondorDevice() ||
        !cmp.Calculated().task_stats.task_valid)
      return false;
  
    const GlideResult& res = cmp.Calculated().task_stats.total.solution_remaining;

    /// @todo should be destination location

    sun.CalcSunTimes(cmp.Basic().location, cmp.Basic().date_time_utc,
                     fixed(GetUTCOffset()) / 3600);
    fixed d1((res.time_elapsed + fixed(DetectCurrentTime(cmp.Basic()))) / 3600);
    fixed d0(DetectCurrentTime(cmp.Basic()) / 3600);

    bool past_sunset = (d1 > sun.TimeOfSunSet) && (d0 < sun.TimeOfSunSet);
    return past_sunset;
  }

  void
  Notify(void)
  {
    Message::AddMessage(_("Expect arrival past sunset"));
  }

  void
  SaveLast(void)
  {
  }
};

/** Checks whether arrival time will be less than AAT time */
class ConditionMonitorAATTime: public ConditionMonitor
{
public:
  ConditionMonitorAATTime():ConditionMonitor(60 * 15, 10)
  {
  }

protected:
  bool
  CheckCondition(const GlideComputer& cmp)
  {
    if (!cmp.Calculated().flight.flying ||
        !cmp.Calculated().task_stats.task_valid ||
        !cmp.Calculated().common_stats.mode_ordered ||
        !cmp.Calculated().common_stats.ordered_valid ||
        !cmp.Calculated().common_stats.ordered_has_targets ||
        !cmp.Calculated().common_stats.task_started ||
        !cmp.Calculated().common_stats.active_has_next ||
        cmp.Calculated().common_stats.task_finished)
      return false;

    if (cmp.Calculated().common_stats.task_time_remaining < 
        cmp.Calculated().common_stats.aat_time_remaining)
      return true;
    else
      return false;
  }

  void
  Notify(void)
  {
    Message::AddMessage(_("Expect early task arrival"));
  }

  void
  SaveLast(void)
  {
  }
};

/**
 * Checks whether aircraft in start sector is within height/speed rules
 */
class ConditionMonitorStartRules: public ConditionMonitor
{
  bool withinMargin;

public:
  ConditionMonitorStartRules()
    :ConditionMonitor(60, 1), withinMargin(false)
  {
  }

protected:
  bool
  CheckCondition(const GlideComputer& cmp)
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
  Notify(void)
  {
    if (withinMargin)
      Message::AddMessage(_("Start rules slightly violated\nbut within margin"));
    else
      Message::AddMessage(_("Start rules violated"));
  }

  void
  SaveLast(void)
  {
  }
};

class ConditionMonitorGlideTerrain: public ConditionMonitor
{
public:
  ConditionMonitorGlideTerrain():ConditionMonitor(60 * 5, 1)
  {
  }

protected:
  bool
  CheckCondition(const GlideComputer& cmp)
  {
    if (!cmp.Calculated().flight.flying ||
        !cmp.Calculated().task_stats.task_valid)
      return false;

    const GlideResult& res = cmp.Calculated().task_stats.total.solution_remaining;
    if (!res.IsFinalGlide() || !res.IsAchievable(true)) {
      // only give message about terrain warnings if above final glide
      return false;
    }

    const GeoPoint null_point(Angle::zero(),
                              Angle::zero());
    return (cmp.Calculated().terrain_warning);
  }

  void
  Notify(void)
  {
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
  }

  void
  SaveLast(void)
  {
  }
};


class ConditionMonitorLandableReachable: public ConditionMonitor
{
  bool last_reachable;
  bool now_reachable;

public:
  ConditionMonitorLandableReachable()
    :ConditionMonitor(60 * 5, 1), last_reachable(false)
  {
  }

protected:
  bool
  CheckCondition(const GlideComputer& cmp)
  {
    if (!cmp.Calculated().flight.flying)
      return false;

    now_reachable = cmp.Calculated().common_stats.landable_reachable;

    if (!now_reachable && last_reachable) {
      // warn when becoming unreachable
      return true;
    } else {
      return false;
    }
  }

  void
  Notify(void)
  {
    InputEvents::processGlideComputer(GCE_LANDABLE_UNREACHABLE);
  }

  void
  SaveLast(void)
  {
    last_reachable = now_reachable;
  }
};


ConditionMonitorWind cm_wind;
ConditionMonitorFinalGlide cm_finalglide;
ConditionMonitorSunset cm_sunset;
ConditionMonitorAATTime cm_aattime;
ConditionMonitorStartRules cm_startrules;
ConditionMonitorGlideTerrain cm_glideterrain;
ConditionMonitorLandableReachable cm_landablereachable;
void
ConditionMonitorsUpdate(const GlideComputer& cmp)
{
  cm_wind.Update(cmp);
  cm_finalglide.Update(cmp);
  cm_sunset.Update(cmp);
  cm_aattime.Update(cmp);
  cm_startrules.Update(cmp);
  cm_glideterrain.Update(cmp);
  cm_landablereachable.Update(cmp);
}
