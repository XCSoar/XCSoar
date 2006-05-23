/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

        M Roberts (original release)
        Robin Birch <robinb@ruffnready.co.uk>
        Samuel Gisiger <samuel.gisiger@triadis.ch>
        Jeff Goodenough <jeff@enborne.f2s.com>
        Alastair Harrison <aharrison@magic.force9.co.uk>
        Scott Penrose <scottp@dd.com.au>
        John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"
#include "Calculations.h"
#include "Dialogs.h"
#include "parser.h"
#include "Utils.h"
#include "Externs.h"
#include "McReady.h"
#include "Airspace.h"
#include "Logger.h"
#include "VarioSound.h"
#include <windows.h>
#include <math.h>
#include "InputEvents.h"
#include "Message.h"

#include <tchar.h>

#include "windanalyser.h"
#include "Atmosphere.h"

#include "VegaVoice.h"

WindAnalyser *windanalyser = NULL;
VegaVoice vegavoice;

int AutoWindMode= 1;
#define D_AUTOWIND_CIRCLING 1
#define D_AUTOWIND_ZIGZAG 2

// 0: Manual
// 1: Circling
// 2: ZigZag
// 3: Both

bool EnableNavBaroAltitude=false;

bool ForceFinalGlide= false;
bool AutoForceFinalGlide= false;

#include "Port.h"

#include "WindZigZag.h"

static void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Heading(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void EnergyHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int  InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int  InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int i);
static int  InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CalculateNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void InAATSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int  InAATStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int  InAATurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void SortLandableWaypoints(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double bearing, distance;
  double lat, lon;

  // estimate max range (only interested in at most one screen distance away)
  double mymaxrange = MapWindow::GetApproxScreenRange();

  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    //    bearing = -90+i*180/NUMTERRAINSWEEPS+Basic->TrackBearing;
    bearing = i*360/NUMTERRAINSWEEPS;
    LockFlightData();
    distance = FinalGlideThroughTerrain(bearing,
                                        Basic,
                                        Calculated, &lat, &lon,
                                        mymaxrange);
    MapWindow::GlideFootPrint[i].x = lon;
    MapWindow::GlideFootPrint[i].y = lat;
    UnlockFlightData();
  }
}


int FinishLine=1;
DWORD FinishRadius=1000;


void RefreshTaskStatistics(void) {
  LockFlightData();
  TaskStatistics(&GPS_INFO, &CALCULATED_INFO, MACCREADY);
  AATStats(&GPS_INFO, &CALCULATED_INFO);
  UnlockFlightData();
}


int getFinalWaypoint() {
  int i;
  i=ActiveWayPoint;

  i++;
  while((Task[i].Index != -1) && (i<MAXTASKPOINTS))
    {
      i++;
    }
  return i-1;
}

int FastLogNum = 0; // number of points to log at high rate


void AddSnailPoint(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (!Calculated->Flying) return;

  SnailTrail[SnailNext].Latitude = (float)(Basic->Latitude);
  SnailTrail[SnailNext].Longitude = (float)(Basic->Longitude);
  SnailTrail[SnailNext].Time = Basic->Time;

  if (Basic->NettoVarioAvailable) {
    SnailTrail[SnailNext].Vario = (float)(Basic->NettoVario) ;
  } else {
    SnailTrail[SnailNext].Vario = (float)(Calculated->Vario) ;
  }
  SnailTrail[SnailNext].Colour = -1; // need to have colour calculated
  SnailTrail[SnailNext].Circling = Calculated->Circling;

  SnailNext ++;
  SnailNext %= TRAILSIZE;

}


int LoggerTimeStepCruise=5;
int LoggerTimeStepCircling=1;


void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static double SnailLastTime=0;
  static double LogLastTime=0;
  static double StatsLastTime=0;
  double dtLog = 5.0;
  double dtSnail = 2.0;
  double dtStats = 60.0;

  if(Basic->Time <= LogLastTime)
    {
      LogLastTime = Basic->Time;
    }
  if(Basic->Time <= SnailLastTime)
    {
      SnailLastTime = Basic->Time;
    }
  if(Basic->Time <= StatsLastTime)
    {
      StatsLastTime = Basic->Time;
    }

  if (FastLogNum) {
    dtLog = 1.0;
  }

  // draw snail points more often in circling mode
  if (Calculated->Circling) {
    dtSnail = LoggerTimeStepCircling*1.0;
  } else {
    dtSnail = LoggerTimeStepCruise*1.0;
  }

  if (Basic->Time - LogLastTime >= dtLog) {
    if(LoggerActive) {
      LogPoint(Basic->Latitude , Basic->Longitude , Basic->Altitude );
    }
    LogLastTime += dtLog;
    if (LogLastTime< Basic->Time-dtLog) {
      LogLastTime = Basic->Time-dtLog;
    }

    if (FastLogNum) FastLogNum--;

  }

  if (Basic->Time - SnailLastTime >= dtSnail) {
    AddSnailPoint(Basic, Calculated);
    SnailLastTime += dtSnail;
    if (SnailLastTime< Basic->Time-dtSnail) {
      SnailLastTime = Basic->Time-dtSnail;
    }
  }

  if (Basic->Time - StatsLastTime >= dtStats) {
    flightstats.Altitude.
      least_squares_update(Basic->Time/3600.0,
                           Calculated->NavAltitude);
    StatsLastTime += dtStats;
    if (StatsLastTime< Basic->Time-dtStats) {
      StatsLastTime = Basic->Time-dtStats;
    }
  }


}


extern int jmw_demo;

void AudioVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double n;

  // get load factor

  if (Basic->AccelerationAvailable) {
    n = Basic->Gload;
  } else {
    n = 1.0;
  }

  // calculate sink rate of glider

  double theSinkRate;

#define AUDIOSCALE 100/7.5  // +/- 7.5 m/s range

  if (Basic->AirspeedAvailable) {
    theSinkRate= GlidePolar::SinkRate(Basic->IndicatedAirspeed, n);
  } else {
    // assume zero wind (Speed=Airspeed, very bad I know)
    theSinkRate= GlidePolar::SinkRate(Basic->Speed, n);
  }

  if (Basic->NettoVarioAvailable) {
    Calculated->NettoVario = Basic->NettoVario;
  } else {
    if (Basic->VarioAvailable) {
      Calculated->NettoVario = Basic->Vario - theSinkRate;
    } else {
      Calculated->NettoVario = Calculated->Vario - theSinkRate;
    }
  }

  if (
      (Basic->AirspeedAvailable &&
       (Basic->IndicatedAirspeed >= NettoSpeed))
      ||
      (!Basic->AirspeedAvailable &&
       (Basic->Speed >= NettoSpeed))
      ) {
    // TODO: slow/smooth switching between netto and not

    VarioSound_SetV((short)((Calculated->NettoVario-GlidePolar::minsink)*AUDIOSCALE));

  } else {

    if (Basic->VarioAvailable) {
      VarioSound_SetV((short)(Basic->Vario*AUDIOSCALE));
    } else {
      VarioSound_SetV((short)(Calculated->Vario*AUDIOSCALE));
    }

  }

  // calculate optimum cruise speed in current track direction
  // this still makes use of mode, so it should agree with
  // Vmcready if the track bearing is the best cruise track
  // this does assume g loading of 1.0

  // this is basically a dolphin soaring calculator

  double dmc;

  if (EnableBlockSTF) {
    dmc = MACCREADY;
  } else {
    dmc = MACCREADY-Calculated->NettoVario;
  }

  if (Calculated->Vario <= MACCREADY) {

    double VOptnew;

    GlidePolar::MacCreadyAltitude(dmc,
                                 100.0, // dummy value
                                 Basic->TrackBearing,
                                 Calculated->WindSpeed,
                                 Calculated->WindBearing,
                                 0,
                                 &VOptnew,
                                 true,
                                 0
                                 );

    // put low pass filter on VOpt so display doesn't jump around
    // too much
    Calculated->VOpt = Calculated->VOpt*0.6+VOptnew*0.4;

  } else {
    // this thermal is better than maccready, so fly at minimum sink
    // speed
    // calculate speed of min sink adjusted for load factor
    Calculated->VOpt = GlidePolar::Vminsink*sqrt(n);
  }

  Calculated->STFMode = false;
  double vdiff;

  if (Basic->AirspeedAvailable) {
    if (Basic->AirspeedAvailable) {
      vdiff = 100*(1.0-Calculated->VOpt/(Basic->IndicatedAirspeed+0.01));
    } else {
      vdiff = 100*(1.0-Calculated->VOpt/(Basic->Speed+0.01));
    }
    VarioSound_SetVAlt((short)(vdiff));
    Calculated->STFMode = false;
    if ((Basic->Speed>NettoSpeed)||
        ((Calculated->VOpt>NettoSpeed)&&(Basic->Speed<Calculated->VOpt*1.1))
        ){
      Calculated->STFMode = true;
    }
    // lock on when supernetto climb rate is half mc
    if (dmc< MACCREADY/2.0) {
      Calculated->STFMode = false;
    }
    // lock on in circling
    if (Calculated->Circling) {
      Calculated->STFMode = false;
    }
  }

  // STF test
  if (jmw_demo==1) {
    Calculated->STFMode = true;
    Calculated->VOpt = 35.0;
    vdiff = 100*(1.0-Calculated->VOpt/(Basic->IndicatedAirspeed+0.01));
    VarioSound_SetVAlt((short)(vdiff));
    VarioSound_SetSTFMode(Calculated->STFMode);
  }
  // Climb test
  if (jmw_demo==2) {
    Calculated->STFMode = false;
    VarioSound_SetV((short)(Basic->Vario*AUDIOSCALE));
    VarioSound_SetVAlt(0);
    VarioSound_SetSTFMode(Calculated->STFMode);
  }
  VarioSound_SoundParam();

  Calculated->STFMode = !Calculated->Circling;
  //  audio_send(Basic, Calculated);

}


BOOL DoCalculationsVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;

  AudioVario(Basic, Calculated);

  // has GPS time advanced?
  if(Basic->Time <= LastTime)
    {
      LastTime = Basic->Time;
      return FALSE;
    }

  LastTime = Basic->Time;

  return TRUE;
}


bool EnableCalibration = false;


void Heading(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double x0, y0, mag;

  if ((Basic->Speed>0)||(Calculated->WindSpeed>0)) {
    x0 = fastsine(Basic->TrackBearing)*Basic->Speed;
    y0 = fastcosine(Basic->TrackBearing)*Basic->Speed;
    x0 += fastsine(Calculated->WindBearing)*Calculated->WindSpeed;
    y0 += fastcosine(Calculated->WindBearing)*Calculated->WindSpeed;

    Calculated->Heading = atan2(x0,y0)*RAD_TO_DEG;
    if (Calculated->Heading<0) {
      Calculated->Heading += 360;
    }

#ifdef _SIM_
    if (!Calculated->Circling) {
      mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
      Basic->TrueAirspeed = mag;
    }
#endif

    if ((AutoWindMode & D_AUTOWIND_ZIGZAG)==D_AUTOWIND_ZIGZAG) {
      Vector v;
      double zzwindspeed;
      double zzwindbearing;
      if (WindZigZagUpdate(Basic, Calculated,
                           &zzwindspeed,
                           &zzwindbearing)) {
        v.x = zzwindspeed*cos(zzwindbearing*3.1415926/180.0);
        v.y = zzwindspeed*sin(zzwindbearing*3.1415926/180.0);
        if (windanalyser) {
          windanalyser->slot_newEstimate(v, 3);
        }
      }
    }

    if (EnableCalibration) {
      mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
      if ((Basic->AirspeedAvailable) && (Basic->IndicatedAirspeed>0)) {

        double k = (mag / max(1.0,Basic->TrueAirspeed));

#ifdef DEBUG
        char buffer[200];
        sprintf(buffer,"%g %g %g %g %g %g %g %g %g # airspeed\r\n",
                Basic->IndicatedAirspeed,
                mag*Basic->IndicatedAirspeed/max(1.0,Basic->TrueAirspeed),
                k,
                Basic->Speed,
                Calculated->WindSpeed,
                Calculated->WindBearing,
                Basic->Gload,
                Basic->BaroAltitude,
                Basic->Altitude);
        DebugStore(buffer);
#endif

      }
    }

  } else {
    Calculated->Heading = Basic->TrackBearing;
  }

}


void  SetWindEstimate(double speed, double bearing) {
  Vector v;
  v.x = speed*cos(bearing*3.1415926/180.0);
  v.y = speed*sin(bearing*3.1415926/180.0);
  if (windanalyser) {
    windanalyser->slot_newEstimate(v, 6);
  }
}

void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // do slow part of calculations (cleanup of caches etc, nothing
  // that changes the state)

  if (FinalGlideTerrain)
     TerrainFootprint(Basic, Calculated);

  static double LastOptimiseTime = 0;

  AirspaceWarning(Basic, Calculated);

  // moved from MapWindow.cpp
  if(Basic->Time> LastOptimiseTime+0.0)
    {
      LastOptimiseTime = Basic->Time;
      LockTerrainDataCalculations();
      if (terrain_dem_calculations.terraincachemisses > 0){
        DWORD tm =GetTickCount();
        terrain_dem_calculations.OptimizeCash();
        tm = GetTickCount()-tm;
        tm = GetTickCount();
      }
      terrain_dem_calculations.SetCacheTime();
      UnlockTerrainDataCalculations();
    }
}


bool TaskFinished = false;


void ResetFlightStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  flightstats.Reset();
  TaskFinished = false;
  Calculated->TaskStartTime = 0;
  Calculated->TaskStartSpeed = 0;
  Calculated->TaskStartAltitude = 0;
  Calculated->LegStartTime = 0;
  Calculated->ValidStart = false;
}


void CloseCalculations() {
  if (windanalyser) {
    delete windanalyser;
    windanalyser = NULL;
  }
}


BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double maccready;

  if (!windanalyser) {
    windanalyser = new WindAnalyser(Basic, Calculated);

    // seed initial wind store with current conditions
    SetWindEstimate(Calculated->WindSpeed,Calculated->WindBearing);

  }

  // Determine which altitude to use for nav functions
  if (EnableNavBaroAltitude && Basic->BaroAltitudeAvailable) {
    Calculated->NavAltitude = Basic->BaroAltitude;
  } else {
    Calculated->NavAltitude = Basic->Altitude;
  }

  DistanceToNext(Basic, Calculated);

  EnergyHeight(Basic, Calculated);

  AltitudeRequired(Basic, Calculated, MACCREADY);

  Heading(Basic, Calculated);

  TerrainHeight(Basic, Calculated);

  if (TaskAborted) {
    SortLandableWaypoints(Basic, Calculated);
  }
  TaskStatistics(Basic, Calculated, MACCREADY);

  if ((Basic->Time != 0) && (Basic->Time <= LastTime))
    // 20060519:sgi added (Basic->Time != 0) dueto alwas return here if no GPS time available
    {

      if (Basic->Time<LastTime) {
              // Reset statistics.. (probably due to being in IGC replay mode)
              ResetFlightStats(Basic, Calculated);
      }

      LastTime = Basic->Time;
      return FALSE;
    }

  LastTime = Basic->Time;

  if ((Calculated->FinalGlide)
      ||(fabs(Calculated->TaskAltitudeDifference)>30)) {
    FinalGlideAlert(Basic, Calculated);
    if (Calculated->AutoMacCready) {
      DoAutoMacCready(Basic, Calculated);
    }
  }

  TakeoffLanding(Basic, Calculated);
  Turning(Basic, Calculated);
  Vario(Basic,Calculated);
  LD(Basic,Calculated);
  CruiseLD(Basic,Calculated);
  Average30s(Basic,Calculated);
  AverageThermal(Basic,Calculated);
  ThermalGain(Basic,Calculated);
  LastThermalStats(Basic, Calculated);
  ThermalBand(Basic, Calculated);

  DistanceToNext(Basic, Calculated);

  // do we need to do this twice?
  if (TaskAborted) {

    SortLandableWaypoints(Basic, Calculated);

  } else {

    InSector(Basic, Calculated);
    InAATSector(Basic, Calculated);

    AATStats(Basic, Calculated);
    TaskStatistics(Basic, Calculated, MACCREADY);

  }

  AltitudeRequired(Basic, Calculated, MACCREADY);

  CalculateNextPosition(Basic, Calculated);

  DoLogging(Basic, Calculated);

  vegavoice.Update(Basic, Calculated);

  return TRUE;
}


void EnergyHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (Basic->AirspeedAvailable) {
    Calculated->EnergyHeight =
      max(0, Basic->IndicatedAirspeed*Basic->IndicatedAirspeed
       -GlidePolar::Vbestld*GlidePolar::Vbestld)/(9.81*2.0);
  } else {
    Calculated->EnergyHeight = 0.0;
  }

}


void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double LastAlt = 0;
  double Gain;

  if(Basic->Time > LastTime)
    {
      Gain = Calculated->NavAltitude - LastAlt;

      if (!Basic->VarioAvailable) {
        // estimate value from GPS
        Calculated->Vario = Gain / (Basic->Time - LastTime);
      } else {
        // get value from instrument
        Calculated->Vario = Basic->Vario;
        // we don't bother with sound here as it is polled at a
        // faster rate in the DoVarioCalcs methods
      }

      LastAlt = Calculated->NavAltitude;
      LastTime = Basic->Time;

    }
  else
    {
      LastTime = Basic->Time;
    }
}


void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double Altitude[30];
  static double Vario[30];
  int Elapsed, i;
  long temp;
  double Gain;

  if(Basic->Time > LastTime)
    {
      Elapsed = (int)(Basic->Time - LastTime);
      for(i=0;i<Elapsed;i++)
        {
          temp = (long)LastTime + i;
          temp %=30;

          Altitude[temp] = Calculated->NavAltitude;
          Vario[temp] = Basic->Vario;
        }
      double Vave = 0;
      for (i=0; i<30; i++) {
        Vave += Vario[i];
      }

      temp = (long)Basic->Time - 1;
      temp = temp%30;
      Gain = Altitude[temp];

      temp = (long)Basic->Time;
      temp = temp%30;
      Gain = Gain - Altitude[temp];

      LastTime = Basic->Time;
      if (Basic->VarioAvailable) {
        Calculated->Average30s = Vave/30;
      } else {
        Calculated->Average30s = Gain/30;
      }
    }
  else
    {
      LastTime = Basic->Time;
    }


}

void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double Gain;

  if(Basic->Time > Calculated->ClimbStartTime)
    {
      Gain = Calculated->NavAltitude - Calculated->ClimbStartAlt;
      Calculated->AverageThermal  = Gain / (Basic->Time - Calculated->ClimbStartTime);
    }
}

void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if(Basic->Time > Calculated->ClimbStartTime)
    {
      Calculated->ThermalGain = Calculated->NavAltitude - Calculated->ClimbStartAlt;
    }
}

void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastLat = 0;
  static double LastLon = 0;
  static double LastTime = 0;
  static double LastAlt = 0;
  double DistanceFlown;
  double AltLost;


  if(Basic->Time - LastTime >20)
    {
      DistanceFlown = Distance(Basic->Latitude, Basic->Longitude, LastLat, LastLon);
      AltLost = LastAlt - Calculated->NavAltitude;
      if(AltLost > 0)
        {
          Calculated->LD = DistanceFlown / AltLost;
          if(Calculated->LD>999)
            {
              Calculated->LD = 999;
            }
        }
      else if (AltLost < 0) {
        Calculated->LD = DistanceFlown / AltLost;
        if (Calculated->LD<-999) {
          Calculated->LD = 999;
        }
      } else {
        Calculated->LD = 999;
      }

      LastLat = Basic->Latitude;
      LastLon = Basic->Longitude;
      LastAlt = Calculated->NavAltitude;
      LastTime = Basic->Time;
    }
}

void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastLat = 0;
  static double LastLon = 0;
  static double LastTime = 0;
  static double LastAlt = 0;
  double DistanceFlown;
  double AltLost;


  if(!Calculated->Circling)
    {

      DistanceFlown = Distance(Basic->Latitude, Basic->Longitude, Calculated->CruiseStartLat, Calculated->CruiseStartLong);
      AltLost = Calculated->CruiseStartAlt - Calculated->NavAltitude;
      if(AltLost > 0)
        {
          Calculated->CruiseLD = DistanceFlown / AltLost;
          if(Calculated->CruiseLD>999)
            {
              Calculated->CruiseLD = 999;
            }
        }
      else if (AltLost <0) {
        Calculated->CruiseLD = DistanceFlown / AltLost;
        if(Calculated->CruiseLD< -999)
          {
            Calculated->CruiseLD = 999;
          }
      } else {
        Calculated->CruiseLD = 999;
      }
    }
}

#define CRUISE 0
#define WAITCLIMB 1
#define CLIMB 2
#define WAITCRUISE 3


double MinTurnRate = 4 ; //10;
double CruiseClimbSwitch = 15;
double ClimbCruiseSwitch = 15;


void SwitchZoomClimb(bool isclimb, bool left) {

  static double CruiseMapScale = 10;
  static double ClimbMapScale = 0.25;
  static bool last_isclimb = false;
  // if AutoZoom

  // JMW
  /* Not needed now since can put it in input events if users want it.
  if (EnableSoundTask) {
    PlayResource(TEXT("IDR_WAV_DRIP"));
  }
  */

  if (CircleZoom) {
    if (isclimb != last_isclimb) {
      if (isclimb) {
        // save cruise scale
        CruiseMapScale = MapWindow::MapScale;
        // switch to climb scale
        MapWindow::RequestMapScale = ClimbMapScale;
        MapWindow::BigZoom = true;
      } else {
        // leaving climb
        // save cruise scale
        ClimbMapScale = MapWindow::MapScale;
        MapWindow::RequestMapScale = CruiseMapScale;
        // switch to climb scale
        MapWindow::BigZoom = true;
      }
      last_isclimb = isclimb;
    } else {
      // nothing to do.
    }
  }
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    windanalyser->slot_newFlightMode(left, 0);
  }

}


void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTrack = 0;
  static double StartTime  = 0;
  static double StartLong = 0;
  static double StartLat = 0;
  static double StartAlt = 0;
  static double LastTime = 0;
  static double timeCircling = 0;
  static double timeCruising = 0;
  static int MODE = CRUISE;
  static bool LEFT = FALSE;
  double Rate;
  static double LastRate=0;
  double dRate;
  double dT;

  if(Basic->Time <= LastTime) {
    LastTime = Basic->Time;
    return;
  }
  dT = Basic->Time - LastTime;
  Rate = Basic->TrackBearing-LastTrack;
  while (Rate>180) {
    Rate-= 360;
  }
  while (Rate<-180) {
    Rate+= 360;
  }
  Rate = Rate / dT;

  if (dT<2.0) {
    // time step ok

    // calculate acceleration
    dRate = (Rate-LastRate)/dT;

    double dtlead=0.3;
    // integrate assuming constant acceleration, for one second
    Calculated->NextTrackBearing = Basic->TrackBearing
      + dtlead*(Rate+0.5*dtlead*dRate);
    // s = u.t+ 0.5*a*t*t

    if (Calculated->NextTrackBearing<0) {
      Calculated->NextTrackBearing+= 360;
    }
    if (Calculated->NextTrackBearing>=360) {
      Calculated->NextTrackBearing-= 360;
    }

  } else {
    // time step too big, so just take it at last measurement
    Calculated->NextTrackBearing = Basic->TrackBearing;
  }

  Calculated->TurnRate = Rate;

  LastRate = Rate;

  // JMW added percent climb calculator

  if (Calculated->Circling) {
    //    timeCircling += (Basic->Time-LastTime);
    timeCircling+= 1.0;
  } else {
    //    timeCruising += (Basic->Time-LastTime);
    timeCruising+= 1.0;
  }

  if (timeCruising+timeCircling>0) {
    Calculated->PercentCircling =
      100.0*(timeCircling)/(timeCruising+timeCircling);
  } else {
    Calculated->PercentCircling = 0.0;
  }

  if(Rate <0)
    {
      if (LEFT) {
        // OK, already going left
      } else {
        LEFT = true;
      }
      Rate *= -1;
    } else {
    if (!LEFT) {
      // OK, already going right
    } else {
      LEFT = false;
    }
  }

  LastTime = Basic->Time;
  LastTrack = Basic->TrackBearing;

  double temp = StartTime;

  switch(MODE) {
  case CRUISE:
    if(Rate >= MinTurnRate) {
      StartTime = Basic->Time;
      StartLong = Basic->Longitude;
      StartLat  = Basic->Latitude;
      StartAlt  = Calculated->NavAltitude;
      MODE = WAITCLIMB;
    }
    break;
  case WAITCLIMB:
    if(Rate >= MinTurnRate) {
      if( (Basic->Time  - StartTime) > CruiseClimbSwitch) {
        Calculated->Circling = TRUE;
        // JMW Transition to climb
        MODE = CLIMB;
        Calculated->ClimbStartLat = StartLat;
        Calculated->ClimbStartLong = StartLong;
        Calculated->ClimbStartAlt = StartAlt;
        Calculated->ClimbStartTime = StartTime;

        flightstats.Altitude_Base.
          least_squares_update(Calculated->ClimbStartTime/3600.0,
                               Calculated->ClimbStartAlt);

        // TODO InputEvents GCE - Move this to InputEvents
        // Consider a way to take the CircleZoom and other logic
        // into InputEvents instead?
        // JMW: NO.  Core functionality must be built into the
        // main program, unable to be overridden.
        SwitchZoomClimb(true, LEFT);
        InputEvents::processGlideComputer(GCE_FLIGHTMODE_CLIMB);
      }
    } else {
      // nope, not turning, so go back to cruise
      MODE = CRUISE;
    }
    break;
  case CLIMB:
    if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
      windanalyser->slot_newSample();
    }

    if(Rate < MinTurnRate) {
      StartTime = Basic->Time;
      StartLong = Basic->Longitude;
      StartLat  = Basic->Latitude;
      StartAlt  = Calculated->NavAltitude;
      // JMW Transition to cruise, due to not properly turning
      MODE = WAITCRUISE;
    }
    break;
  case WAITCRUISE:
    if(Rate < MinTurnRate) {
      if( (Basic->Time  - StartTime) > ClimbCruiseSwitch) {
        Calculated->Circling = FALSE;

        // Transition to cruise
        MODE = CRUISE;
        Calculated->CruiseStartLat = StartLat;
        Calculated->CruiseStartLong = StartLong;
        Calculated->CruiseStartAlt = StartAlt;
        Calculated->CruiseStartTime = StartTime;

        flightstats.Altitude_Ceiling.
          least_squares_update(Calculated->CruiseStartTime/3600.0,
                               Calculated->CruiseStartAlt);

        SwitchZoomClimb(false, LEFT);
        InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
      }
    } else {
      // JMW Transition back to climb, because we are turning again
      MODE = CLIMB;
    }
    break;
  default:
    // error, go to cruise
    MODE = CRUISE;
  }
  // generate new wind vector if altitude changes or a new
  // estimate is available
  if (AutoWindMode>0) {
    windanalyser->slot_Altitude();
  }

  // update atmospheric model
  CuSonde::updateMeasurements(Basic, Calculated);

}

static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static int LastCircling = FALSE;
  double ThermalGain;
  double ThermalTime;
  //  double ThermalDrift;
  //  double DriftAngle;

  if((Calculated->Circling == FALSE) && (LastCircling == TRUE))
    {
      ThermalGain = Calculated->CruiseStartAlt - Calculated->ClimbStartAlt;
      ThermalTime = Calculated->CruiseStartTime - Calculated->ClimbStartTime;

      /* Thermal drift calculations now done internally
         to wind analyser

      ThermalDrift = Distance(Calculated->CruiseStartLat,  Calculated->CruiseStartLong, Calculated->ClimbStartLat,  Calculated->ClimbStartLong);
      DriftAngle = Bearing(Calculated->ClimbStartLat,  Calculated->ClimbStartLong,Calculated->CruiseStartLat, Calculated->CruiseStartLong);
      */

      if(ThermalTime >0)
        {
          Calculated->LastThermalAverage = ThermalGain/ThermalTime;
          Calculated->LastThermalGain = ThermalGain;
          Calculated->LastThermalTime = ThermalTime;

          if (Calculated->LastThermalAverage>0) {
            flightstats.ThermalAverage.
              least_squares_update(Calculated->LastThermalAverage);

#ifdef DEBUG
            char Temp[100];
            sprintf(Temp,"%f %f # thermal stats\n",
                    flightstats.ThermalAverage.m,
                    flightstats.ThermalAverage.b
                    );
            DebugStore(Temp);
#endif
          }


          /*
          if(ThermalTime > 120)
            {

              // Don't set it immediately, go through the new
              //   wind model
              Calculated->WindSpeed = ThermalDrift/ThermalTime;

              if(DriftAngle >=180)
                DriftAngle -= 180;
              else
                DriftAngle += 180;

              Calculated->WindBearing = DriftAngle;

              // NOW DONE INTERNALLY TO WINDANALYSER
              Vector v;
              v.x = -ThermalDrift/ThermalTime*cos(DriftAngle*3.1415926/180.0);
              v.y = -ThermalDrift/ThermalTime*sin(DriftAngle*3.1415926/180.0);

              windanalyser->slot_newEstimate(v, 6);
              // 6 is the code for external estimates
            }
          */
        }
    }
  LastCircling = Calculated->Circling;
}

void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  // JMW these things should be in Calculated, not Basic

  if (!WayPointList) return;

  LockFlightData();

  if(ActiveWayPoint >=0)
    {
      Calculated->WaypointDistance = Distance(Basic->Latitude, Basic->Longitude,
                                         WayPointList[Task[ActiveWayPoint].Index].Latitude,
                                         WayPointList[Task[ActiveWayPoint].Index].Longitude);
      if (Calculated->WaypointDistance > 0.5)
        Calculated->WaypointBearing = Bearing(Basic->Latitude, Basic->Longitude,
                                       WayPointList[Task[ActiveWayPoint].Index].Latitude,
                                       WayPointList[Task[ActiveWayPoint].Index].Longitude);
      else {
        Calculated->WaypointDistance = 0;
        Calculated->WaypointBearing = 360;
      }
    }
  else
    {
      Calculated->WaypointDistance = 0;
    }
  UnlockFlightData();
}


void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready)
{
  LockFlightData();
  if((ActiveWayPoint >=0)&&(WayPointList))
    {
      Calculated->NextAltitudeRequired =
        GlidePolar::MacCreadyAltitude(maccready,
                        Calculated->WaypointDistance,
                        Calculated->WaypointBearing,
                        Calculated->WindSpeed, Calculated->WindBearing,
                        0, 0, (ActiveWayPoint == getFinalWaypoint()),
                        0
                        // ||
                        // (Calculated->TaskAltitudeDifference>30)
                        // JMW TODO!!!!!!!!!
                        );

      Calculated->NextAltitudeRequired =
        Calculated->NextAltitudeRequired + SAFETYALTITUDEARRIVAL ;

      Calculated->NextAltitudeDifference =
        Calculated->NavAltitude - (Calculated->NextAltitudeRequired
                           + WayPointList[Task[ActiveWayPoint].Index].Altitude)         + Calculated->EnergyHeight;
    }
  else
    {
      Calculated->NextAltitudeRequired = 0;
      Calculated->NextAltitudeDifference = 0;
    }
  UnlockFlightData();
}

int InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double AircraftBearing;

  if (!WayPointList) return FALSE;

  if(SectorType==0)
    {
      if(Calculated->WaypointDistance < SectorRadius)
        {
          return TRUE;
        }
    }
  if (SectorType>0)
    {
      AircraftBearing =
        Bearing(WayPointList[Task[ActiveWayPoint].Index].Latitude,
                WayPointList[Task[ActiveWayPoint].Index].Longitude,
                Basic->Latitude ,
                Basic->Longitude);

      AircraftBearing = AircraftBearing - Task[ActiveWayPoint].Bisector ;
      while (AircraftBearing<-180) {
        AircraftBearing+= 360;
      }
      while (AircraftBearing>180) {
        AircraftBearing-= 360;
      }

      if (SectorType==2) {
        // JMW added german rules
        if (Calculated->WaypointDistance<500) {
          return TRUE;
        }
      }
      if( (AircraftBearing >= -45) && (AircraftBearing <= 45))
        {
          if (SectorType==1) {
            if(Calculated->WaypointDistance < 20000)
              {
                return TRUE;
              }
          } else {
            // JMW added german rules
            if(Calculated->WaypointDistance < 10000)
              {
                return TRUE;
              }
          }
        }
    }
  return FALSE;
}

int InAATTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double AircraftBearing;

  if (!WayPointList) return FALSE;

  if(Task[ActiveWayPoint].AATType ==  CIRCLE)
    {
      if(Calculated->WaypointDistance < Task[ActiveWayPoint].AATCircleRadius)
        {
          return TRUE;
        }
    }
  else if(Calculated->WaypointDistance < Task[ActiveWayPoint].AATSectorRadius)
    {

      AircraftBearing = Bearing(WayPointList[Task[ActiveWayPoint].Index].Latitude,
                                WayPointList[Task[ActiveWayPoint].Index].Longitude,
                                Basic->Latitude ,
                                Basic->Longitude);

      if(Task[ActiveWayPoint].AATStartRadial < Task[ActiveWayPoint].AATFinishRadial )
        {
          if(
             (AircraftBearing > Task[ActiveWayPoint].AATStartRadial)
             &&
             (AircraftBearing < Task[ActiveWayPoint].AATFinishRadial)
             )
            return TRUE;
        }

      if(Task[ActiveWayPoint].AATStartRadial > Task[ActiveWayPoint].AATFinishRadial )
        {
          if(
             (AircraftBearing > Task[ActiveWayPoint].AATStartRadial)
             ||
             (AircraftBearing < Task[ActiveWayPoint].AATFinishRadial)
             )
            return TRUE;
        }
    }
  return FALSE;
}


bool ValidFinish(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  bool valid = true;
  if ((FinishMinHeight!=0)&&(Calculated->TerrainValid)) {
    if (Calculated->AltitudeAGL<FinishMinHeight)
      valid = false;
  }
  return valid;
}


int InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                   int i)
{
  static int LastInSector = FALSE;
  double AircraftBearing;
  double FirstPointDistance;

  if (!WayPointList) return FALSE;

  if (ValidFinish(Basic, Calculated)) return FALSE;

  // Finish invalid
  if(Task[i].Index == -1)
    {
      return FALSE;
    }

  // distance from aircraft to start point
  FirstPointDistance = Distance(Basic->Latitude,
                                Basic->Longitude,
                                WayPointList[Task[i].Index].Latitude,
                                WayPointList[Task[i].Index].Longitude);
  bool inrange = false;
  inrange = (FirstPointDistance<FinishRadius);
  if (!inrange) {
    LastInSector = false;
  }

  if(!FinishLine) // Start Circle
    {
      return inrange;
    }

  // Start Line
  AircraftBearing = Bearing(Basic->Latitude ,
                            Basic->Longitude,
                            WayPointList[Task[i].Index].Latitude,
                            WayPointList[Task[i].Index].Longitude);

  AircraftBearing = AircraftBearing - Task[i].InBound ;
  while (AircraftBearing<-180) {
    AircraftBearing+= 360;
  }
  while (AircraftBearing>180) {
    AircraftBearing-= 360;
  }
  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));

  if (inrange) {

    if (LastInSector) {
      // previously approaching the finish line
      if (!approaching) {
        // now moving away from finish line
        LastInSector = false;
        return TRUE;
      }
    } else {
      if (approaching) {
        // now approaching the finish line
        LastInSector = true;
      }
    }

  } else {
    LastInSector = false;
  }

  return FALSE;
}


int InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static int LastInSector = FALSE;
  double AircraftBearing;
  double FirstPointDistance;

  if (!WayPointList) return FALSE;

  // No Task Loaded
  if(Task[0].Index == -1)
    {
      return FALSE;
    }

  // distance from aircraft to start point
  FirstPointDistance = Distance(Basic->Latitude,
                                Basic->Longitude,
                                WayPointList[Task[0].Index].Latitude,
                                WayPointList[Task[0].Index].Longitude);
  bool inrange = false;
  inrange = (FirstPointDistance<StartRadius);
  if (!inrange) {
    LastInSector = false;
  }

  if(!StartLine) // Start Circle
    {
      return inrange;
    }

  // Start Line
  AircraftBearing = Bearing(Basic->Latitude ,
                            Basic->Longitude,
                            WayPointList[Task[0].Index].Latitude,
                            WayPointList[Task[0].Index].Longitude);

  AircraftBearing = AircraftBearing - Task[0].OutBound ;
  while (AircraftBearing<-180) {
    AircraftBearing+= 360;
  }
  while (AircraftBearing>180) {
    AircraftBearing-= 360;
  }
  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));

  if (inrange) {

    if (LastInSector) {
      // previously approaching the start line
      if (!approaching) {
        // now moving away from start line
        LastInSector = false;
        return TRUE;
      }
    } else {
      if (approaching) {
        // now approaching the start line
        LastInSector = true;
      }
    }

  } else {
    LastInSector = false;
  }

  return FALSE;
}


void AnnounceWayPointSwitch() {
  if (ActiveWayPoint == 0) {
    InputEvents::processGlideComputer(GCE_TASK_START);
  } else if (TaskFinished) {
    InputEvents::processGlideComputer(GCE_TASK_FINISH);
  } else {
    InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
  }

  // start logging data at faster rate
  FastLogNum = 5;

  // play sound
  /* Not needed now since can put it in input events if users want it.
  if (EnableSoundTask) {
    PlayResource(TEXT("IDR_WAV_TASKTURNPOINT"));
  }
  */

}



bool ReadyToAdvance(void) {

  // 0: Manual
  // 1: Auto
  // 2: Arm
  // 3: Arm start

  if (AutoAdvance==1) {  // auto
    AdvanceArmed = false;
    return true;
  }
  if ((AutoAdvance==2)&&(AdvanceArmed)) { // arm
    AdvanceArmed = false;
    return true;
  }
  if (AutoAdvance==3) { // arm start
    if (ActiveWayPoint>0) { // past start
      AdvanceArmed = false;
      return true;
    }
    if (AdvanceArmed) { // on start
      AdvanceArmed = false;
      return true;
    }
  }
  return false;
}


/*

  Track 'TaskStarted' in Calculated info, so it can be
  displayed in the task status dialog.

  Must be reset at start of flight.

  For multiple starts, after start has been passed, need
  to set the first waypoint to the start waypoint and
  then recalculate task stats.

*/

bool ValidStart(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  bool valid = true;
  if ((StartMaxHeight!=0)&&(Calculated->TerrainValid)) {
    if (Calculated->AltitudeAGL>StartMaxHeight)
      valid = false;
  }
  if (StartMaxSpeed!=0) {
    if (Basic->AirspeedAvailable) {
      if (Basic->IndicatedAirspeed>StartMaxSpeed)
        valid = false;
    } else {
      if (Basic->Speed>StartMaxSpeed)
        valid = false;
    }
  }
  return valid;
}




void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL StartSectorEntered = FALSE;

  if(AATEnabled)
    return;

  LockFlightData();

  Calculated->IsInSector = false;

  if(ActiveWayPoint == 0)
    {
      TaskFinished = false;
      if(InStartSector(Basic,Calculated))
        {
          Calculated->IsInSector = true;
          StartSectorEntered = TRUE;

          // TODO monitor start speed throughout time in start sector

        }
      else
        {
          if((StartSectorEntered == TRUE)&&
             (ValidStart(Basic, Calculated)))
            {
              if(ActiveWayPoint < MAXTASKPOINTS)
                {
                  if(Task[ActiveWayPoint+1].Index >= 0)
                    {
                      Calculated->ValidStart = true;
                      if (ReadyToAdvance()) {
                        ActiveWayPoint++;
                        Calculated->TaskStartTime = Basic->Time ;
                        Calculated->TaskStartSpeed = Basic->Speed;
                        Calculated->TaskStartAltitude = Basic->Altitude;
                        Calculated->LegStartTime = Basic->Time;
                        AnnounceWayPointSwitch();

                      }
                      TaskFinished = false;
                      StartSectorEntered = FALSE;
                      // JMW TODO: This causes Vaverage to go bonkers
                      // if the user has already passed the start
                      // but selects the start

                      // Note: pilot must have armed advance
                      // for the start to be registered
                    }
                }
            }
        }
    }
  else if(ActiveWayPoint >0)
    {
      // This allows restart if within 10 minutes of previous start
      if(InStartSector(Basic, Calculated))
        {
          Calculated->IsInSector = true;
          if(Basic->Time - Calculated->TaskStartTime < 600)
            {
              if (ReadyToAdvance()) {
                AdvanceArmed = false;
                Calculated->TaskStartTime = 0;
                ActiveWayPoint = 0;
                AnnounceWayPointSwitch();
                StartSectorEntered = TRUE;
              }
              TaskFinished = false;
            }
        }

      if(ActiveWayPoint < MAXTASKPOINTS) {
        if(Task[ActiveWayPoint+1].Index >= 0) {
          if(InTurnSector(Basic,Calculated)) {
            Calculated->IsInSector = true;
            Calculated->LegStartTime = Basic->Time;

            TaskFinished = false;
            if (ReadyToAdvance()) {
              ActiveWayPoint++;
              AnnounceWayPointSwitch();
            }
            UnlockFlightData();

            return;
          }
        } else {
          if (InFinishSector(Basic,Calculated, ActiveWayPoint)) {
            Calculated->IsInSector = true;
            if (!TaskFinished) {
              TaskFinished = true;
              AnnounceWayPointSwitch();
            }
          } else {
            //      TaskFinished = false;
          }
        }
      }
    }
  UnlockFlightData();
}


void InAATSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL StartSectorEntered = FALSE;

  if(!AATEnabled)
    return;

  LockFlightData();

  Calculated->IsInSector = false;

  if(ActiveWayPoint == 0)
    {
      TaskFinished = false;
      if(InStartSector(Basic,Calculated))
        {
          Calculated->IsInSector = true;
          StartSectorEntered = TRUE;
        }
      else
        {
          if((StartSectorEntered == TRUE)&&
             (ValidStart(Basic, Calculated)))
            {
              if(ActiveWayPoint < MAXTASKPOINTS)
                {
                  if(Task[ActiveWayPoint+1].Index >= 0)
                    {
                      Calculated->ValidStart = true;
                      TaskFinished = false;
                      if (ReadyToAdvance()) {
                        ActiveWayPoint++;
                        Calculated->TaskStartTime = Basic->Time ;
                        Calculated->TaskStartSpeed = Basic->Speed;
                        Calculated->TaskStartAltitude = Basic->Altitude;
                        Calculated->LegStartTime = Basic->Time;
                        AnnounceWayPointSwitch();
                      }
                      StartSectorEntered = FALSE;
                      // JMW TODO: make sure this is valid for manual start
                    }
                }
            }
        }
    }
  else if(ActiveWayPoint >0)
    {
      if(InStartSector(Basic, Calculated)) {
        Calculated->IsInSector = true;
        if(Basic->Time - Calculated->TaskStartTime < 600)
          // this allows restart if returned to start sector before
          // 10 minutes after task start
          {
            if (ReadyToAdvance()) {
              AdvanceArmed = false;
              Calculated->TaskStartTime = 0;
              ActiveWayPoint = 0;
              AnnounceWayPointSwitch();
              StartSectorEntered = TRUE;
            }
            TaskFinished = false;
          }
      }
      if(ActiveWayPoint < MAXTASKPOINTS) {
        if(Task[ActiveWayPoint+1].Index >= 0) {
          if(InAATTurnSector(Basic,Calculated)) {
            Calculated->IsInSector = true;
            Calculated->LegStartTime = Basic->Time;

            if (ReadyToAdvance()) {
              AdvanceArmed = false;
              ActiveWayPoint++;
              AnnounceWayPointSwitch();
            }
            TaskFinished = false;
            UnlockFlightData();
            return;
          }
        } else {
          if (InFinishSector(Basic,Calculated, ActiveWayPoint)) {
            Calculated->IsInSector = true;
            if (!TaskFinished) {
              TaskFinished = true;
              AnnounceWayPointSwitch();
            }
          } else {
            //      TaskFinished = false;
          }
        }
      }
    }
  UnlockFlightData();
}


//////////////////////////////////////////////////////





///////////////////////////////////
#include "RasterTerrain.h"

RasterTerrain terrain_dem_graphics;
RasterTerrain terrain_dem_calculations;

void OpenTerrain(void) {
  LockTerrainDataCalculations();
  LockTerrainDataGraphics();
  RasterTerrain::OpenTerrain();
  terrain_dem_graphics.ClearTerrainCache();
  terrain_dem_calculations.ClearTerrainCache();
  UnlockTerrainDataCalculations();
  UnlockTerrainDataGraphics();
}


void CloseTerrain(void) {
  LockTerrainDataCalculations();
  LockTerrainDataGraphics();
  RasterTerrain::CloseTerrain();
  UnlockTerrainDataCalculations();
  UnlockTerrainDataGraphics();
}


static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double Alt = 0;

  LockTerrainDataCalculations();
  // want most accurate rounding here
  terrain_dem_calculations.SetTerrainRounding(0,0);
  Alt = terrain_dem_calculations.
    GetTerrainHeight(Basic->Latitude , Basic->Longitude);
  UnlockTerrainDataCalculations();

  if(Alt<0) {
    Alt = 0;
    Calculated->TerrainValid = false;
    Calculated->TerrainAlt = 0;
  } else {
    Calculated->TerrainValid = true;
    Calculated->TerrainAlt = Alt;
  }
  Calculated->AltitudeAGL = Basic->Altitude - Calculated->TerrainAlt;

}


/////////////////////////////////////////

void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready)
{
  int i;
  double LegCovered, LegToGo, LegDistance, LegBearing, LegAltitude;
  double TaskAltitudeRequired = 0;
  static bool fgtt = false;
  bool fgttnew = false;

  if (!WayPointList) return;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  LockFlightData();

  // Calculate Task Distances
  if(ActiveWayPoint >=1)
    {

      // JMW added support for target in AAT

      w1lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
      w1lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
      w0lat = WayPointList[Task[ActiveWayPoint-1].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint-1].Index].Longitude;

      if (AATEnabled) {
        w1lat = Task[ActiveWayPoint].AATTargetLat;
        w1lon = Task[ActiveWayPoint].AATTargetLon;
        w0lat = Task[ActiveWayPoint-1].AATTargetLat;
        w0lon = Task[ActiveWayPoint-1].AATTargetLon;
      }

      LegDistance =
        Distance(w1lat,
                 w1lon,
                 w0lat,
                 w0lon);

      LegToGo =
        Distance(Basic->Latitude,
                 Basic->Longitude,
                 w1lat,
                 w1lon);

      if (!StartLine && (ActiveWayPoint==1)) {
        // Correct speed calculations for radius
        LegDistance -= StartRadius;
      }

      LegCovered = LegDistance - LegToGo;

      if(LegCovered <=0)
        Calculated->TaskDistanceCovered = 0;
      else
        Calculated->TaskDistanceCovered = LegCovered;

      Calculated->LegDistanceToGo = LegToGo;
      Calculated->LegDistanceCovered = Calculated->TaskDistanceCovered;

      if(Basic->Time > Calculated->LegStartTime)
        Calculated->LegSpeed = Calculated->LegDistanceCovered
          / (Basic->Time - Calculated->LegStartTime);

      for(i=0;i<ActiveWayPoint-1;i++)
        {
          if (Task[i].Index<0) continue;
          if (Task[i+1].Index<0) continue;

          w1lat = WayPointList[Task[i].Index].Latitude;
          w1lon = WayPointList[Task[i].Index].Longitude;
          w0lat = WayPointList[Task[i+1].Index].Latitude;
          w0lon = WayPointList[Task[i+1].Index].Longitude;

          if (AATEnabled) {
            w1lat = Task[i].AATTargetLat;
            w1lon = Task[i].AATTargetLon;
            w0lat = Task[i+1].AATTargetLat;
            w0lon = Task[i+1].AATTargetLon;
          }

          LegDistance =
            Distance(w1lat,
                     w1lon,
                     w0lat,
                     w0lon);

          Calculated->TaskDistanceCovered += LegDistance;
        }

      if(Basic->Time > Calculated->TaskStartTime)
        Calculated->TaskSpeed =
          Calculated->TaskDistanceCovered
          / (Basic->Time - Calculated->TaskStartTime);
    }
  else
    {
      // haven't started task yet
      Calculated->TaskSpeed = 0;
      Calculated->TaskDistanceCovered = 0;
    }

  // Calculate Final Glide To Finish
  Calculated->TaskDistanceToGo = 0;
  Calculated->TaskTimeToGo = 0;
  if(ActiveWayPoint >=0)
    {
      i=ActiveWayPoint;

      int FinalWayPoint = getFinalWaypoint();

      // update final glide mode status
      if (
          ((ActiveWayPoint == FinalWayPoint)&&(ActiveWayPoint>=0))
          ||(ForceFinalGlide)
          ||(TaskAborted)) {
        // JMW on final glide
        if (Calculated->FinalGlide == 0)
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);
        Calculated->FinalGlide = 1;
      } else {
        if (Calculated->FinalGlide == 1)
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
        Calculated->FinalGlide = 0;
      }

      if (AATEnabled) {

        LegBearing = Bearing(Basic->Latitude ,
                             Basic->Longitude ,
                             Task[i].AATTargetLat,
                             Task[i].AATTargetLon);

        LegToGo = Distance(Basic->Latitude ,
                           Basic->Longitude ,
                             Task[i].AATTargetLat,
                             Task[i].AATTargetLon);
      } else {

        if (Task[i].Index>=0) {

          LegBearing = Bearing(Basic->Latitude ,
                               Basic->Longitude ,
                               WayPointList[Task[i].Index].Latitude,
                               WayPointList[Task[i].Index].Longitude);

          LegToGo = Distance(Basic->Latitude ,
                             Basic->Longitude ,
                             WayPointList[Task[i].Index].Latitude,
                             WayPointList[Task[i].Index].Longitude);
        }
      }

      LegAltitude =
        GlidePolar::MacCreadyAltitude(maccready,
                                      LegToGo,
                                      LegBearing,
                                      Calculated->WindSpeed,
                                      Calculated->WindBearing,
                                      &(Calculated->BestCruiseTrack),
                                      &(Calculated->VMacCready),
                                      (Calculated->FinalGlide==1),
                                      &(Calculated->LegTimeToGo)
                                      );

      if (Calculated->FinalGlide) {
        double lat, lon;
        double distancesoarable =
          FinalGlideThroughTerrain(LegBearing, Basic, Calculated,
                                   &lat,
                                   &lon,
                                   LegToGo);

        if (distancesoarable< LegToGo) {
          // JMW TODO issue terrain warning (audio?)
          Calculated->TerrainWarningLatitude = lat;
          Calculated->TerrainWarningLongitude = lon;

          fgttnew = true;
        } else {
          Calculated->TerrainWarningLatitude = 0.0;
          Calculated->TerrainWarningLongitude = 0.0;
          fgtt = false;
        }

      } else {
        Calculated->TerrainWarningLatitude = 0.0;
        Calculated->TerrainWarningLongitude = 0.0;
        fgtt = false;
      }

      TaskAltitudeRequired = LegAltitude;
      Calculated->TaskDistanceToGo = LegToGo;
      Calculated->TaskTimeToGo = Calculated->LegTimeToGo;

      double LegTimeToGo;

      if(  (Calculated->NavAltitude - LegAltitude - SAFETYALTITUDEARRIVAL) > 0)
        {
          Calculated->LDNext =
            Calculated->TaskDistanceToGo
            / (Calculated->NavAltitude - LegAltitude - SAFETYALTITUDEARRIVAL)  ;
        }
      else
        {
          Calculated->LDNext = 999;
        }

      i++;
      while((Task[i].Index != -1) && (i<MAXTASKPOINTS) && (!TaskAborted))
        {

          w1lat = WayPointList[Task[i].Index].Latitude;
          w1lon = WayPointList[Task[i].Index].Longitude;
          w0lat = WayPointList[Task[i-1].Index].Latitude;
          w0lon = WayPointList[Task[i-1].Index].Longitude;

          if (AATEnabled) {
            w1lat = Task[i].AATTargetLat;
            w1lon = Task[i].AATTargetLon;
            w0lat = Task[i-1].AATTargetLat;
            w0lon = Task[i-1].AATTargetLon;
          }

          LegDistance = Distance(w1lat,
                                 w1lon,
                                 w0lat,
                                 w0lon);

          LegBearing = Bearing(w1lat,
                               w1lon,
                               w0lat,
                               w0lon);

          LegAltitude = GlidePolar::
            MacCreadyAltitude(maccready,
                              LegDistance, LegBearing,
                              Calculated->WindSpeed,
                              Calculated->WindBearing,
                              0, 0,
                              (Calculated->FinalGlide==1),
                              &LegTimeToGo);

          TaskAltitudeRequired += LegAltitude;

          Calculated->TaskDistanceToGo += LegDistance;
          Calculated->TaskTimeToGo += LegTimeToGo;

          i++;
        }

      Calculated->TaskAltitudeRequired = TaskAltitudeRequired
        + SAFETYALTITUDEARRIVAL;

      Calculated->TaskAltitudeDifference = Calculated->NavAltitude
        - (Calculated->TaskAltitudeRequired
           + WayPointList[Task[i-1].Index].Altitude)
        + Calculated->EnergyHeight;

      if (Calculated->TaskAltitudeDifference>0) {
        if (!fgtt && fgttnew) {
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
          fgtt = true;
        }
      } else {
        fgtt = false;
      }

      double temp = Calculated->NavAltitude
        - WayPointList[Task[i-1].Index].Altitude
        + Calculated->EnergyHeight - SAFETYALTITUDEARRIVAL;

      if(  (temp) > 0)
        {
          Calculated->LDFinish = Calculated->TaskDistanceToGo
            /(temp);
        }
      else
        {
          Calculated->LDFinish = 999;
        }

      // Auto Force Final Glide forces final glide mode
      // if above final glide...
      if (AutoForceFinalGlide) {
        if (Calculated->TaskAltitudeDifference>0) {
          ForceFinalGlide = true;
        } else {
          ForceFinalGlide = false;
        }
      }

    } else {
    // no task selected, so work things out at current heading

    Calculated->FinalGlide = 0;

    GlidePolar::MacCreadyAltitude(maccready, 100.0, Basic->TrackBearing,
                                 Calculated->WindSpeed,
                                 Calculated->WindBearing,
                                 &(Calculated->BestCruiseTrack),
                                 &(Calculated->VMacCready),
                                  (Calculated->FinalGlide==1),
                                 0);

  }
  UnlockFlightData();

}


void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double tad=0.0;
  static double dmc=0.0;

  LockFlightData();

  double slope =
    (Calculated->NavAltitude
     - SAFETYALTITUDEARRIVAL
     - WayPointList[Task[ActiveWayPoint].Index].Altitude)/
    (Calculated->WaypointDistance+1);

  double mcp = PirkerAnalysis(Basic, Calculated,
                       Calculated->WaypointBearing,
                       slope);
  if (mcp>0) {
    MACCREADY = mcp;
  } else {
    MACCREADY = 0.0;
  }

  UnlockFlightData();

}

void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL BelowGlide = TRUE;
  static double delayedTAD = 0.0;

  delayedTAD = 0.95*delayedTAD+0.05*Calculated->TaskAltitudeDifference;

  if(BelowGlide == TRUE)
    {
      if(delayedTAD > 50)
        {
          BelowGlide = FALSE;
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_ABOVE);
        }
    }
  else
    {
      if(delayedTAD < 50)
        {
          BelowGlide = TRUE;

          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_BELOW);
        }
    }
}

extern int AIRSPACEWARNINGS;
extern int WarningTime;
extern int AcknowledgementTime;


void CalculateNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if(Calculated->Circling)
    {
      Calculated->NextLatitude = Basic->Latitude;
      Calculated->NextLongitude = Basic->Longitude;
      Calculated->NextAltitude = Calculated->NavAltitude + Calculated->Average30s * 30;
    }
  else
    {
      Calculated->NextLatitude = FindLatitude(Basic->Latitude,
                                              Basic->Longitude,
                                              Basic->TrackBearing,
                                              Basic->Speed*WarningTime );
      Calculated->NextLongitude = FindLongitude(Basic->Latitude,
                                                Basic->Longitude,
                                                Basic->TrackBearing,
                                                Basic->Speed*WarningTime);
      if (Basic->BaroAltitudeAvailable) {
        Calculated->NextAltitude = Basic->BaroAltitude + Calculated->Average30s * WarningTime;
      } else {
        Calculated->NextAltitude = Calculated->NavAltitude + Calculated->Average30s * WarningTime;
      }
    }
}


//////////////////////////////////////////////

bool GlobalClearAirspaceWarnings = false;

bool ClearAirspaceWarnings(bool ack, bool allday) {
  unsigned int i;
  if (ack) {
    GlobalClearAirspaceWarnings = true;
    for (i=0; i<NumberOfAirspaceCircles; i++) {
      if (AirspaceCircle[i].WarningLevel>0) {
              AirspaceCircle[i].Ack.AcknowledgementTime = GPS_INFO.Time;
              if (allday) {
                AirspaceCircle[i].Ack.AcknowledgedToday = true;
              }
              AirspaceCircle[i].WarningLevel = 0;
      }
    }
    for (i=0; i<NumberOfAirspaceAreas; i++) {
      if (AirspaceArea[i].WarningLevel>0) {
              AirspaceArea[i].Ack.AcknowledgementTime = GPS_INFO.Time;
              if (allday) {
                AirspaceArea[i].Ack.AcknowledgedToday = true;
              }
              AirspaceArea[i].WarningLevel = 0;
      }
    }
    return Message::Acknowledge(MSG_AIRSPACE);
  }
  return false;
}

#if 0
void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  unsigned int i;
  TCHAR szMessageBuffer[1024];
  TCHAR szTitleBuffer[1024];
  TCHAR text[1024];
  bool inside;

  if(!AIRSPACEWARNINGS)
      return;

  static bool next = false;

  LockFlightData();

  if (GlobalClearAirspaceWarnings == true) {
    GlobalClearAirspaceWarnings = false;
    Calculated->IsInAirspace = false;
  }

  next = !next;
  // every second time step, do predicted position rather than
  // current position

  double alt;
  double lat;
  double lon;

  if (next) {
    alt = Calculated->NextAltitude;
    lat = Calculated->NextLatitude;
    lon = Calculated->NextLongitude;
  } else {
    if (Basic->BaroAltitudeAvailable) {
      alt = Basic->BaroAltitude;
    } else {
      alt = Basic->Altitude;
    }
    lat = Basic->Latitude;
    lon = Basic->Longitude;
  }

  // JMW TODO: FindAirspaceCircle etc should sort results, return
  // the most critical or closest.

  for (i=0; i<NumberOfAirspaceCircles; i++) {
    inside = false;

    if ((alt >= AirspaceCircle[i].Base.Altitude )
              && (alt < AirspaceCircle[i].Top.Altitude)) {
            inside = InsideAirspaceCircle(lon, lat, i);

            if (MapWindow::iAirspaceMode[AirspaceCircle[i].Type]< 2) {
              // don't want warnings for this one
              inside = false;
      }

    }
    if (inside) { hash
      if (AirspaceCircle[i].WarningLevel>0) {
              // already warned
              continue;
      }

      if (AirspaceCircle[i].Ack.AcknowledgedToday) {
        continue;
      }
      if ((AirspaceCircle[i].Ack.AcknowledgementTime!=0) &&
	  /*
              ((Basic->Time-AirspaceCircle[i].Ack.AcknowledgementTime)<
               AcknowledgementTime)) {
            continue;
          }
          if (next) {
                  AirspaceCircle[i].WarningLevel |= 1;
          } else {
                  AirspaceCircle[i].WarningLevel |= 2;
          }
          */
	  ((Basic->Time-AirspaceCircle[i].Ack.AcknowledgementTime)<
	   AcknowledgementTime)) {
	continue;
      }

      int oldwarninglevel = AirspaceCircle[i].WarningLevel;

      if (next) {
	AirspaceCircle[i].WarningLevel |= 1;
      } else {
	AirspaceCircle[i].WarningLevel |= 2;
      }

      if (AirspaceCircle[i].WarningLevel > oldwarninglevel) {

#ifndef DISABLEAUDIO
	MessageBeep(MB_ICONEXCLAMATION);
#endif

	FormatWarningString(AirspaceCircle[i].Type , AirspaceCircle[i].Name ,
			    AirspaceCircle[i].Base, AirspaceCircle[i].Top,
			    szMessageBuffer, szTitleBuffer );

	wsprintf(text,TEXT("AIRSPACE: %s\r\n%s"),
		 szTitleBuffer,szMessageBuffer);

	// clear previous warning if any
	// Message::Acknowledge(MSG_AIRSPACE);
	Message::AddMessage(5000, MSG_AIRSPACE, text);

	InputEvents::processGlideComputer(GCE_AIRSPACE_ENTER);
      }
      Calculated->IsInAirspace = true;

    } else {
      if (AirspaceCircle[i].WarningLevel>0) {
              if (next) {
                if (AirspaceCircle[i].WarningLevel %2 == 1) {
                  AirspaceCircle[i].WarningLevel -= 1;
                }
              } else {
                if (AirspaceCircle[i].WarningLevel>1) {
                  AirspaceCircle[i].WarningLevel -= 2;
                }
              }
              if (AirspaceCircle[i].WarningLevel == 0) {

                AirspaceCircle[i].Ack.AcknowledgementTime =
                  Basic->Time-AcknowledgementTime;
                Message::Acknowledge(MSG_AIRSPACE);
                InputEvents::processGlideComputer(GCE_AIRSPACE_LEAVE);
                Calculated->IsInAirspace = false;
              }
      }
    }
  }

  // repeat process for areas

  for (i=0; i<NumberOfAirspaceAreas; i++) {
    inside = false;

    if ((alt >= AirspaceArea[i].Base.Altitude )
              && (alt < AirspaceArea[i].Top.Altitude)) {

      inside = InsideAirspaceArea(lon, lat, i);

      if (MapWindow::iAirspaceMode[AirspaceArea[i].Type]< 2) {
              // don't want warnings for this one
              inside = false;
      }

    }
    if (inside) {
      if (AirspaceArea[i].WarningLevel>0) {
              // already warned
              continue;
      }
      if (AirspaceArea[i].Ack.AcknowledgedToday) {
        continue;
      }
      if ((AirspaceArea[i].Ack.AcknowledgementTime!=0) &&
                ((Basic->Time-AirspaceArea[i].Ack.AcknowledgementTime)<
                 AcknowledgementTime)) {
              continue;
      }

      int oldwarninglevel = AirspaceArea[i].WarningLevel;

      if (next) {
        AirspaceArea[i].WarningLevel |= 1;
      } else {
        AirspaceArea[i].WarningLevel |= 2;
      }

      if (AirspaceArea[i].WarningLevel > oldwarninglevel) {

#ifndef DISABLEAUDIO
	MessageBeep(MB_ICONEXCLAMATION);
#endif
	FormatWarningString(AirspaceArea[i].Type , AirspaceArea[i].Name ,
			    AirspaceArea[i].Base, AirspaceArea[i].Top,
			    szMessageBuffer, szTitleBuffer );

	wsprintf(text,TEXT("AIRSPACE: %s\r\n%s"),
		 szTitleBuffer,szMessageBuffer);

	// clear previous warning if any
	// Message::Acknowledge(MSG_AIRSPACE);
	Message::AddMessage(5000, MSG_AIRSPACE, text);

	InputEvents::processGlideComputer(GCE_AIRSPACE_ENTER);
      }
      Calculated->IsInAirspace = true;

    } else {
      if (AirspaceArea[i].WarningLevel>0) {

              if (next) {
                if (AirspaceArea[i].WarningLevel %2 == 1) {
                  AirspaceArea[i].WarningLevel -= 1;
                }
              } else {
                if (AirspaceArea[i].WarningLevel>1) {
                  AirspaceArea[i].WarningLevel -= 2;
                }
              }
              if (AirspaceArea[i].WarningLevel == 0) {

                AirspaceArea[i].Ack.AcknowledgementTime =
                  Basic->Time-AcknowledgementTime;
                Message::Acknowledge(MSG_AIRSPACE);
                InputEvents::processGlideComputer(GCE_AIRSPACE_LEAVE);
                Calculated->IsInAirspace = false;
              }

      }
    }
  }
  UnlockFlightData();

}

#else // new style airspace warnings

// defined in AirspaceWarning.cpp

void AirspaceWarnListAdd(NMEA_INFO *Basic, int Sequence, bool Predicted, bool IsCircle, int AsIdx);
void AirspaceWarnListProcess(NMEA_INFO *Basic);


void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated){
  unsigned int i;

//  TCHAR text[1024];
  bool inside;

  if(!AIRSPACEWARNINGS)
      return;

  static bool predicted = false;
  static int  UpdateSequence = 0;

  LockFlightData();

  if (GlobalClearAirspaceWarnings == true) {
    GlobalClearAirspaceWarnings = false;
    Calculated->IsInAirspace = false;
  }

  predicted = !predicted;
  // every second time step, do predicted position rather than
  // current position

  if (!predicted){
    if (++UpdateSequence > 1000)
      UpdateSequence = 1;
  }

  double alt;
  double lat;
  double lon;

  if (predicted) {
    alt = Calculated->NextAltitude;
    lat = Calculated->NextLatitude;
    lon = Calculated->NextLongitude;
  } else {
    if (Basic->BaroAltitudeAvailable) {
      alt = Basic->BaroAltitude;
    } else {
      alt = Basic->Altitude;
    }
    lat = Basic->Latitude;
    lon = Basic->Longitude;
  }

  // JMW TODO: FindAirspaceCircle etc should sort results, return
  // the most critical or closest.

  for (i=0; i<NumberOfAirspaceCircles; i++) {
    inside = false;

    if ((alt >= AirspaceCircle[i].Base.Altitude )
              && (alt < AirspaceCircle[i].Top.Altitude)) {


      if (InsideAirspaceCircle(lon, lat, i) && (MapWindow::iAirspaceMode[AirspaceCircle[i].Type] >= 2)){
        AirspaceWarnListAdd(Basic, UpdateSequence, predicted, 1, i);
      }

    }

  }

  // repeat process for areas

  for (i=0; i<NumberOfAirspaceAreas; i++) {
    inside = false;

    if ((alt >= AirspaceArea[i].Base.Altitude )
              && (alt < AirspaceArea[i].Top.Altitude)) {

      if ((MapWindow::iAirspaceMode[AirspaceArea[i].Type] >= 2) && InsideAirspaceArea(lon, lat, i)){
        AirspaceWarnListAdd(Basic, UpdateSequence, predicted, 0, i);
      }

    }
  }


  AirspaceWarnListProcess(Basic);

  UnlockFlightData();

}
#endif
//////////////////////////////////////////////

void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double Temp;
  int i;
  double MaxDistance, MinDistance, TargetDistance;
  double LegToGo, LegDistance, TargetLegToGo, TargetLegDistance;
  double TaskAltitudeRequired = 0;

  if (!WayPointList) return ;

  if(!AATEnabled)
    {
      return;
    }

  LockFlightData();

  Temp = Basic->Time - Calculated->TaskStartTime;

  if ((ActiveWayPoint==0)&&(Calculated->AATTimeToGo==0)) {
    Calculated->AATTimeToGo = AATTaskLength*60;
  }

  if((Temp >=0)&&(ActiveWayPoint >0))
    {
      Calculated->AATTimeToGo = (AATTaskLength*60) - Temp;
      if(Calculated->AATTimeToGo <= 0)
        Calculated->AATTimeToGo = 0;
      if(Calculated->AATTimeToGo >= (AATTaskLength * 60) )
        Calculated->AATTimeToGo = (AATTaskLength * 60);
    }

  MaxDistance = 0; MinDistance = 0; TargetDistance = 0;
  // Calculate Task Distances

  //  Calculated->TaskDistanceToGo = 0;
  // JMW: not sure why this is here?

  if(ActiveWayPoint >=0)
    {
      i=ActiveWayPoint;

      LegToGo = Distance(Basic->Latitude , Basic->Longitude ,
                         WayPointList[Task[i].Index].Latitude,
                         WayPointList[Task[i].Index].Longitude);

      TargetLegToGo = Distance(Basic->Latitude , Basic->Longitude ,
                               Task[i].AATTargetLat,
                               Task[i].AATTargetLon);

      if(Task[ActiveWayPoint].AATType == CIRCLE)
        {
          MaxDistance = LegToGo + (Task[i].AATCircleRadius * 2);
          MinDistance = LegToGo - (Task[i].AATCircleRadius * 2);
        }
      else
        {
          MaxDistance = LegToGo + (Task[ActiveWayPoint].AATSectorRadius * 2);
          MinDistance = LegToGo;
        }

      TargetDistance = TargetLegToGo;

      i++;
      while((Task[i].Index != -1) && (i<MAXTASKPOINTS))
        {
          LegDistance = Distance(WayPointList[Task[i].Index].Latitude,
                                 WayPointList[Task[i].Index].Longitude,
                                 WayPointList[Task[i-1].Index].Latitude,
                                 WayPointList[Task[i-1].Index].Longitude);

          TargetLegDistance = Distance(Task[i].AATTargetLat,
                                       Task[i].AATTargetLon,
                                       Task[i-1].AATTargetLat,
                                       Task[i-1].AATTargetLon);

          if(Task[ActiveWayPoint].AATType == CIRCLE)
            {
              MaxDistance += LegDistance + (Task[i].AATCircleRadius * 2);
              MinDistance += LegDistance- (Task[i].AATCircleRadius * 2);
            }
          else
            {
              MaxDistance += LegDistance + (Task[ActiveWayPoint].AATSectorRadius * 2);
              MinDistance += LegDistance;
            }
          TargetDistance += TargetLegDistance;
          i++;
        }

      // JMW TODO: make this more accurate, because currently it is
      // very approximate.

      Calculated->AATMaxDistance = MaxDistance;
      Calculated->AATMinDistance = MinDistance;
      Calculated->AATTargetDistance = TargetDistance;

      if(Calculated->AATTimeToGo >0)
        {
          Calculated->AATMaxSpeed = Calculated->AATMaxDistance / Calculated->AATTimeToGo;
          Calculated->AATMinSpeed = Calculated->AATMinDistance / Calculated->AATTimeToGo;
          Calculated->AATTargetSpeed = Calculated->AATTargetDistance / Calculated->AATTimeToGo;
        }
    }
  UnlockFlightData();
}


void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  if(Basic->Time <= LastTime)
    {
      LastTime = Basic->Time;
      return;
    }
  LastTime = Basic->Time;


  // JMW TODO: Should really work out dt here, but i'm assuming constant time steps
  double dheight = Calculated->NavAltitude-SAFETYALTITUDEBREAKOFF;

  int index, i, j;

  if (dheight<0) {
    return; // nothing to do.
  }
  if (Calculated->MaxThermalHeight==0) {
    Calculated->MaxThermalHeight = dheight;
  }

  // only do this if in thermal and have been climbing
  if ((!Calculated->Circling)||(Calculated->Average30s<0)) return;

  LockFlightData();

  if (dheight > Calculated->MaxThermalHeight) {

    /* JMW oh, it is a bit annoying really
  if (EnableSoundTask) {
    PlayResource(TEXT("IDR_WAV_BEEPBWEEP"));
    }
    */

    // moved beyond ceiling, so redistribute buckets
    double mthnew;
    double tmpW[NUMTHERMALBUCKETS];
    int tmpN[NUMTHERMALBUCKETS];
    double h;

    // calculate new buckets so glider is below max
    double hbuk = Calculated->MaxThermalHeight/NUMTHERMALBUCKETS;

    mthnew = max(1, Calculated->MaxThermalHeight);
    while (mthnew<dheight) {
      mthnew += hbuk;
    }

    // reset counters
    for (i=0; i<NUMTHERMALBUCKETS; i++) {
      tmpW[i]= 0.0;
      tmpN[i]= 0;
    }
    // shift data into new buckets
    for (i=0; i<NUMTHERMALBUCKETS; i++) {
      h = (i)*(Calculated->MaxThermalHeight)/(NUMTHERMALBUCKETS);
      // height of center of bucket
      j = iround(NUMTHERMALBUCKETS*h/mthnew);

      //      h = (i)*(mthnew)/(NUMTHERMALBUCKETS); // height of center of bucket
      //      j = iround(NUMTHERMALBUCKETS*h/Calculated->MaxThermalHeight);

      if (j<NUMTHERMALBUCKETS) {
        if (Calculated->ThermalProfileN[i]>0) {
          tmpW[j] += Calculated->ThermalProfileW[i];
          tmpN[j] += Calculated->ThermalProfileN[i];
        }
      }
    }
    for (i=0; i<NUMTHERMALBUCKETS; i++) {
      Calculated->ThermalProfileW[i]= tmpW[i];
      Calculated->ThermalProfileN[i]= tmpN[i];
    }
    Calculated->MaxThermalHeight= mthnew;
  }

  index = iround(NUMTHERMALBUCKETS*(dheight/max(1.0,
                            Calculated->MaxThermalHeight)));
  if (index==NUMTHERMALBUCKETS) {
    index= NUMTHERMALBUCKETS-1;
  }

  Calculated->ThermalProfileW[index]+= Calculated->Vario;
  Calculated->ThermalProfileN[index]++;
  UnlockFlightData();

}



//////////////////////////////////////////////////////////
// Final glide through terrain and footprint calculations

void ExitFinalGlideThroughTerrain() {
  UnlockTerrainDataCalculations();
}


double FinalGlideThroughTerrain(double bearing, NMEA_INFO *Basic,
                                DERIVED_INFO *Calculated,
                                double *retlat, double *retlon,
                                double maxrange)
{
  double ialtitude = GlidePolar::MacCreadyAltitude(MACCREADY,
                                 // should this be zero?
                                                   1.0, bearing,
                                                   Calculated->WindSpeed,
                                                   Calculated->WindBearing,
                                                   0, 0, true, 0);
  if (retlat && retlon) {
    *retlat = 0;
    *retlon = 0;
  }

  if (ialtitude<=0.0)
    return 0;

  double glidemaxrange = Calculated->NavAltitude/ialtitude;
  if (glidemaxrange<=0.0)
    return 0;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude
  double lat, lon;
  double latlast, lonlast;
  double h=0.0, dh=0.0;
  int imax=0;
  double dhlast=0;
  double altitude;

  LockTerrainDataCalculations();

  // calculate terrain rounding factor

  lat = FindLatitude(Basic->Latitude, Basic->Longitude, 0,
                     glidemaxrange/NUMFINALGLIDETERRAIN);
  lon = FindLongitude(Basic->Latitude, Basic->Longitude, 90,
                      glidemaxrange/NUMFINALGLIDETERRAIN);
  double Xrounding = fabs(lon-Basic->Longitude)/2;
  double Yrounding = fabs(lat-Basic->Latitude)/2;
  terrain_dem_calculations.SetTerrainRounding(Xrounding, Yrounding);

  // find grid
  double dlat = FindLatitude(Basic->Latitude, Basic->Longitude, bearing,
                                glidemaxrange)-Basic->Latitude;
  double dlon = FindLongitude(Basic->Latitude, Basic->Longitude, bearing,
                                 glidemaxrange)-Basic->Longitude;

  for (int i=0; i<=NUMFINALGLIDETERRAIN; i++) {
    double fi = (i*1.0)/NUMFINALGLIDETERRAIN;

    if ((maxrange>0)&&(fi>maxrange/glidemaxrange)) {
      // early exit
      ExitFinalGlideThroughTerrain();
      return maxrange;
    }

    altitude = (1.0-fi)*Calculated->NavAltitude;

    // find lat, lon of point of interest

    lat = Basic->Latitude+dlat*fi;
    lon = Basic->Longitude+dlon*fi;

    // find height over terrain
    h =  terrain_dem_calculations.GetTerrainHeight(lat, lon);

    dh = altitude - h -  SAFETYALTITUDETERRAIN;

    if ((dh<=0)&&(dhlast>0)) {
      double f;
      if (dhlast-dh>0) {
        f = (-dhlast)/(dh-dhlast);
      } else {
        f = 0.0;
      }
      if (retlat && retlon) {
        *retlat = latlast*(1.0-f)+lat*f;
        *retlon = lonlast*(1.0-f)+lon*f;
      }
      ExitFinalGlideThroughTerrain();
      return Distance(Basic->Latitude, Basic->Longitude, lat, lon);
    }
    dhlast = dh;
    latlast = lat;
    lonlast = lon;
  }

  ExitFinalGlideThroughTerrain();
  return glidemaxrange;
}


//////////////////////////////////////////////////////////////////


void LatLon2Flat(double lon, double lat, int *scx, int *scy) {
  *scx = (int)(lon*fastcosine(lat)*100);
  *scy = (int)(lat*100);
}

int CalculateWaypointApproxDistance(int scx_aircraft, int scy_aircraft,
                                    int i) {

  // Do preliminary fast search
  int scx, scy;
  LatLon2Flat(WayPointList[i].Longitude,
              WayPointList[i].Latitude, &scx, &scy);
  int dx, dy;
  dx = scx_aircraft-scx;
  dy = scy_aircraft-scy;

  return isqrt4(dx*dx+dy*dy);
}



double CalculateWaypointArrivalAltitude(NMEA_INFO *Basic,
                                        DERIVED_INFO *Calculated,
                                        int i) {
  double AltReqd;
  double wDistance, wBearing;

  wDistance = Distance(Basic->Latitude,
                       Basic->Longitude,
                       WayPointList[i].Latitude,
                       WayPointList[i].Longitude);

  wBearing = Bearing(Basic->Latitude,
                     Basic->Longitude,
                     WayPointList[i].Latitude,
                     WayPointList[i].Longitude);

  AltReqd = GlidePolar::MacCreadyAltitude
    (GlidePolar::AbortSafetyMacCready(),
     wDistance,
     wBearing,
     Calculated->WindSpeed,
     Calculated->WindBearing,
     0,
     0,
     true,
     0);

  return ((Calculated->NavAltitude) - AltReqd
          - WayPointList[i].Altitude - SAFETYALTITUDEARRIVAL);
}



void SortLandableWaypoints(NMEA_INFO *Basic,
                           DERIVED_INFO *Calculated)
{
  int SortedLandableIndex[MAXTASKPOINTS];
  double SortedArrivalAltitude[MAXTASKPOINTS];
  int SortedApproxDistance[MAXTASKPOINTS*2];
  int SortedApproxIndex[MAXTASKPOINTS*2];
  int i, k, l;
  double aa;
  int ai;

  if (!WayPointList) return;

  LockFlightData();

  // Do preliminary fast search
  int scx_aircraft, scy_aircraft;
  LatLon2Flat(Basic->Longitude, Basic->Latitude, &scx_aircraft, &scy_aircraft);

  for (i=0; i<MAXTASKPOINTS*2; i++)
    {
      SortedApproxIndex[i]= -1;
      SortedApproxDistance[i] = 0;
    }

  for (i=0; i<(int)NumberOfWayPoints; i++)
    {
      if (!(((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
            ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT)))
      {
        continue; // ignore non-landable fields
      }

      ai = CalculateWaypointApproxDistance(scx_aircraft, scy_aircraft, i);

      // see if this fits into slot
      for (k=0; k< MAXTASKPOINTS*2; k++)
      {
        if (((ai < SortedApproxDistance[k]) ||  // closer than this one
             (SortedApproxIndex[k]== -1))       &&              // or this one isn't filled
            (SortedApproxIndex[k]!= i))                                 // and not replacing with same
        {
            // ok, got new biggest, put it into the slot.
          for (l=MAXTASKPOINTS*2-1; l>k; l--)
          {
            if (l>0)
              {
                SortedApproxDistance[l] = SortedApproxDistance[l-1];
                SortedApproxIndex[l] = SortedApproxIndex[l-1];
              }
          }

          SortedApproxDistance[k] = ai;
          SortedApproxIndex[k] = i;
          k=MAXTASKPOINTS*2;
        }
      }
    }

  // Now do detailed search
  for (i=0; i<MAXTASKPOINTS; i++)
    {
      SortedLandableIndex[i]= -1;
      SortedArrivalAltitude[i] = 0;
    }

  int scanairportsfirst;
  bool foundreachableairport = false;

  for (scanairportsfirst=0; scanairportsfirst<2; scanairportsfirst++)
    {
      if (foundreachableairport)
      {
        continue; // don't bother filling the rest of the list
      }

      for (i=0; i<MAXTASKPOINTS*2; i++)
        {
        if (SortedApproxIndex[i]<0)                     // ignore invalid points
          {
            continue;
          }

          if (((WayPointList[SortedApproxIndex[i]].Flags & LANDPOINT) == LANDPOINT) &&
              (scanairportsfirst==0))
            {
              // we are in the first scan, looking for airports only
              continue;
            }

          aa = CalculateWaypointArrivalAltitude(Basic,
                                                Calculated,
                                       SortedApproxIndex[i]);

          if (scanairportsfirst==0)
            {
              if (aa<0)
                {
                  // in first scan, this airport is unreachable, so ignore it.
                  continue;
                }
              else
                {
                  // this airport is reachable
                  foundreachableairport = true;
                }
            }

          // see if this fits into slot
          for (k=0; k< MAXTASKPOINTS; k++)
            {
              if (((aa > SortedArrivalAltitude[k]) ||   // closer than this one
                   (SortedLandableIndex[k]== -1))       &&      // or this one isn't filled
                  (SortedLandableIndex[k]!= i))                         // and not replacing with same
                {

                  double LegBearing =
                    Bearing(Basic->Latitude , Basic->Longitude ,
                            WayPointList[SortedApproxIndex[i]].Latitude,
                            WayPointList[SortedApproxIndex[i]].Longitude);
                  double LegToGo =
                    Distance(Basic->Latitude , Basic->Longitude ,
                             WayPointList[SortedApproxIndex[i]].Latitude,
                             WayPointList[SortedApproxIndex[i]].Longitude);

                  double distancesoarable =
                    FinalGlideThroughTerrain(LegBearing, Basic, Calculated,
                                             NULL,
                                             NULL,
                                             LegToGo);

                  if ((distancesoarable>= LegToGo)||(aa<0)) {
                    // only put this in the index if it is reachable
                    // and doesn't go through terrain, OR, if it is unreachable
                    // it doesn't matter if it goes through terrain because
                    // pilot has to climb first anyway

                    // ok, got new biggest, put it into the slot.
                    for (l=MAXTASKPOINTS-1; l>k; l--)
                      {
                        if (l>0)
                          {
                            SortedArrivalAltitude[l] = SortedArrivalAltitude[l-1];
                            SortedLandableIndex[l] = SortedLandableIndex[l-1];
                          }
                      }

                    SortedArrivalAltitude[k] = aa;
                    SortedLandableIndex[k] = SortedApproxIndex[i];
                    k=MAXTASKPOINTS;
                  }
                }
            }
        }
    }

  // now we have a sorted list.
  // check if current waypoint is in the sorted list
  int foundActiveWayPoint = -1;
  for (i=0; i<MAXTASKPOINTS; i++)
    {
      if (ActiveWayPoint>=0)
        {
          if (SortedLandableIndex[i] == Task[ActiveWayPoint].Index)
            {
              foundActiveWayPoint = i;
            }
        }
    }

  if (foundActiveWayPoint != -1)
    {
      ActiveWayPoint = foundActiveWayPoint;
    }
  else
    {
      // if not found, keep on field or set active waypoint to closest
      if (ActiveWayPoint>=0)
        {
          aa = CalculateWaypointArrivalAltitude(Basic, Calculated,
                                                Task[ActiveWayPoint].Index);
        }

      if (aa <= 0)
        {
          DoStatusMessage(gettext(TEXT("Closest Airfield Changed!")));
          ActiveWayPoint = 0;
        }
      else
        {
          if (ActiveWayPoint>=0)
            {
              SortedLandableIndex[MAXTASKPOINTS-1] = Task[ActiveWayPoint].Index;
            }
          else
            {
              // JMW not sure this is right..
              SortedLandableIndex[MAXTASKPOINTS-1] = 0;
            }

          ActiveWayPoint = MAXTASKPOINTS-1;
        }
    }

  // set new waypoints in task

  for (i=0; i<MAXTASKPOINTS; i++)
    {
      Task[i].Index = SortedLandableIndex[i];
    }
  UnlockFlightData();
}


void ResumeAbortTask(int set) {
  static int OldTask[MAXTASKPOINTS];
  static int OldActiveWayPoint= -1;
  static bool OldAATEnabled= false;
  int i;

  bool oldTaskAborted = TaskAborted;

  LockFlightData();

  if (set == 0)
        TaskAborted = !TaskAborted;
  else if (set > 0)
        TaskAborted = true;
  else if (set < 0)
        TaskAborted = false;

  if (oldTaskAborted != TaskAborted) {
    if (TaskAborted) {

      // save current task in backup

      for (i=0; i<MAXTASKPOINTS; i++) {
        OldTask[i]= Task[i].Index;
      }
      OldActiveWayPoint = ActiveWayPoint;
      if (AATEnabled) {
        OldAATEnabled = true;
      } else {
        OldAATEnabled = false;
      }

      // force new waypoint to be the closest
      ActiveWayPoint = -1;

      // force AAT off
      AATEnabled = false;

      // set MacCready
      if (!GlidePolar::AbortSafetyUseCurrent)  // 20060520:sgi added
        MACCREADY = min(MACCREADY,GlidePolar::AbortSafetyMacCready());

    } else {

      // reload backup task

      for (i=0; i<MAXTASKPOINTS; i++) {
        Task[i].Index = OldTask[i];
      }
      ActiveWayPoint = OldActiveWayPoint;
      AATEnabled = OldAATEnabled;

      RefreshTask();
    }
  }
  UnlockFlightData();

}


#define TAKEOFFSPEEDTHRESHOLD (0.5*GlidePolar::Vminsink)

void DoAutoQNH(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static bool done_autoqnh = false;

  // Reject if already done
  if (done_autoqnh) return;

  // Reject if in IGC logger mode
  if (ReplayLogger::IsEnabled()) return;

  // Reject if no valid GPS fix
  if (Basic->NAVWarning) return;

  // Reject if no baro altitude
  if (!Basic->BaroAltitudeAvailable) return;

  // Reject if terrain height is invalid
  if (!Calculated->TerrainValid) return;

  double fixaltitude = Calculated->TerrainAlt;

  QNH = FindQNH(Basic->BaroAltitude, fixaltitude);
  done_autoqnh = true;
}


void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static int ntimeinflight = 0;
  static int ntimeonground = 0;

  if (Basic->Speed> TAKEOFFSPEEDTHRESHOLD) {
    ntimeinflight++;
    ntimeonground=0;
  } else {
    if ((Calculated->AltitudeAGL<300)&&(Calculated->TerrainValid)) {
      ntimeinflight--;
    }
    ntimeonground++;
  }

  ntimeinflight = min(30, max(0,ntimeinflight));
  ntimeonground = min(30, max(0,ntimeonground));

  // JMW logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds
  //
  //   detect landing when below threshold speed for 30 seconds
  //
  // TODO: make this more robust by making use of terrain height data
  // if available

  if ((ntimeonground<=10)||(ReplayLogger::IsEnabled())) {
    // Don't allow 'OnGround' calculations if in IGC replay mode
    Calculated->OnGround = FALSE;
  }

  if (!Calculated->Flying) {
    // detect takeoff
    if (ntimeinflight>10) {
      Calculated->Flying = TRUE;
      InputEvents::processGlideComputer(GCE_TAKEOFF);

      // reset stats on takeoff
      ResetFlightStats(Basic, Calculated);
    }
    if (ntimeonground>10) {
      Calculated->OnGround = TRUE;
      DoAutoQNH(Basic, Calculated);
    }
  } else {
    // detect landing
    if (ntimeinflight==0) {
      // have been stationary for a minute
      Calculated->Flying = FALSE;
      InputEvents::processGlideComputer(GCE_LANDING);
    }

  }

}


////////////

double PirkerAnalysis(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      double bearing,
                      double GlideSlope) {

  bool maxfound = false;
  bool first = true;
  double pmc = 0.0;
  double htarget = GlideSlope;
  double h;
  double dh= 1.0;
  double pmclast = 5.0;
  double dhlast = -1.0;
  double pmczero = 0.0;

  while (pmc<10.0) {

    h = GlidePolar::MacCreadyAltitude(pmc,
                                      // should this be zero?
                                      1.0, bearing,
                                      Calculated->WindSpeed,
                                      Calculated->WindBearing,
                                      0, 0, true, 0);

    dh = (htarget-h);
    // height difference, how much we have compared to
    // how much we need at that speed.
    //   dh>0, we can afford to speed up

    if (dh==dhlast) {
      // same height, must have hit max speed.
      if (dh>0) {
        return pmclast;
      } else {
        return 0.0;
      }
    }

    if ((dh<=0)&&(dhlast>0)) {
      double f = (-dhlast)/(dh-dhlast);
      pmczero = pmclast*(1.0-f)+f*pmc;
      return pmczero;
    }
    dhlast = dh;
    pmclast = pmc;

    pmc += 0.5;
  }
  return -1.0; // no solution found, unreachable without further climb
}
