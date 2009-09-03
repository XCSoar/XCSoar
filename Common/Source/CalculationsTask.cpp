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

#include "CalculationsTask.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Settings.hpp"
#include "Math/FastMath.h"
#include "Math/LowPassFilter.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include "Math/Pressure.h"
#include "WayPoint.hpp"
#include "McReady.h"
#include "GlideSolvers.hpp"
#include "Dialogs.h"
#include "GlideComputer.hpp"
#include "InputEvents.h"
#include "GlideRatio.hpp"
#include "CalculationsTerrain.hpp"

#ifdef DEBUG
#define DEBUGTASKSPEED
#endif

static void
CheckTransitionFinalGlide(const NMEA_INFO *Basic,
                          DERIVED_INFO *Calculated)
{
  int FinalWayPoint = getFinalWaypoint();
  // update final glide mode status
  if (((ActiveWayPoint == FinalWayPoint)
       ||(ForceFinalGlide))
      && (ValidTaskPoint(ActiveWayPoint))) {

    if (Calculated->FinalGlide == 0)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);
    Calculated->FinalGlide = 1;
  } else {
    if (Calculated->FinalGlide == 1)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
    Calculated->FinalGlide = 0;
  }

}


static double
SpeedHeight(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
  (void)Basic;
  if (Calculated->TaskDistanceToGo<=0) {
    return 0;
  }

  // Fraction of task distance covered
  double d_fraction = Calculated->TaskDistanceCovered/
    (Calculated->TaskDistanceCovered+Calculated->TaskDistanceToGo);

  double dh_start = Calculated->TaskStartAltitude;

  double dh_finish = FAIFinishHeight(Basic, Calculated, -1);

  // Excess height
  return Calculated->NavAltitude
    - (dh_start*(1.0-d_fraction)+dh_finish*(d_fraction));
}

static void
DebugTaskCalculations(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
#ifdef DEBUGTASKSPEED
  if ((Calculated->TaskStartTime>0)
      && (Basic->Time-Calculated->TaskStartTime>0)) {
      if (Calculated->Flying) {

        double effective_mc = EffectiveMacCready(Basic, Calculated);
        DebugStore("%g %g %g %g %g %g %g %g %g %g %d %g %g # taskspeed\r\n",
                Basic->Time-Calculated->TaskStartTime,
                Calculated->TaskDistanceCovered,
                Calculated->TaskDistanceToGo,
                Calculated->TaskAltitudeRequired,
                Calculated->NavAltitude,
                Calculated->TaskSpeedAchieved,
                Calculated->TaskSpeed,
                Calculated->TaskSpeedInstantaneous,
                MACCREADY,
                effective_mc,
                ActiveWayPoint,
                Calculated->DistanceVario,
                Calculated->GPSVario);
      }
    }
#endif
}

static double
AATCloseBearing(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
  // ensure waypoint goes in direction of track if very close
  double course_bearing;
  DistanceBearing(Task[ActiveWayPoint-1].AATTargetLat,
		  Task[ActiveWayPoint-1].AATTargetLon,
		  Basic->Latitude,
		  Basic->Longitude,
		  NULL, &course_bearing);

  course_bearing = AngleLimit360(course_bearing+
				 Task[ActiveWayPoint].AATTargetOffsetRadial);
  return course_bearing;
}

void
DistanceToNext(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  mutexTaskData.Lock();

  if(ValidTaskPoint(ActiveWayPoint))
    {
      double w1lat, w1lon;
      double w0lat, w0lon;

      w0lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
      DistanceBearing(Basic->Latitude, Basic->Longitude,
                      w0lat, w0lon,
                      &Calculated->WaypointDistance,
                      &Calculated->WaypointBearing);

      Calculated->ZoomDistance = Calculated->WaypointDistance;

      if (AATEnabled && !TaskIsTemporary()
	  && (ActiveWayPoint>0) &&
          ValidTaskPoint(ActiveWayPoint+1)) {

        w1lat = Task[ActiveWayPoint].AATTargetLat;
        w1lon = Task[ActiveWayPoint].AATTargetLon;

        DistanceBearing(Basic->Latitude, Basic->Longitude,
                        w1lat, w1lon,
                        &Calculated->WaypointDistance,
                        &Calculated->WaypointBearing);

        if (Calculated->WaypointDistance>AATCloseDistance()*3.0) {
          Calculated->ZoomDistance = max(Calculated->WaypointDistance,
                                         Calculated->ZoomDistance);
        } else {
	  Calculated->WaypointBearing = AATCloseBearing(Basic, Calculated);
        }

      } else if ((ActiveWayPoint==0) && (ValidTaskPoint(ActiveWayPoint+1))
                 && (Calculated->IsInSector) &&
		 !TaskIsTemporary()) {

        // JMW set waypoint bearing to start direction if in start sector

        if (AATEnabled) {
          w1lat = Task[ActiveWayPoint+1].AATTargetLat;
          w1lon = Task[ActiveWayPoint+1].AATTargetLon;
        } else {
          w1lat = WayPointList[Task[ActiveWayPoint+1].Index].Latitude;
          w1lon = WayPointList[Task[ActiveWayPoint+1].Index].Longitude;
        }

        DistanceBearing(Basic->Latitude, Basic->Longitude,
                        w1lat, w1lon,
                        NULL,
                        &Calculated->WaypointBearing);
      }
    }
  else
    {
      Calculated->ZoomDistance = 0;
      Calculated->WaypointDistance = 0;
      Calculated->WaypointBearing = 0;
    }
  mutexTaskData.Unlock();
}

void
AltitudeRequired(const NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                 const double this_maccready,
                 const double cruise_efficiency)
{
  (void)Basic;
  mutexTaskData.Lock();
  if(ValidTaskPoint(ActiveWayPoint))
    {
      double wp_alt = FAIFinishHeight(Basic, Calculated, ActiveWayPoint);
      double height_above_wp =
	Calculated->NavAltitude + Calculated->EnergyHeight
	- wp_alt;

      Calculated->NextAltitudeRequired =
        GlidePolar::MacCreadyAltitude(this_maccready,
                        Calculated->WaypointDistance,
                        Calculated->WaypointBearing,
                        Calculated->WindSpeed, Calculated->WindBearing,
                        0, 0,
			true,
			NULL, height_above_wp, cruise_efficiency
                        );
      // JMW CHECK FGAMT

	// VENTA6
	if (this_maccready==0 ) Calculated->NextAltitudeRequired0=Calculated->NextAltitudeRequired;
        else
	      Calculated->NextAltitudeRequired0 =
		GlidePolar::MacCreadyAltitude(0,
				Calculated->WaypointDistance,
				Calculated->WaypointBearing,
				Calculated->WindSpeed, Calculated->WindBearing,
				0, 0,
				true,
				NULL, height_above_wp, cruise_efficiency
				);



      Calculated->NextAltitudeRequired += wp_alt;
      Calculated->NextAltitudeRequired0 += wp_alt; // VENTA6

      Calculated->NextAltitudeDifference =
        Calculated->NavAltitude
        + Calculated->EnergyHeight
        - Calculated->NextAltitudeRequired;

      Calculated->NextAltitudeDifference0 =
        Calculated->NavAltitude
        + Calculated->EnergyHeight
        - Calculated->NextAltitudeRequired0;
    }
  else
    {
      Calculated->NextAltitudeRequired = 0;
      Calculated->NextAltitudeDifference = 0;
      Calculated->NextAltitudeDifference0 = 0; // VENTA6
    }
  mutexTaskData.Unlock();
}

static bool
TaskAltitudeRequired(const NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                     double this_maccready, double *Vfinal,
                     double *TotalTime, double *TotalDistance,
                     int *ifinal,
                     const double cruise_efficiency)
{
  int i;
  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;
  double LegTime, LegDistance, LegBearing, LegAltitude;
  bool retval = false;

  // Calculate altitude required from start of task

  bool isfinal=true;
  LegAltitude = 0;
  double TotalAltitude = 0;
  *TotalTime = 0; *TotalDistance = 0;
  *ifinal = 0;

  mutexTaskData.Lock();

  double height_above_finish = FAIFinishHeight(Basic, Calculated, 0)-
    FAIFinishHeight(Basic, Calculated, -1);

  for(i=MAXTASKPOINTS-2;i>=0;i--) {


    if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

    w1lat = WayPointList[Task[i].Index].Latitude;
    w1lon = WayPointList[Task[i].Index].Longitude;
    w0lat = WayPointList[Task[i+1].Index].Latitude;
    w0lon = WayPointList[Task[i+1].Index].Longitude;

    if (AATEnabled) {
      w1lat = Task[i].AATTargetLat;
      w1lon = Task[i].AATTargetLon;
      if (!isfinal) {
        w0lat = Task[i+1].AATTargetLat;
        w0lon = Task[i+1].AATTargetLon;
      }
    }

    DistanceBearing(w1lat, w1lon,
                    w0lat, w0lon,
                    &LegDistance, &LegBearing);

    *TotalDistance += LegDistance;

    LegAltitude =
      GlidePolar::MacCreadyAltitude(this_maccready,
                                    LegDistance,
                                    LegBearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0,
                                    0,
                                    true,
                                    &LegTime,
				    height_above_finish,
				    cruise_efficiency
                                    );

    // JMW CHECK FGAMT
    height_above_finish-= LegAltitude;

    TotalAltitude += LegAltitude;

    if (LegTime<0) {
      mutexTaskData.Unlock();
      return false;
    } else {
      *TotalTime += LegTime;
    }
    if (isfinal) {
      *ifinal = i+1;
      if (LegTime>0) {
        *Vfinal = LegDistance/LegTime;
      }
    }
    isfinal = false;
  }

  if (*ifinal==0) {
    retval = false;
    goto OnExit;
  }

  TotalAltitude += FAIFinishHeight(Basic, Calculated, -1);

  if (!ValidTaskPoint(*ifinal)) {
    Calculated->TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = false;
  } else {
    Calculated->TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = true;
  }
 OnExit:
  mutexTaskData.Unlock();
  return retval;
}

static double
MacCreadyOrAvClimbRate(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated,
                       double this_maccready)
{
  double mc_val = this_maccready;
  bool is_final_glide = false;

  if (Calculated->FinalGlide) {
    is_final_glide = true;
  }

  // when calculating 'achieved' task speed, need to use Mc if
  // not in final glide, or if in final glide mode and using
  // auto Mc, use the average climb rate achieved so far.

  if ((mc_val<0.1) ||
      (Calculated->AutoMacCready &&
       ((AutoMcMode==0) ||
        ((AutoMcMode==2)&&(is_final_glide))
        ))
      ) {

    mutexGlideComputer.Lock();
    if (GlideComputer::flightstats.ThermalAverage.y_ave>0) {
      mc_val = GlideComputer::flightstats.ThermalAverage.y_ave;
    } else if (Calculated->AverageThermal>0) {
      // insufficient stats, so use this/last thermal's average
      mc_val = Calculated->AverageThermal;
    }
    mutexGlideComputer.Unlock();
  }
  return max(0.1, mc_val);

}


void
TaskSpeed(const NMEA_INFO *Basic, DERIVED_INFO *Calculated, 
          const double this_maccready,
          const double cruise_efficiency)
{
  int ifinal;
  static double LastTime = 0;
  static double LastTimeStats = 0;
  double TotalTime=0, TotalDistance=0, Vfinal=0;

  if (!ValidTaskPoint(ActiveWayPoint)) return;
  if (TaskIsTemporary()) return;
  if (Calculated->ValidFinish) return;
  if (!Calculated->Flying) return;

  // in case we leave early due to error
  Calculated->TaskSpeedAchieved = 0;
  Calculated->TaskSpeed = 0;

  if (ActiveWayPoint<=0) { // no task speed before start
    Calculated->TaskSpeedInstantaneous = 0;
    return;
  }

  mutexTaskData.Lock();

  if (TaskAltitudeRequired(Basic, Calculated, this_maccready, &Vfinal,
                           &TotalTime, &TotalDistance, &ifinal, 
			   cruise_efficiency)) {

    double t0 = TotalTime;
    // total time expected for task

    double t1 = Basic->Time-Calculated->TaskStartTime;
    // time elapsed since start

    double d0 = TotalDistance;
    // total task distance

    double d1 = Calculated->TaskDistanceCovered;
    // actual distance covered

    double dr = Calculated->TaskDistanceToGo;
    // distance remaining

    double t2;
    // equivalent time elapsed after final glide

    double d2;
    // equivalent distance travelled after final glide

    double hf = FAIFinishHeight(Basic, Calculated, -1);

    double h0 = Calculated->TaskAltitudeRequiredFromStart-hf;
    // total height required from start (takes safety arrival alt
    // and finish waypoint altitude into account)

    double h1 = max(0,Calculated->NavAltitude-hf);
    // height above target

    double dFinal;
    // final glide distance

    // equivalent speed
    double v2, v1;

    if ((t1<=0) || (d1<=0) || (d0<=0) || (t0<=0) || (h0<=0)) {
      // haven't started yet or not a real task
      Calculated->TaskSpeedInstantaneous = 0;
      //?      Calculated->TaskSpeed = 0;
      goto OnExit;
    }

    // JB's task speed...
    double hx = max(0,SpeedHeight(Basic, Calculated));
    double t1mod = t1-hx/MacCreadyOrAvClimbRate(Basic, Calculated, this_maccready);
    // only valid if flown for 5 minutes or more
    if (t1mod>300.0) {
      Calculated->TaskSpeedAchieved = d1/t1mod;
    } else {
      Calculated->TaskSpeedAchieved = d1/t1;
    }
    Calculated->TaskSpeed = Calculated->TaskSpeedAchieved;

    if (Vfinal<=0) {
      // can't reach target at current mc
      goto OnExit;
    }

    // distance that can be usefully final glided from here
    // (assumes average task glide angle of d0/h0)
    // JMW TODO accuracy: make this more accurate by working out final glide
    // through remaining turnpoints.  This will more correctly account
    // for wind.

    dFinal = min(dr, d0*min(1.0,max(0.0,h1/h0)));

    if (Calculated->ValidFinish) {
      dFinal = 0;
    }

    double dc = max(0,dr-dFinal);
    // amount of extra distance to travel in cruise/climb before final glide

    // equivalent distance to end of final glide
    d2 = d1+dFinal;

    // time at end of final glide
    t2 = t1+dFinal/Vfinal;

    // actual task speed achieved so far
    v1 = d1/t1;

#ifdef OLDTASKSPEED
    // average speed to end of final glide from here
    v2 = d2/t2;
    Calculated->TaskSpeed = max(v1,v2);
#else
    // average speed to end of final glide from here, weighted
    // according to how much extra time would be spent in cruise/climb
    // the closer dc (the difference between remaining distance and
    // final glidable distance) gets to zero, the closer v2 approaches
    // the average speed to end of final glide from here
    // in other words, the more we consider the final glide part to have
    // been earned.

    // this will be bogus at fast starts though...
    if (v1>0) {
      v2 = (d1+dc+dFinal)/(t1+dc/v1+dFinal/Vfinal);
    } else {
      v2 = (d1+dFinal)/(t1+dFinal/Vfinal);
    }
    Calculated->TaskSpeed = v2;
#endif

    double konst = 1.1;
    if (TaskModified)
      {
	konst = 1.0;
      }

    double termikLigaPoints = 0;
    if (d1 > 0)
      {
	termikLigaPoints = konst*(0.015*0.001*d1-(400.0/(0.001*d1))+12.0)*v1*3.6*100.0/(double)Handicap;
      }

    Calculated->TermikLigaPoints = termikLigaPoints;

    if(Basic->Time < LastTime) {
      LastTime = Basic->Time;
    } else if (Basic->Time-LastTime >=1.0) {

      double dt = Basic->Time-LastTime;
      LastTime = Basic->Time;

      // Calculate contribution to average task speed.
      // This is equal to the change in virtual distance
      // divided by the time step

      // This is a novel concept.
      // When climbing at the MC setting, this number should
      // be similar to the estimated task speed.
      // When climbing slowly or when flying off-course,
      // this number will drop.
      // In cruise at the optimum speed in zero lift, this
      // number will be similar to the estimated task speed.

      // A low pass filter is applied so it doesn't jump around
      // too much when circling.

      // If this number is higher than the overall task average speed,
      // it means that the task average speed is increasing.

      // When cruising in sink, this number will decrease.
      // When cruising in lift, this number will increase.

      // Therefore, it shows well whether at any time the glider
      // is wasting time.

      static double dr_last = 0;

      double mc_safe = max(0.1,this_maccready);
      double Vstar = max(1.0,Calculated->VMacCready);
      double vthis = (Calculated->LegDistanceCovered-dr_last)/dt;
      vthis /= AirDensityRatio(Calculated->NavAltitude);

      dr_last = Calculated->LegDistanceCovered;
      double ttg = max(1,Calculated->LegTimeToGo);
      //      double Vav = d0/max(1.0,t0);
      double Vrem = Calculated->LegDistanceToGo/ttg;
      double Vref = // Vav;
	Vrem;
      double sr = -GlidePolar::SinkRate(Vstar);
      double height_diff = max(0,-Calculated->TaskAltitudeDifference);

      if (Calculated->timeCircling>30) {
	mc_safe = max(this_maccready,
		      Calculated->TotalHeightClimb/Calculated->timeCircling);
      }
      // circling percentage during cruise/climb
      double rho_cruise = max(0.0,min(1.0,mc_safe/(sr+mc_safe)));
      double rho_climb = 1.0-rho_cruise;
      double time_climb = height_diff/mc_safe;

      // calculate amount of time in cruise/climb glide
      double rho_c = max(0,min(1,time_climb/ttg));

      if (Calculated->FinalGlide) {
	if (rho_climb>0) {
	  rho_c = max(0,min(1,rho_c/rho_climb));
	}
	if (!Calculated->Circling) {
	  if (Calculated->TaskAltitudeDifference>0) {
	    rho_climb *= rho_c;
	    rho_cruise *= rho_c;
	    // Vref = Vrem;
	  }
	}
      }

      double w_comp = min(10.0,max(-10.0,Calculated->Vario/mc_safe));
      double vdiff = vthis/Vstar + w_comp*rho_cruise + rho_climb;

      if (vthis > SAFTEYSPEED*2) {
	vdiff = 1.0;
	// prevent funny numbers when starting mid-track
      }
      //      Calculated->Experimental = vdiff*100.0;

      vdiff *= Vref;

      if (t1<5) {
        Calculated->TaskSpeedInstantaneous = vdiff;
        // initialise
      } else {
        static int lastActiveWayPoint = 0;
	static double tsi_av = 0;
	static int n_av = 0;
        if ((ActiveWayPoint==lastActiveWayPoint)
	    && (Calculated->LegDistanceToGo>1000.0)
	    && (Calculated->LegDistanceCovered>1000.0)) {

          Calculated->TaskSpeedInstantaneous =
            LowPassFilter(Calculated->TaskSpeedInstantaneous, vdiff, 0.1);

          // update stats
          if(Basic->Time < LastTimeStats) {
            LastTimeStats = Basic->Time;
	    tsi_av = 0;
	    n_av = 0;
          } else if (n_av>=60) {
	    tsi_av/= n_av;

	    mutexGlideComputer.Lock();
            GlideComputer::flightstats.Task_Speed.
              least_squares_update(
                                   max(0,
                                       Basic->Time-Calculated->TaskStartTime)/3600.0,
                                   max(0,min(100.0,tsi_av)));
	    mutexGlideComputer.Unlock();

            LastTimeStats = Basic->Time;
	    tsi_av = 0;
	    n_av = 0;
          }
	  tsi_av += Calculated->TaskSpeedInstantaneous;
	  n_av ++;

        } else {

          Calculated->TaskSpeedInstantaneous =
            LowPassFilter(Calculated->TaskSpeedInstantaneous, vdiff, 0.5);

	  //	  Calculated->TaskSpeedInstantaneous = vdiff;
	  tsi_av = 0;
	  n_av = 0;
	}
        lastActiveWayPoint = ActiveWayPoint;
      }
    }
  }
 OnExit:
  mutexTaskData.Unlock();

}

void
LDNext(const NMEA_INFO *Basic, DERIVED_INFO *Calculated, const double LegToGo)
{
  double height_above_leg = Calculated->NavAltitude+Calculated->EnergyHeight
    - FAIFinishHeight(Basic, Calculated, ActiveWayPoint);

  Calculated->LDNext = UpdateLD(Calculated->LDNext,
                                LegToGo,
                                height_above_leg,
                                0.5);
}

static void
CheckForceFinalGlide(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
  // Auto Force Final Glide forces final glide mode
  // if above final glide...
  if (TaskAborted) {
    ForceFinalGlide = false;
  } else {
    if (AutoForceFinalGlide) {
      if (!Calculated->FinalGlide) {
        if (Calculated->TaskAltitudeDifference>120) {
          ForceFinalGlide = true;
        } else {
          ForceFinalGlide = false;
        }
      } else {
        if (Calculated->TaskAltitudeDifference<-120) {
          ForceFinalGlide = false;
        } else {
          ForceFinalGlide = true;
        }
      }
    }
  }
}

void
TaskStatistics(const NMEA_INFO *Basic, DERIVED_INFO *Calculated,
               const double this_maccready,
               const double cruise_efficiency)
{

  if (!ValidTaskPoint(ActiveWayPoint) ||
      ((ActiveWayPoint>0) && !ValidTaskPoint(ActiveWayPoint-1))) {

    Calculated->LegSpeed = 0;
    Calculated->LegDistanceToGo = 0;
    Calculated->LegDistanceCovered = 0;
    Calculated->LegTimeToGo = 0;

    if (!AATEnabled) {
      Calculated->AATTimeToGo = 0;
    }

    //    Calculated->TaskSpeed = 0;

    Calculated->TaskDistanceToGo = 0;
    Calculated->TaskDistanceCovered = 0;
    Calculated->TaskTimeToGo = 0;
    Calculated->TaskTimeToGoTurningNow = -1;

    Calculated->TaskAltitudeRequired = 0;
    Calculated->TaskAltitudeDifference = 0;
    Calculated->TaskAltitudeDifference0 = 0;

    Calculated->TerrainWarningLatitude = 0.0;
    Calculated->TerrainWarningLongitude = 0.0;

    Calculated->LDFinish = INVALID_GR;
    Calculated->GRFinish = INVALID_GR; // VENTA-ADDON
    Calculated->LDNext = INVALID_GR;

    Calculated->FinalGlide = 0;
    CheckFinalGlideThroughTerrain(Basic, Calculated,
                                  0.0, 0.0);

    // no task selected, so work things out at current heading

    GlidePolar::MacCreadyAltitude(this_maccready, 100.0,
                                  Basic->TrackBearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  &(Calculated->BestCruiseTrack),
                                  &(Calculated->VMacCready),
                                  (Calculated->FinalGlide==1),
                                  NULL, 1.0e6, cruise_efficiency);

    return;
  }

  mutexTaskData.Lock();

  ///////////////////////////////////////////////
  // Calculate Task Distances
  // First calculate distances for this waypoint

  double LegCovered, LegToGo=0;
  double LegDistance, LegBearing=0;
  bool calc_turning_now;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  if (AATEnabled && (ActiveWayPoint>0) &&
      !TaskIsTemporary() && (ValidTaskPoint(ActiveWayPoint+1))) {
    w1lat = Task[ActiveWayPoint].AATTargetLat;
    w1lon = Task[ActiveWayPoint].AATTargetLon;
  } else {
    w1lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    w1lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
  }

  DistanceBearing(Basic->Latitude,
                  Basic->Longitude,
                  w1lat,
                  w1lon,
                  &LegToGo, &LegBearing);

  if (AATEnabled && (ActiveWayPoint>0) && ValidTaskPoint(ActiveWayPoint+1)
      && Calculated->IsInSector && (this_maccready>0.1) &&
      !TaskIsTemporary()) {
    calc_turning_now = true;
  } else {
    calc_turning_now = false;
  }

  if ((ActiveWayPoint<1) || TaskIsTemporary()) {
    LegCovered = 0;
    if (!TaskIsTemporary()) { // RLD if task not started, exclude distance to start point
      LegToGo=0;
    }
   } else {
    if (AATEnabled) {
      // TODO accuracy: Get best range point to here...
      w0lat = Task[ActiveWayPoint-1].AATTargetLat;
      w0lon = Task[ActiveWayPoint-1].AATTargetLon;
    } else {
      w0lat = WayPointList[Task[ActiveWayPoint-1].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint-1].Index].Longitude;
    }

    DistanceBearing(w1lat,
                    w1lon,
                    w0lat,
                    w0lon,
                    &LegDistance, NULL);

    LegCovered = ProjectedDistance(w0lon, w0lat,
                                   w1lon, w1lat,
                                   Basic->Longitude,
                                   Basic->Latitude);

    if ((StartLine==0) && (ActiveWayPoint==1)) {
      // Correct speed calculations for radius
      // JMW TODO accuracy: legcovered replace this with more accurate version
      // LegDistance -= StartRadius;
      LegCovered = max(0,LegCovered-StartRadius);
    }
  }

  Calculated->LegDistanceToGo = LegToGo;
  Calculated->LegDistanceCovered = LegCovered;
  Calculated->TaskDistanceCovered = LegCovered;

  if (Basic->Time > Calculated->LegStartTime) {
    mutexGlideComputer.Lock();
    if (GlideComputer::flightstats.LegStartTime[ActiveWayPoint]<0) {
      GlideComputer::flightstats.LegStartTime[ActiveWayPoint] = Basic->Time;
    }
    mutexGlideComputer.Unlock();
    Calculated->LegSpeed = Calculated->LegDistanceCovered
      / (Basic->Time - Calculated->LegStartTime);
  }

  ///////////////////////////////////////////////////
  // Now add distances for start to previous waypoint

  if (!TaskIsTemporary()) {

    if (!AATEnabled) {
      for(int i=0;i< ActiveWayPoint-1; i++)
        {
          if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

          w1lat = WayPointList[Task[i].Index].Latitude;
          w1lon = WayPointList[Task[i].Index].Longitude;
          w0lat = WayPointList[Task[i+1].Index].Latitude;
          w0lon = WayPointList[Task[i+1].Index].Longitude;

          DistanceBearing(w1lat,
                          w1lon,
                          w0lat,
                          w0lon,
                          &LegDistance, NULL);
          Calculated->TaskDistanceCovered += LegDistance;
        }
    } else if (ActiveWayPoint>0) {
      // JMW added correction for distance covered
      mutexGlideComputer.Lock();
      Calculated->TaskDistanceCovered =
        GlideComputer::aatdistance.DistanceCovered(Basic->Longitude,
                                    Basic->Latitude,
                                    ActiveWayPoint);
      mutexGlideComputer.Unlock();
    }
  }

  ///////////////////////////////////////////////////////////

  CheckTransitionFinalGlide(Basic, Calculated);

  // accumulators
  double TaskAltitudeRequired = 0;
  double TaskAltitudeRequired0 = 0;
  Calculated->TaskDistanceToGo = 0;
  Calculated->TaskTimeToGo = 0;
  Calculated->TaskTimeToGoTurningNow = 0;

  double LegTime0;

  // Calculate Final Glide To Finish

  int FinalWayPoint = getFinalWaypoint();

  double height_above_finish = Calculated->NavAltitude+
    Calculated->EnergyHeight-FAIFinishHeight(Basic, Calculated, -1);

  //////////////////
  // Now add it for remaining waypoints
  int task_index= FinalWayPoint;

  double StartBestCruiseTrack = 0;

  if (!TaskIsTemporary()) {
    while ((task_index>ActiveWayPoint) && (ValidTaskPoint(task_index))) {
      double this_LegTimeToGo;
      bool this_is_final = (task_index==FinalWayPoint)
	|| ForceFinalGlide;

      this_is_final = true; // JMW CHECK FGAMT

      if (AATEnabled) {
	w1lat = Task[task_index].AATTargetLat;
	w1lon = Task[task_index].AATTargetLon;
	w0lat = Task[task_index-1].AATTargetLat;
	w0lon = Task[task_index-1].AATTargetLon;
      } else {
	w1lat = WayPointList[Task[task_index].Index].Latitude;
	w1lon = WayPointList[Task[task_index].Index].Longitude;
	w0lat = WayPointList[Task[task_index-1].Index].Latitude;
	w0lon = WayPointList[Task[task_index-1].Index].Longitude;
      }

      double NextLegDistance, NextLegBearing;

      DistanceBearing(w0lat,
		      w0lon,
		      w1lat,
		      w1lon,
		      &NextLegDistance, &NextLegBearing);

      double LegAltitude = GlidePolar::
	MacCreadyAltitude(this_maccready,
			  NextLegDistance, NextLegBearing,
			  Calculated->WindSpeed,
			  Calculated->WindBearing,
			  0, 0,
			  this_is_final,
			  &this_LegTimeToGo,
			  height_above_finish, cruise_efficiency);

      double LegAltitude0 = GlidePolar::
	MacCreadyAltitude(0,
			  NextLegDistance, NextLegBearing,
			  Calculated->WindSpeed,
			  Calculated->WindBearing,
			  0, 0,
			  true,
			  &LegTime0, 1.0e6, cruise_efficiency
			  );

      if (LegTime0>=0.9*ERROR_TIME) {
	// can't make it, so assume flying at current mc
	LegAltitude0 = LegAltitude;
      }

      TaskAltitudeRequired += LegAltitude;
      TaskAltitudeRequired0 += LegAltitude0;

      Calculated->TaskDistanceToGo += NextLegDistance;
      Calculated->TaskTimeToGo += this_LegTimeToGo;

      if (task_index==1) {
	StartBestCruiseTrack = NextLegBearing;
      }

      if (calc_turning_now) {
	if (task_index == ActiveWayPoint+1) {

	  double NextLegDistanceTurningNow, NextLegBearingTurningNow;
	  double this_LegTimeToGo_turningnow=0;

	  DistanceBearing(Basic->Latitude,
			  Basic->Longitude,
			  w1lat,
			  w1lon,
			  &NextLegDistanceTurningNow,
			  &NextLegBearingTurningNow);

	  GlidePolar::
	    MacCreadyAltitude(this_maccready,
			      NextLegDistanceTurningNow,
			      NextLegBearingTurningNow,
			      Calculated->WindSpeed,
			      Calculated->WindBearing,
			      0, 0,
			      this_is_final,
			      &this_LegTimeToGo_turningnow,
			      height_above_finish, cruise_efficiency);
	  Calculated->TaskTimeToGoTurningNow += this_LegTimeToGo_turningnow;
	} else {
	  Calculated->TaskTimeToGoTurningNow += this_LegTimeToGo;
	}
      }

      height_above_finish-= LegAltitude;

      task_index--;
    }
  }
  ////////////////


  /////// current waypoint, do this last!

  if (AATEnabled && !TaskIsTemporary()
      && (ActiveWayPoint>0) &&
      ValidTaskPoint(ActiveWayPoint+1) && Calculated->IsInSector) {
    if (Calculated->WaypointDistance<AATCloseDistance()*3.0) {
      LegBearing = AATCloseBearing(Basic, Calculated);
    }
  }

  // JMW TODO accuracy: use mc based on risk? no!
  double LegAltitude =
    GlidePolar::MacCreadyAltitude(this_maccready,
                                  LegToGo,
                                  LegBearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  &(Calculated->BestCruiseTrack),
                                  &(Calculated->VMacCready),

				  // (Calculated->FinalGlide==1),
				  true,  // JMW CHECK FGAMT

                                  &(Calculated->LegTimeToGo),
                                  height_above_finish, cruise_efficiency);

  double LegAltitude0 =
    GlidePolar::MacCreadyAltitude(0,
                                  LegToGo,
                                  LegBearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  0,
                                  0,
                                  true,
                                  &LegTime0, 1.0e6, cruise_efficiency
                                  );

  if (Calculated->IsInSector && (ActiveWayPoint==0) && !TaskIsTemporary()) {
    // set best cruise track to first leg bearing when in start sector
    Calculated->BestCruiseTrack = StartBestCruiseTrack;
  }

  // JMW TODO accuracy: Use safetymc where appropriate

  LDNext(Basic, Calculated, LegToGo);

  if (LegTime0>= 0.9*ERROR_TIME) {
    // can't make it, so assume flying at current mc
    LegAltitude0 = LegAltitude;
  }

  TaskAltitudeRequired += LegAltitude;
  TaskAltitudeRequired0 += LegAltitude0;
  Calculated->TaskDistanceToGo += LegToGo;
  Calculated->TaskTimeToGo += Calculated->LegTimeToGo;

  height_above_finish-= LegAltitude;

  ////////////////

  if (calc_turning_now) {
    Calculated->TaskTimeToGoTurningNow +=
      Basic->Time-Calculated->TaskStartTime;
  } else {
    Calculated->TaskTimeToGoTurningNow = -1;
  }

  double final_height = FAIFinishHeight(Basic, Calculated, -1);

  double total_energy_height = Calculated->NavAltitude
    + Calculated->EnergyHeight;

  Calculated->TaskAltitudeRequired = TaskAltitudeRequired + final_height;

  TaskAltitudeRequired0 += final_height;

  Calculated->TaskAltitudeDifference = total_energy_height
    - Calculated->TaskAltitudeRequired;

  Calculated->TaskAltitudeDifference0 = total_energy_height
    - TaskAltitudeRequired0;

  // VENTA6
  Calculated->NextAltitudeDifference0 = total_energy_height
    - Calculated->NextAltitudeRequired0;

  Calculated->LDFinish = UpdateLD(Calculated->LDFinish,
                                  Calculated->TaskDistanceToGo,
                                  total_energy_height-final_height,
                                  0.5);

  // VENTA-ADDON Classic geometric GR calculation without Total Energy
  /*
   * Paolo Ventafridda> adding a classic standard glide ratio
   * computation based on a geometric path with no total energy and
   * wind. This value is auto limited to a reasonable level which can
   * be useful during flight, currently 200. Over 200, you are no more
   * gliding to the final destination I am afraid, even on an ETA
   * . The infobox value has a decimal point if it is between 1 and
   * 99, otherwise it's a simple integer.
   */
  double GRsafecalc = Calculated->NavAltitude - final_height;
  if (GRsafecalc <=0) Calculated->GRFinish = INVALID_GR;
  else {
    Calculated->GRFinish = Calculated->TaskDistanceToGo / GRsafecalc;
    if ( Calculated->GRFinish >ALTERNATE_MAXVALIDGR || Calculated->GRFinish <0 ) Calculated->GRFinish = INVALID_GR;
    else
      if ( Calculated->GRFinish <1 ) Calculated->GRFinish = 1;
  }
  // END VENTA-ADDON

  CheckFinalGlideThroughTerrain(Basic, Calculated,
                                LegToGo, LegBearing);

  CheckForceFinalGlide(Basic, Calculated);

  mutexTaskData.Unlock();

}

static void
AATStats_Time(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  // Task time to go calculations

  double aat_tasktime_elapsed = Basic->Time - Calculated->TaskStartTime;
  double aat_tasklength_seconds = AATTaskLength*60;

  if (ActiveWayPoint==0) {
    if (Calculated->AATTimeToGo==0) {
      Calculated->AATTimeToGo = aat_tasklength_seconds;
    }
  } else if (aat_tasktime_elapsed>=0) {
    Calculated->AATTimeToGo = max(0,
				  aat_tasklength_seconds
				  - aat_tasktime_elapsed);
  }

  if(ValidTaskPoint(ActiveWayPoint) && (Calculated->AATTimeToGo>0)) {
    Calculated->AATMaxSpeed =
      Calculated->AATMaxDistance / Calculated->AATTimeToGo;
    Calculated->AATMinSpeed =
      Calculated->AATMinDistance / Calculated->AATTimeToGo;
    Calculated->AATTargetSpeed =
      Calculated->AATTargetDistance / Calculated->AATTimeToGo;
  }
}

static void
AATStats_Distance(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  int i;
  double MaxDistance, MinDistance, TargetDistance;

  mutexTaskData.Lock();

  MaxDistance = 0; MinDistance = 0; TargetDistance = 0;
  // Calculate Task Distances

  if(ValidTaskPoint(ActiveWayPoint))
    {
      i=ActiveWayPoint;

      double LegToGo=0, TargetLegToGo=0;

      if (i > 0 ) { //RLD only include distance from glider to next leg if we've started the task
        DistanceBearing(Basic->Latitude , Basic->Longitude ,
                        WayPointList[Task[i].Index].Latitude,
                        WayPointList[Task[i].Index].Longitude,
                        &LegToGo, NULL);

        DistanceBearing(Basic->Latitude , Basic->Longitude ,
                        Task[i].AATTargetLat,
                        Task[i].AATTargetLon,
                        &TargetLegToGo, NULL);

        if(Task[i].AATType == CIRCLE)
        {
          MaxDistance = LegToGo + (Task[i].AATCircleRadius );  // ToDo: should be adjusted for angle of max target and for national rules
          MinDistance = LegToGo - (Task[i].AATCircleRadius );
        }
        else
        {
          MaxDistance = LegToGo + (Task[i].AATSectorRadius );  // ToDo: should be adjusted for angle of max target.
          MinDistance = LegToGo;
        }

        TargetDistance = TargetLegToGo;
      }

      i++;
      while(ValidTaskPoint(i)) {
	double LegDistance, TargetLegDistance;

	DistanceBearing(WayPointList[Task[i].Index].Latitude,
			WayPointList[Task[i].Index].Longitude,
			WayPointList[Task[i-1].Index].Latitude,
			WayPointList[Task[i-1].Index].Longitude,
			&LegDistance, NULL);

	DistanceBearing(Task[i].AATTargetLat,
			Task[i].AATTargetLon,
			Task[i-1].AATTargetLat,
			Task[i-1].AATTargetLon,
			&TargetLegDistance, NULL);

	MaxDistance += LegDistance;
	MinDistance += LegDistance;

	if(Task[ActiveWayPoint].AATType == CIRCLE) {
	  // breaking out single Areas increases accuracy for start
	  // and finish

	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance -= StartRadius; // e.g. Sports 2009 US Rules A116.3.2.  To Do: This should be configured multiple countries
	    MinDistance -= StartRadius;
	  } else { // not first leg of task
	    MaxDistance += (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance -= FinishRadius; // To Do: This can be configured for finish rules
	    MinDistance -= FinishRadius;
	  } else { // not last leg of task
	    MaxDistance += (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }
	} else { // not circle (pie slice)
	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for start rules
	  } else { // not first leg of task
	    MaxDistance += (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for finish rules
	  } else { // not last leg of task
	    MaxDistance += (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }
	}
	TargetDistance += TargetLegDistance;
	i++;
      }

      // JMW TODO accuracy: make these calculations more accurate, because
      // currently they are very approximate.

      Calculated->AATMaxDistance = MaxDistance;
      Calculated->AATMinDistance = MinDistance;
      Calculated->AATTargetDistance = TargetDistance;
    }
  mutexTaskData.Unlock();
}

void
AATStats(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{

  if (!WayPointList
      || !AATEnabled
      || Calculated->ValidFinish) return ;

  AATStats_Distance(Basic, Calculated);
  AATStats_Time(Basic, Calculated);

}
