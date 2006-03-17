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

WindAnalyser *windanalyser = NULL;

#include "Port.h"

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
static void DoAutoMacCready(DERIVED_INFO *Calculated);
static void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void SortLandableWaypoints(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double bearing, distance;
  double lat, lon;
  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    bearing = -90+i*180.0/NUMTERRAINSWEEPS+Basic->TrackBearing;
    distance = FinalGlideThroughTerrain(bearing, 
					Basic, 
					Calculated, &lat, &lon);
    MapWindow::GlideFootPrint[i].x = lon;
    MapWindow::GlideFootPrint[i].y = lat;
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

  SnailTrail[SnailNext].Latitude = Basic->Latitude;
  SnailTrail[SnailNext].Longitude = Basic->Longitude;
        
  // JMW TODO: if circling, color according to 30s average?
  if (Basic->NettoVarioAvailable) {
    SnailTrail[SnailNext].Vario = Basic->NettoVario ;
  } else {
    SnailTrail[SnailNext].Vario = Calculated->Vario ;
  }

  SnailNext ++;
  SnailNext %= TRAILSIZE;

}


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
    dtSnail = 2.0;
  } else {
    dtSnail = 5.0;
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
      least_squares_update(Basic->Time/3600.0, Basic->Altitude);
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

  // TODO: Stall warning

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
    // TODO: Work out effect on maccready speed to be in speed error
    // and the volume should be scaled by this.
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

    if (EnableCalibration) {
      mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
      if ((Basic->AirspeedAvailable) && (Basic->IndicatedAirspeed>0)) {
        
        double k = (mag / Basic->TrueAirspeed);
        
        char buffer[200];
	sprintf(buffer,"%g %g %g %g %g %g %g %g %g # airspeed\r\n",
                Basic->IndicatedAirspeed, 
                mag*Basic->IndicatedAirspeed/Basic->TrueAirspeed, 
		k,
		Basic->Speed, 
		Calculated->WindSpeed,
		Calculated->WindBearing,
		Basic->Gload,
		Basic->BaroAltitude,
		Basic->Altitude);
        DebugStore(buffer);
		
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


BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double maccready;
  static double LastOptimiseTime = 0;

  if (!windanalyser) {
    windanalyser = new WindAnalyser(Basic, Calculated);

    // seed initial wind store with current conditions
    SetWindEstimate(Calculated->WindSpeed,Calculated->WindBearing);

  }

  maccready = MACCREADY;

  DistanceToNext(Basic, Calculated);
  EnergyHeight(Basic, Calculated);
  AltitudeRequired(Basic, Calculated, maccready);
  Heading(Basic, Calculated);

  TerrainHeight(Basic, Calculated);

  if (TaskAborted) {
    SortLandableWaypoints(Basic, Calculated);
  } 
  TaskStatistics(Basic, Calculated, maccready);

  if(Basic->Time <= LastTime)
    {

      if (Basic->Time<LastTime) {
	// Reset statistics.. (probably due to being in IGC replay mode)
	flightstats.Reset();
      }

      LastTime = Basic->Time; 
      return FALSE;      
    }

  LastTime = Basic->Time;

  if ((Calculated->FinalGlide)
      ||(fabs(Calculated->TaskAltitudeDifference)>30)) {
    FinalGlideAlert(Basic, Calculated);
    if (Calculated->AutoMacCready) {
      DoAutoMacCready(Calculated);
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
    //    TaskStatistics(Basic, Calculated, maccready);

  } else {

    InSector(Basic, Calculated);
    InAATSector(Basic, Calculated);

    AATStats(Basic, Calculated);  
    TaskStatistics(Basic, Calculated, maccready);

  }

  AltitudeRequired(Basic, Calculated, maccready);
  
  TerrainHeight(Basic, Calculated);
               
  TerrainFootprint(Basic, Calculated);
   
  CalculateNextPosition(Basic, Calculated);

  AirspaceWarning(Basic, Calculated);

  DoLogging(Basic, Calculated);

  // moved from MapWindow.cpp
  if(Basic->Time> LastOptimiseTime+15.0)
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

  return TRUE;
}


void EnergyHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated) 
{
  if (Basic->AirspeedAvailable) {
    Calculated->EnergyHeight = 
      (Basic->IndicatedAirspeed*Basic->IndicatedAirspeed
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
      Gain = Basic->Altitude - LastAlt;

      if (!Basic->VarioAvailable) {
        // estimate value from GPS
        Calculated->Vario = Gain / (Basic->Time - LastTime);
      } else {
        // get value from instrument
        Calculated->Vario = Basic->Vario;
        // we don't bother with sound here as it is polled at a 
        // faster rate in the DoVarioCalcs methods
      }

      LastAlt = Basic->Altitude;
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

          Altitude[temp] = Basic->Altitude;
        }
      temp = (long)Basic->Time - 1;
      temp = temp%30;
      Gain = Altitude[temp];
                
      temp = (long)Basic->Time;
      temp = temp%30;
      Gain = Gain - Altitude[temp];

      LastTime = Basic->Time;
      Calculated->Average30s = Gain/30;
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
      Gain = Basic->Altitude - Calculated->ClimbStartAlt;
      Calculated->AverageThermal  = Gain / (Basic->Time - Calculated->ClimbStartTime);
    }
}

void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if(Basic->Time > Calculated->ClimbStartTime)
    {
      Calculated->ThermalGain = Basic->Altitude - Calculated->ClimbStartAlt;
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
      AltLost = LastAlt - Basic->Altitude;
      if(AltLost > 0)
        {
          Calculated->LD = DistanceFlown / AltLost;
          if(Calculated->LD>999)
            {
              Calculated->LD = 999;
            }
        }
      else if (AltLost < 0) {
        // JMW added negative LD calculations TODO: TEST
        
        Calculated->LD = DistanceFlown / AltLost;
        if (Calculated->LD<-999) {
          Calculated->LD = 999;
        }
      } else {
        Calculated->LD = 999;
      }

      LastLat = Basic->Latitude;
      LastLon = Basic->Longitude;
      LastAlt = Basic->Altitude;
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
      AltLost = Calculated->CruiseStartAlt - Basic->Altitude;
      if(AltLost > 0)
        {
          Calculated->CruiseLD = DistanceFlown / AltLost;
          if(Calculated->CruiseLD>999)
            {
              Calculated->CruiseLD = 999;
            }
        }
      // JMW added negative LD calculations TODO: TEST
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

  windanalyser->slot_newFlightMode(left, 0);
  
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
  // JMW TODO: switch flight modes if changing direction?

  LastTime = Basic->Time;
  LastTrack = Basic->TrackBearing;

  double temp = StartTime;

  switch(MODE) {
  case CRUISE:
    if(Rate >= MinTurnRate) {
      StartTime = Basic->Time;
      StartLong = Basic->Longitude;
      StartLat  = Basic->Latitude;
      StartAlt  = Basic->Altitude;
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
	// into InputEvents instead
	SwitchZoomClimb(true, LEFT);
	InputEvents::processGlideComputer(GCE_FLIGHTMODE_CLIMB);
      }
    } else {
      // nope, not turning, so go back to cruise
      MODE = CRUISE;
    }
    break;
  case CLIMB:
    windanalyser->slot_newSample();
    
    if(Rate < MinTurnRate) {
      StartTime = Basic->Time;
      StartLong = Basic->Longitude;
      StartLat  = Basic->Latitude;
      StartAlt  = Basic->Altitude;
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
  windanalyser->slot_Altitude();

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
}

void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready)
{
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
        Basic->Altitude - (Calculated->NextAltitudeRequired
                           + WayPointList[Task[ActiveWayPoint].Index].Altitude)         + Calculated->EnergyHeight;            
    }
  else
    {
      Calculated->NextAltitudeRequired = 0;
      Calculated->NextAltitudeDifference = 0;
    }
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


int InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
		   int i)
{
  static int LastInSector = FALSE;
  double AircraftBearing;
  double FirstPointDistance;

  if (!WayPointList) return FALSE;

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
  if (AutoAdvance==1) {
    AdvanceArmed = false;
    return true;
  }
  if ((AutoAdvance==2)&&(AdvanceArmed)) {
    AdvanceArmed = false;
    return true;
  }
  if (AutoAdvance==3) {
    if (ActiveWayPoint>0) {
      AdvanceArmed = false;
      return true;
    }
    if (AdvanceArmed) {
      AdvanceArmed = false;
      return true;
    }
  }
  return false;
}




void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL StartSectorEntered = FALSE;
  static bool TaskFinished = false;

  if(AATEnabled)
    return;

  if(ActiveWayPoint == 0)
    {
      TaskFinished = false;
      if(InStartSector(Basic,Calculated))
        {
          StartSectorEntered = TRUE;
        }
      else
        {
          if(StartSectorEntered == TRUE)
            {
              if(ActiveWayPoint < MAXTASKPOINTS)
                {
                  if(Task[ActiveWayPoint+1].Index >= 0)
                    {
		      if (ReadyToAdvance()) {
			ActiveWayPoint++;
			InputEvents::processGlideComputer(GCE_TASK_START);
			AnnounceWayPointSwitch();
		      }
		      TaskFinished = false;
		      StartSectorEntered = FALSE;
		      Calculated->TaskStartTime = Basic->Time ;
                      Calculated->LegStartTime = Basic->Time;
                    }
                }
            }
        }
    }
  else if(ActiveWayPoint >0)
    {
      // JMW what does this do? restart?
      if(InStartSector(Basic, Calculated))
        {
          if(Basic->Time - Calculated->TaskStartTime < 600)
            {
	      if (ReadyToAdvance()) {
		AdvanceArmed = false;	
		ActiveWayPoint = 0;
		StartSectorEntered = TRUE;
	      }
	      TaskFinished = false;
            }
        }

      if(ActiveWayPoint < MAXTASKPOINTS) {
	if(Task[ActiveWayPoint+1].Index >= 0) {
	  if(InTurnSector(Basic,Calculated)) {
	    Calculated->LegStartTime = Basic->Time;
	    
	    if (ReadyToAdvance()) {
	      ActiveWayPoint++;
	      AnnounceWayPointSwitch();
	      InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
	    }
	    TaskFinished = false;
	    
	    return;
	  }
	} else {
	  if (InFinishSector(Basic,Calculated, ActiveWayPoint)) {
	    if (!TaskFinished) {
	      AnnounceWayPointSwitch();
	      InputEvents::processGlideComputer(GCE_TASK_FINISH);
	      TaskFinished = true;
	    }
	  } else {
	    //	    TaskFinished = false;
	  }
	}
      }
    }                   
}

void InAATSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL StartSectorEntered = FALSE;
  static bool TaskFinished = false;

  if(!AATEnabled)
    return;

  if(ActiveWayPoint == 0)
    {
      TaskFinished = false;
      if(InStartSector(Basic,Calculated))
        {
          StartSectorEntered = TRUE;
        }
      else
        {
          if(StartSectorEntered == TRUE)
            {
              if(ActiveWayPoint < MAXTASKPOINTS)
                {
                  if(Task[ActiveWayPoint+1].Index >= 0)
                    {
		      if (ReadyToAdvance()) {
			ActiveWayPoint++;
			InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
			AnnounceWayPointSwitch();
		      }
		      TaskFinished = false;
		      StartSectorEntered = FALSE;
                      Calculated->TaskStartTime = Basic->Time ;
                      Calculated->LegStartTime = Basic->Time;
		      // JMW TODO: make sure this is valid for manual start
                    }
                }
            }
        }
    }
  else if(ActiveWayPoint >0)
    {
      if(InStartSector(Basic, Calculated))
        {
          if(Basic->Time - Calculated->TaskStartTime < 600)
	    // this allows restart if returned to start sector before
	    // 10 minutes after task start
            {
	      if (ReadyToAdvance()) {
		ActiveWayPoint = 0;
		StartSectorEntered = TRUE;
	      }
	      TaskFinished = false;
            }
        }
      if(ActiveWayPoint < MAXTASKPOINTS) {
	if(Task[ActiveWayPoint+1].Index >= 0) {
	  if(InAATTurnSector(Basic,Calculated)) {
	    Calculated->LegStartTime = Basic->Time;

	    if (ReadyToAdvance()) {
	      AdvanceArmed = false;		
	      ActiveWayPoint++;
	      InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
	      
	      AnnounceWayPointSwitch();
	    }
	    TaskFinished = false;

	    return;
	  }
	} else {
	  if (InFinishSector(Basic,Calculated, ActiveWayPoint)) {
	    if (!TaskFinished) {
	      AnnounceWayPointSwitch();
	      InputEvents::processGlideComputer(GCE_TASK_FINISH);
	      TaskFinished = true;
	    }
	  } else {
	    //	    TaskFinished = false;
	  }
	}	
      }
    }
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
  // TODO: check this rounding is OK.
  terrain_dem_calculations.SetTerrainRounding(0);
  Alt = terrain_dem_calculations.
    GetTerrainHeight(Basic->Latitude , Basic->Longitude);
  UnlockTerrainDataCalculations();

  if(Alt<0) {
    Alt = 0; 
    Calculated->TerrainValid = false; 
  } else {
    Calculated->TerrainValid = true;
  }

  Calculated->TerrainAlt = Alt;
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
      
      LegCovered = LegDistance - LegToGo;
      
      if(LegCovered <=0)
        Calculated->TaskDistanceCovered = 0;
      else
        Calculated->TaskDistanceCovered = LegCovered;	
      
      Calculated->LegDistanceToGo = LegToGo;
      Calculated->LegDistanceCovered = Calculated->TaskDistanceCovered;
      
      if(Basic->Time != Calculated->LegStartTime) 
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
                
      if(Basic->Time != Calculated->TaskStartTime) 
        Calculated->TaskSpeed = 
          Calculated->TaskDistanceCovered 
          / (Basic->Time - Calculated->TaskStartTime); 
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
          ((ActiveWayPoint == FinalWayPoint)
           &&(ActiveWayPoint>=0))
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

      // JMW TODO: use instantaneous maccready here again to calculate
      // dolphin speed to fly 
      LegAltitude = 
	GlidePolar::MacCreadyAltitude(maccready, 
				      LegToGo, 
				      LegBearing, 
				      Calculated->WindSpeed, 
				      Calculated->WindBearing,
				      &(Calculated->BestCruiseTrack),
				      &(Calculated->VMacCready),
				      (i==FinalWayPoint),
				      &(Calculated->LegTimeToGo)
				      // ||()
				      // JMW TODO!!!!!!!!!!!
				      );

      if ((i==FinalWayPoint)||(TaskAborted)) {
        double lat, lon;
        double distancesoarable = 
          FinalGlideThroughTerrain(LegBearing, Basic, Calculated, 
                                   &lat,
                                   &lon);

        if (distancesoarable< LegToGo) {
          // JMW TODO display terrain warning
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

      if(  (Basic->Altitude - LegAltitude - SAFETYALTITUDEARRIVAL) > 0)
        {
          Calculated->LDNext = 
	    Calculated->TaskDistanceToGo 
	    / (Basic->Altitude - LegAltitude - SAFETYALTITUDEARRIVAL)  ;
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
			      Calculated->WindBearing, 0, 0,
			      (i==FinalWayPoint), // ||() JMW TODO!!!!!!!!!
			      &LegTimeToGo);
                        
          TaskAltitudeRequired += LegAltitude;

          Calculated->TaskDistanceToGo += LegDistance;
          Calculated->TaskTimeToGo += LegTimeToGo;      
                        
          i++;
        }

      Calculated->TaskAltitudeRequired = TaskAltitudeRequired 
	+ SAFETYALTITUDEARRIVAL;

      Calculated->TaskAltitudeDifference = Basic->Altitude 
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

      if(  (Basic->Altitude - WayPointList[Task[i-1].Index].Altitude  
	    + Calculated->EnergyHeight) > 0)
        {
          Calculated->LDFinish = Calculated->TaskDistanceToGo 
	    / (Basic->Altitude - WayPointList[Task[i-1].Index].Altitude 
	       + Calculated->EnergyHeight)  ;
        }
      else
        {
          Calculated->LDFinish = 999;
        }

    } else { 
    // no task selected, so work things out at current heading

    Calculated->FinalGlide = 0;
    
    GlidePolar::MacCreadyAltitude(maccready, 100.0, Basic->TrackBearing, 
                                 Calculated->WindSpeed, 
                                 Calculated->WindBearing, 
                                 &(Calculated->BestCruiseTrack),
                                 &(Calculated->VMacCready),
                                 false,
                                 // ||()
                                 // JMW TODO!!!!!!!!!!!
                                 0);

  }

}


void DoAutoMacCready(DERIVED_INFO *Calculated)
{
  static double tad=0.0;
  static double dmc=0.0;

  tad = Calculated->TaskAltitudeDifference/(Calculated->TaskDistanceToGo+1);
  
  dmc = dmc*0.2+0.8*0.5*min(1.0,max(-1.0,tad/0.001));

  MACCREADY += dmc;
  MACCREADY = min(5.0,max(0,MACCREADY));

  /* NOT WORKING
  static double tad=0.0;
  static double mclast = 0.0;
  static double tadlast= 0.0;
  static double slope = 0.0;
  double mcnew;
  double delta;

  tad = Calculated->TaskAltitudeDifference;

  if (fabs(tad)<5.0) {
    tadlast = tad;
    mclast = MACCREADY;
    return;
  }

  // no change detected, increment until see something

  if (fabs(tad-tadlast)>0.0001) {
    slope = 0.9*slope+0.1*(MACCREADY-mclast)/(tad-tadlast);
  } else {
  }
 
  if (fabs(slope)<0.01) {
    if (tad>0) {
      mcnew= MACCREADY+0.1;
    } else {
      mcnew= MACCREADY-0.1;
    }
  } else {

    // y = mx + c
    // -c = mx
    // x = -c/m
    // 5 -> 100
    // 4 -> 200
    // slope=(5-4)/(100-200)= -0.1
    delta = (-slope*tad);
    delta = min(1.0,max(-1.0,delta));
    mcnew = MACCREADY+0.3*(delta);
  }
  tadlast = tad;
  mclast = MACCREADY;

  MACCREADY = mcnew;
  if (MACCREADY>10.0) {
    MACCREADY = 10.0;
  }
  if (MACCREADY<0.0) {
    MACCREADY = 0.0;
  }
  */
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
      Calculated->NextAltitude = Basic->Altitude + Calculated->Average30s * 30;
    }
  else
    {
      Calculated->NextLatitude = FindLatitude(Basic->Latitude, Basic->Longitude, Basic->TrackBearing, Basic->Speed*WarningTime );
      Calculated->NextLongitude = FindLongitude(Basic->Latitude, Basic->Longitude, Basic->TrackBearing, Basic->Speed*WarningTime);
      Calculated->NextAltitude = Basic->Altitude + Calculated->Average30s * WarningTime;
    }
}


int AirspaceLastCircle =-1;
int AirspaceLastArea =-1;

bool ClearAirspaceWarnings(bool ack, bool allday) {
  if (ack) {
    if (AirspaceLastCircle!= -1) {
      AirspaceCircle[AirspaceLastCircle].Ack.AcknowledgementTime = GPS_INFO.Time;
      if (allday) {
		AirspaceCircle[AirspaceLastCircle].Ack.AcknowledgedToday = true;
      }
      AirspaceLastCircle = -1;
    }
    if (AirspaceLastArea!= -1) {
      AirspaceArea[AirspaceLastArea].Ack.AcknowledgementTime = GPS_INFO.Time;
      if (allday) {
		AirspaceArea[AirspaceLastArea].Ack.AcknowledgedToday = true;
      }
      AirspaceLastArea = -1;
    }
    return Message::Acknowledge(MSG_AIRSPACE);
  }
  return false;
}


void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  int i;
  TCHAR szMessageBuffer[1024];
  TCHAR szTitleBuffer[1024];
  TCHAR text[1024];

  if(!AIRSPACEWARNINGS)
      return;

  // JMW TODO: FindAirspaceCircle etc should sort results, return 
  // the most critical or closest

  i= FindAirspaceCircle(Calculated->NextLongitude, 
			Calculated->NextLatitude, false);
  if(i != -1)
    {

      if(i == AirspaceLastCircle)
        {   // already being displayed
          return;
        }

      if (AirspaceCircle[i].Ack.AcknowledgedToday) {
	return;
      }

      if ((AirspaceCircle[i].Ack.AcknowledgementTime!=0) &&
	  ((GPS_INFO.Time-AirspaceCircle[i].Ack.AcknowledgementTime)<
	   AcknowledgementTime)) {
	return;
      }

      MessageBeep(MB_ICONEXCLAMATION);
      FormatWarningString(AirspaceCircle[i].Type , AirspaceCircle[i].Name , 
			  AirspaceCircle[i].Base, AirspaceCircle[i].Top, 
			  szMessageBuffer, szTitleBuffer );

      wsprintf(text,TEXT("AIRSPACE: %s\r\n%s"),
	       szTitleBuffer,szMessageBuffer);

      // clear previous warning if any
      Message::Acknowledge(MSG_AIRSPACE);
      Message::AddMessage(5000, MSG_AIRSPACE, text);

      if (AirspaceLastCircle != i) {
	InputEvents::processGlideComputer(GCE_AIRSPACE_ENTER);
      }

      AirspaceLastCircle = i;
      return;
    }
  else
    {
      if (AirspaceLastCircle != -1) {
	// left area, so re-set acknowledgement
	AirspaceCircle[AirspaceLastCircle].Ack.AcknowledgementTime = 
	  GPS_INFO.Time-AcknowledgementTime;

	Message::Acknowledge(MSG_AIRSPACE);
	InputEvents::processGlideComputer(GCE_AIRSPACE_LEAVE);
      }
      AirspaceLastCircle = -1;
    }
        

  i= FindAirspaceArea(Calculated->NextLongitude,
		      Calculated->NextLatitude, false);
  if(i != -1)
    {
      if(i == AirspaceLastArea)
        {
          return;
        }

      if (AirspaceArea[i].Ack.AcknowledgedToday) {
	return;
      }

      if ((AirspaceArea[i].Ack.AcknowledgementTime!=0) &&
	  ((GPS_INFO.Time-AirspaceArea[i].Ack.AcknowledgementTime)<
	   AcknowledgementTime)) {
	return;
      }

      MessageBeep(MB_ICONEXCLAMATION);
      FormatWarningString(AirspaceArea[i].Type , AirspaceArea[i].Name , 
			  AirspaceArea[i].Base, AirspaceArea[i].Top, 
			  szMessageBuffer, szTitleBuffer );

      wsprintf(text,TEXT("AIRSPACE: %s\r\n%s"),
	       szTitleBuffer,
	       szMessageBuffer);
      Message::Acknowledge(MSG_AIRSPACE);
      Message::AddMessage(10000, MSG_AIRSPACE, text);

      if (AirspaceLastArea != i) {
	InputEvents::processGlideComputer(GCE_AIRSPACE_ENTER);
      }
                
      AirspaceLastArea = i;
      return;
    }
  else
    {
      if (AirspaceLastArea != -1) {
	AirspaceArea[AirspaceLastArea].Ack.AcknowledgementTime = 
	  GPS_INFO.Time-AcknowledgementTime;

	Message::Acknowledge(MSG_AIRSPACE);
	InputEvents::processGlideComputer(GCE_AIRSPACE_LEAVE);
      }
      AirspaceLastArea = -1;
    }
}


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
  double dheight = Basic->Altitude-SAFETYALTITUDEBREAKOFF;

  int index, i, j;

  if (dheight<0) {
    return; // nothing to do.
  }
  if (Calculated->MaxThermalHeight==0) {
    Calculated->MaxThermalHeight = dheight;
  }

  // only do this if in thermal and have been climbing
  if ((!Calculated->Circling)||(Calculated->Average30s<0)) return;

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
  
    mthnew = Calculated->MaxThermalHeight;
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
      h = (i)*(Calculated->MaxThermalHeight)/(NUMTHERMALBUCKETS); // height of center of bucket
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

  index = iround(NUMTHERMALBUCKETS*(dheight/Calculated->MaxThermalHeight));
  if (index==NUMTHERMALBUCKETS) {
    index= NUMTHERMALBUCKETS-1;
  }

  Calculated->ThermalProfileW[index]+= Calculated->Vario;
  Calculated->ThermalProfileN[index]++;

}



//////////////////////////////////////////////////////////
// Final glide through terrain and footprint calculations

void ExitFinalGlideThroughTerrain() {
  UnlockTerrainDataCalculations();
}


double FinalGlideThroughTerrain(double bearing, NMEA_INFO *Basic, 
                                DERIVED_INFO *Calculated,
                                double *retlat, double *retlon) 
{

  // returns distance one would arrive at altitude in straight glide

  // first estimate max range at this altitude
  double ialtitude = GlidePolar::MacCreadyAltitude(MACCREADY, 
                                                  1.0, bearing, 
                                                  Calculated->WindSpeed, 
                                                  Calculated->WindBearing, 
                                                  0, 0, true, 0);
  double maxrange = Basic->Altitude/ialtitude;
  double lat, lon;
  double latlast, lonlast;
  double h=0.0, dh=0.0;
  int imax=0;
  double dhlast=0;
  double distance, altitude;
  double distancelast=0;
 
  if (retlat && retlon) {
    *retlat = Basic->Latitude;
    *retlon = Basic->Longitude;
  }
  
  // calculate terrain rounding factor
  LockTerrainDataCalculations();

  terrain_dem_calculations.
    SetTerrainRounding(maxrange/NUMFINALGLIDETERRAIN/1000.0);

  for (int i=0; i<=NUMFINALGLIDETERRAIN; i++) {
    distance = i*maxrange/NUMFINALGLIDETERRAIN;
    altitude = (NUMFINALGLIDETERRAIN-i)*(Basic->Altitude)/NUMFINALGLIDETERRAIN;

    // find lat, lon of point of interest
      
    lat = FindLatitude(Basic->Latitude, Basic->Longitude, bearing, distance);
    lon = FindLongitude(Basic->Latitude, Basic->Longitude, bearing, distance);

    // find height over terrain
    h =  terrain_dem_calculations.
      GetTerrainHeight(lat, lon); // latitude, longitude      

    dh = altitude - h -  SAFETYALTITUDETERRAIN;
    //SAFETYALTITUDEARRIVAL;

    if ((dh<=0)&&(dhlast>0)) {
      double f = (0.0-dhlast)/(dh-dhlast);
      if (retlat && retlon) {
        *retlat = latlast*(1.0-f)+lat*f;
        *retlon = lonlast*(1.0-f)+lon*f;
      }
      ExitFinalGlideThroughTerrain();
     return distancelast*(1.0-f)+distance*(f);
    }
    if (i&&(distance<= 0.0)) {
      ExitFinalGlideThroughTerrain();
      return 0.0;
    }

    distancelast = distance;
    dhlast = dh;
    latlast = lat;
    lonlast = lon;
  }

  ExitFinalGlideThroughTerrain();
  return 0.0;
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
  
  AltReqd = GlidePolar::MacCreadyAltitude(0.0, 
                                         wDistance, 
                                         wBearing, 
                                         Calculated->WindSpeed, 
                                         Calculated->WindBearing, 
                                         0, 
                                         0,
                                         true,
                                         0);
  
  return ((Basic->Altitude) - AltReqd - WayPointList[i].Altitude - SAFETYALTITUDEARRIVAL);
}



void SortLandableWaypoints (NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  int SortedLandableIndex[MAXTASKPOINTS];
  double SortedArrivalAltitude[MAXTASKPOINTS];
  int SortedApproxDistance[MAXTASKPOINTS*2];
  int SortedApproxIndex[MAXTASKPOINTS*2];
  int i, k, l;
  double aa;
  int ai;

  if (!WayPointList) return;

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
	  if (((ai < SortedApproxDistance[k]) ||	// closer than this one
	       (SortedApproxIndex[k]== -1))	&&		// or this one isn't filled
	      (SortedApproxIndex[k]!= i))					// and not replacing with same
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
	  if (SortedApproxIndex[i]<0)			// ignore invalid points
	    {
	      continue;
	    }
			
	  if (((WayPointList[SortedApproxIndex[i]].Flags & LANDPOINT) == LANDPOINT) &&
	      (scanairportsfirst==0))
	    {
	      // we are in the first scan, looking for airports only
	      continue;
	    }
			
	  aa = CalculateWaypointArrivalAltitude(Basic, Calculated, SortedApproxIndex[i]);

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
	      if (((aa > SortedArrivalAltitude[k]) ||	// closer than this one
		   (SortedLandableIndex[k]== -1))	&&	// or this one isn't filled
		  (SortedLandableIndex[k]!= i))				// and not replacing with same
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
					     NULL);
		  
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
	  DoStatusMessage(TEXT("Closest Airfield Changed!"));
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
}


void ResumeAbortTask(int set) {
  static int OldTask[MAXTASKPOINTS];
  static int OldActiveWayPoint= -1;
  static bool OldAATEnabled= false;
  int i;

  bool oldTaskAborted = TaskAborted;

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

}


#define TAKEOFFSPEEDTHRESHOLD (0.5*GlidePolar::Vminsink)

void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static int ntimeinflight = 0;

  if (Basic->Speed> TAKEOFFSPEEDTHRESHOLD) {
    ntimeinflight++;
  } else {
    if ((Calculated->AltitudeAGL<300)&&(Calculated->TerrainValid)) {
      ntimeinflight--;
    }
  }  
  ntimeinflight = min(30, max(0,ntimeinflight));

  // JMW logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds

  //   detect landing when below threshold speed for 30 seconds

  // TODO: make this more robust my making use of terrain height data if available

  if (!Calculated->Flying) {
    // detect takeoff

    if (ntimeinflight>10) {
      Calculated->Flying = TRUE;
      InputEvents::processGlideComputer(GCE_TAKEOFF);
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

