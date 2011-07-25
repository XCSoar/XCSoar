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

#include "ConditionMonitor.hpp"
#include "Message.hpp"
#include "Device/device.hpp"
#include "Protection.hpp"
#include "Math/SunEphemeris.hpp"
#include "LocalTime.hpp"
#include "InputEvents.hpp"
#include "GlideComputer.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"

#include <math.h>

// TODO: JMW: make this use GPSClock (code re-use)

/**
 * Base class for system to monitor changes in state and issue
 * warnings or informational messages based on various conditions.
 */
class ConditionMonitor
{
protected:
  fixed LastTime_Notification;
  fixed LastTime_Check;
  fixed Interval_Notification;
  fixed Interval_Check;

public:
  ConditionMonitor(unsigned _interval_notification,
                   unsigned _interval_check)
    :LastTime_Notification(-1), LastTime_Check(-1),
     Interval_Notification(_interval_notification),
     Interval_Check(_interval_check)
  {
  }

  void
  Update(const GlideComputer& cmp)
  {
    if (!cmp.Calculated().flight.Flying)
      return;

    bool restart = false;
    const fixed Time = cmp.Basic().Time;
    if (Ready_Time_Check(Time, &restart)) {
      LastTime_Check = Time;
      if (CheckCondition(cmp)) {
        if (Ready_Time_Notification(Time) && !restart) {
          LastTime_Notification = Time;
          Notify();
          SaveLast();
        }
      }

      if (restart)
        SaveLast();
    }
  }

private:

  virtual bool CheckCondition(const GlideComputer& cmp) = 0;
  virtual void Notify(void) = 0;
  virtual void SaveLast(void) = 0;

  bool
  Ready_Time_Notification(fixed T)
  {
    if (!positive(T))
      return false;

    if (negative(LastTime_Notification) || T < LastTime_Notification)
      return true;

    if (T >= LastTime_Notification + Interval_Notification)
      return true;

    return false;
  }

  bool
  Ready_Time_Check(fixed T, bool *restart)
  {
    if (!positive(T))
      return false;

    if (negative(LastTime_Check) || T < LastTime_Check) {
      LastTime_Notification = fixed_minus_one;
      *restart = true;
      return true;
    }

    if (T >= LastTime_Check + Interval_Check)
      return true;

    return false;
  }
};

/**
 * #ConditionMonitor to track/warn on significant changes in wind speed
 * 
 */
class ConditionMonitorWind: public ConditionMonitor
{
  SpeedVector wind;
  SpeedVector last_wind;

public:
  ConditionMonitorWind():ConditionMonitor(60 * 5, 10)
  {
  }

protected:
  bool
  CheckCondition(const GlideComputer& cmp)
  {
    wind = cmp.Calculated().wind;

    if (!cmp.Calculated().flight.Flying) {
      last_wind = wind;
      return false;
    }

    fixed mag_change = fabs(wind.norm - last_wind.norm);
    fixed dir_change = (wind.bearing - last_wind.bearing).as_delta().magnitude_degrees();

    if (mag_change > Units::ToSysUnit(fixed(5), unKnots))
      return true;

    if ((wind.norm > Units::ToSysUnit(fixed(10), unKnots)) &&
        (dir_change > fixed(45)))
      return true;

    return false;
  }

  void
  Notify(void)
  {
    Message::AddMessage(_("Significant wind change"));
  }

  void
  SaveLast(void)
  {
    last_wind = wind;
  }
};

class ConditionMonitorFinalGlide: public ConditionMonitor
{
  fixed tad;
  fixed last_tad;

public:
  ConditionMonitorFinalGlide()
    :ConditionMonitor(60 * 5, 1), tad(fixed_zero)
  {
  }

protected:
  bool
  CheckCondition(const GlideComputer& cmp)
  {
    if (!cmp.Calculated().flight.Flying || !cmp.Calculated().task_stats.task_valid)
      return false;

    const GlideResult& res = cmp.Calculated().task_stats.total.solution_remaining;

    // TODO: use low pass filter
    tad = res.AltitudeDifference * fixed(0.2) + fixed(0.8) * tad;

    bool BeforeFinalGlide = !res.is_final_glide();

    if (BeforeFinalGlide) {
      Interval_Notification = fixed(60 * 5);
      if ((tad > fixed(50)) && (last_tad < fixed(-50)))
        // report above final glide early
        return true;
      else if (tad < fixed(-50))
        last_tad = tad;
    } else {
      Interval_Notification = fixed(60);
      if (res.is_final_glide()) {
        if ((last_tad < fixed(-50)) && (tad > fixed_one))
          // just reached final glide, previously well below
          return true;

        if ((last_tad > fixed_one) && (tad < fixed(-50))) {
          // dropped well below final glide, previously above
          last_tad = tad;
          return true; // JMW this was true before
        }
      }
    }
    return false;
  }

  void
  Notify(void)
  {
    if (tad > fixed_one)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_ABOVE);
    if (tad < fixed(-1))
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_BELOW);
  }

  void
  SaveLast(void)
  {
    last_tad = tad;
  }
};

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
    if (!cmp.Basic().LocationAvailable ||
        !cmp.Calculated().flight.Flying || HaveCondorDevice() ||
        !cmp.Calculated().task_stats.task_valid)
      return false;
  
    const GlideResult& res = cmp.Calculated().task_stats.total.solution_remaining;

    /// @todo should be destination location

    sun.CalcSunTimes(cmp.Basic().Location, cmp.Basic().DateTime,
                     fixed(GetUTCOffset()) / 3600);
    fixed d1((res.TimeElapsed + fixed(DetectCurrentTime(cmp.Basic()))) / 3600);
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
    if (!cmp.Calculated().flight.Flying ||
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
        || !cmp.Basic().Flying
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
    if (!cmp.Calculated().flight.Flying ||
        !cmp.Calculated().task_stats.task_valid)
      return false;

    const GlideResult& res = cmp.Calculated().task_stats.total.solution_remaining;
    if (!res.is_final_glide() || !res.glide_reachable(true)) {
      // only give message about terrain warnings if above final glide
      return false;
    }

    const GeoPoint null_point(Angle::zero(),
                              Angle::zero());
    return (cmp.Calculated().TerrainWarning);
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
    if (!cmp.Calculated().flight.Flying)
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
