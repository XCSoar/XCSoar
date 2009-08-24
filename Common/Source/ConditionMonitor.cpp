/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

//#include "ConditionMonitor.h"
#include "XCSoar.h"
#include "Calculations.h"
#include "Math/Geometry.hpp"
#include "Math/Units.h"
#include "Dialogs.h"
#include "Task.h"
#include "Device/device.h"
#include "SettingsTask.hpp"
#include "Utils.h"
#include "Process.h"
#include "InputEvents.h"
#include "WayPoint.hpp"

class ConditionMonitor {
public:
  ConditionMonitor() {
    LastTime_Notification = -1;
    LastTime_Check = -1;
  }

  void Update(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
    if (!Calculated->Flying)
      return;

    bool restart = false;
    if (Ready_Time_Check(Basic->Time, &restart)) {
      LastTime_Check = Basic->Time;
      if (CheckCondition(Basic, Calculated)) {
	if (Ready_Time_Notification(Basic->Time) && !restart) {
	  LastTime_Notification = Basic->Time;
	  Notify();
	  SaveLast();
	}
      }
      if (restart) {
        SaveLast();
      }
    }
  }

protected:
  double LastTime_Notification;
  double LastTime_Check;
  double Interval_Notification;
  double Interval_Check;

private:

  virtual bool CheckCondition(NMEA_INFO *Basic, DERIVED_INFO *Calculated) = 0;
  virtual void Notify(void) = 0;
  virtual void SaveLast(void) = 0;

  bool Ready_Time_Notification(double T) {
    if (T<=0) {
      return false;
    }
    if ((T<LastTime_Notification) || (LastTime_Notification== -1)) {
      return true;
    }
    if (T>= LastTime_Notification + Interval_Notification) {
      return true;
    }
    return false;
  }

  bool Ready_Time_Check(double T, bool *restart) {
    if (T<=0) {
      return false;
    }
    if ((T<LastTime_Check) || (LastTime_Check== -1)) {
      LastTime_Notification = -1;
      *restart = true;
      return true;
    }
    if (T>= LastTime_Check + Interval_Check) {
      return true;
    }
    return false;
  };

};


///////


class ConditionMonitorWind: public ConditionMonitor {
public:
  ConditionMonitorWind() {
    Interval_Notification = 60*5;
    Interval_Check = 10;
    wind_mag = 0;
    wind_bearing = 0;
  }
protected:

  bool CheckCondition(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

    wind_mag = Calculated->WindSpeed;
    wind_bearing = Calculated->WindBearing;

    if (!Calculated->Flying) {
      last_wind_mag = wind_mag;
      last_wind_bearing = wind_bearing;
      return false;
    }

    double mag_change = fabs(wind_mag - last_wind_mag);
    double dir_change = fabs(AngleLimit180(wind_bearing-last_wind_bearing));
    if (mag_change > 5/TOKNOTS) {
      return true;
    }
    if ((wind_mag>10/TOKNOTS) && (dir_change > 45)) {
      return true;
    }
    return false;
  };

  void Notify(void) {
    DoStatusMessage(TEXT("Significant wind change"));
  };

  void SaveLast(void) {
    last_wind_mag = wind_mag;
    last_wind_bearing = wind_bearing;
  };

private:
  double wind_mag;
  double wind_bearing;
  double last_wind_mag;
  double last_wind_bearing;

};


class ConditionMonitorFinalGlide: public ConditionMonitor {
public:
  ConditionMonitorFinalGlide() {
    Interval_Notification = 60*5;
    Interval_Check = 1;
    tad = 0;
  }
protected:

  bool CheckCondition(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
    if (!Calculated->Flying || !ValidTaskPoint(ActiveWayPoint)) {
      return false;
    }

    tad = Calculated->TaskAltitudeDifference*0.2+0.8*tad;

    bool BeforeFinalGlide =
      (ValidTaskPoint(ActiveWayPoint+1) && !Calculated->FinalGlide);

    if (BeforeFinalGlide) {
      Interval_Notification = 60*5;
      if ((tad>50) && (last_tad< -50)) {
        // report above final glide early
        return true;
      } else if (tad< -50) {
        last_tad = tad;
      }
    } else {
      Interval_Notification = 60;
      if (Calculated->FinalGlide) {
        if ((last_tad< -50) && (tad>1)) {
          // just reached final glide, previously well below
          return true;
        }
        if ((last_tad> 1) && (tad< -50)) {
          // dropped well below final glide, previously above
	  last_tad = tad;
          return true; // JMW this was true before
        }
      }
    }
    return false;
  };

  void Notify(void) {
    if (tad>1) {
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_ABOVE);
    }
    if (tad<-1) {
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_BELOW);
    }
  };

  void SaveLast(void) {
    last_tad = tad;
  };

private:
  double tad;
  double last_tad;
};



class ConditionMonitorSunset: public ConditionMonitor {
public:
  ConditionMonitorSunset() {
    Interval_Notification = 60*30;
    Interval_Check = 60;
  }
protected:

  bool CheckCondition(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
    if (!ValidTaskPoint(ActiveWayPoint) || !Calculated->Flying) {
      return false;
    }

    double sunsettime
      = DoSunEphemeris(
                       WayPointList[Task[ActiveWayPoint].Index].Longitude,
                       WayPointList[Task[ActiveWayPoint].Index].Latitude);
    double d1 = (Calculated->TaskTimeToGo+DetectCurrentTime())/3600;
    double d0 = (DetectCurrentTime())/3600;

    bool past_sunset = (d1>sunsettime) && (d0<sunsettime);

    if (past_sunset && !devIsCondor(devA())) {
      // notify on change only
      return true;
    } else {
      return false;
    }
  };

  void Notify(void) {
    DoStatusMessage(TEXT("Expect arrival past sunset"));
  };

  void SaveLast(void) {
  };

private:
};


class ConditionMonitorAATTime: public ConditionMonitor {
public:
  ConditionMonitorAATTime() {
    Interval_Notification = 60*15;
    Interval_Check = 10;
  }
protected:

  bool CheckCondition(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
    if (!AATEnabled || !ValidTaskPoint(ActiveWayPoint) || TaskIsTemporary()
        || !(Calculated->ValidStart && !Calculated->ValidFinish)
        || !Calculated->Flying) {
      return false;
    }
    bool OnFinalWaypoint = !ValidTaskPoint(ActiveWayPoint);
    if (OnFinalWaypoint) {
      // can't do much about it now, so don't give a warning
      return false;
    }
    if (Calculated->TaskTimeToGo < Calculated->AATTimeToGo) {
      return true;
    } else {
      return false;
    }
  };

  void Notify(void) {
    DoStatusMessage(TEXT("Expect early task arrival"));
  };

  void SaveLast(void) {
  };

private:
};


class ConditionMonitorStartRules: public ConditionMonitor {
public:
  ConditionMonitorStartRules() {
    Interval_Notification = 60;
    Interval_Check = 1;
    withinMargin = false;
  }
protected:

  bool CheckCondition(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
    if (!ValidTaskPoint(ActiveWayPoint) || !Calculated->Flying
        || (ActiveWayPoint>0) || !ValidTaskPoint(ActiveWayPoint+1)) {
      return false;
    }
    if (Calculated->LegDistanceToGo>StartRadius) {
      return false;
    }
    if (ValidStartSpeed(Basic, Calculated, StartMaxSpeedMargin) && InsideStartHeight(Basic, Calculated, StartMaxHeightMargin))
    {
      withinMargin = true;
    } else {
      withinMargin = false;
    }
    return !(ValidStartSpeed(Basic, Calculated)
	     && InsideStartHeight(Basic, Calculated));
  };

  void Notify(void) {
    if (withinMargin)
      DoStatusMessage(TEXT("Start rules slightly violated\r\nbut within margin"));
    else
      DoStatusMessage(TEXT("Start rules violated"));
  };

  void SaveLast(void) {
  };

private:
  bool withinMargin;
};


class ConditionMonitorGlideTerrain: public ConditionMonitor {
public:
  ConditionMonitorGlideTerrain() {
    Interval_Notification = 60*5;
    Interval_Check = 1;
    fgtt = 0;
    fgtt_last = false;
  }

protected:

  bool CheckCondition(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
    if (!Calculated->Flying || !ValidTaskPoint(ActiveWayPoint)) {
      return false;
    }

    fgtt = !((Calculated->TerrainWarningLatitude == 0.0) &&
	     (Calculated->TerrainWarningLongitude == 0.0));

    if (!Calculated->FinalGlide || (Calculated->TaskAltitudeDifference<-50)) {
      fgtt_last = false;
    } else if ((fgtt) && (!fgtt_last)) {
      // just reached final glide, previously well below
      return true;
    }
    return false;
  };

  void Notify(void) {
    InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
  };

  void SaveLast(void) {
    fgtt_last = fgtt;
  };

private:
  bool fgtt;
  bool fgtt_last;
};



ConditionMonitorWind       cm_wind;
ConditionMonitorFinalGlide cm_finalglide;
ConditionMonitorSunset     cm_sunset;
ConditionMonitorAATTime    cm_aattime;
ConditionMonitorStartRules cm_startrules;
ConditionMonitorGlideTerrain cm_glideterrain;

void ConditionMonitorsUpdate(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  cm_wind.Update(Basic, Calculated);
  cm_finalglide.Update(Basic, Calculated);
  cm_sunset.Update(Basic, Calculated);
  cm_aattime.Update(Basic, Calculated);
  cm_startrules.Update(Basic, Calculated);
  cm_glideterrain.Update(Basic, Calculated);
}
