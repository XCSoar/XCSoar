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
#include <windows.h>
#include <math.h>
#include "InputEvents.h"
#include "Message.h"
#include "RasterTerrain.h"
#include "TeamCodeCalculation.h"
#include "Process.h"

#include <tchar.h>

#include "ThermalLocator.h"
#include "windanalyser.h"
#include "Atmosphere.h"

#include "VegaVoice.h"

#include "OnLineContest.h"
#include "AATDistance.h"

#include "NavFunctions.h" // used for team code

extern OLCOptimizer olc;

int FastLogNum = 0; // number of points to log at high rate

void AddSnailPoint(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (!Calculated->Flying) return;

  SnailTrail[SnailNext].Latitude = (float)(Basic->Latitude);
  SnailTrail[SnailNext].Longitude = (float)(Basic->Longitude);
  SnailTrail[SnailNext].Time = Basic->Time;
  SnailTrail[SnailNext].FarVisible = true; // hasn't been filtered out yet.

  if (Basic->NettoVarioAvailable && !(ReplayLogger::IsEnabled())) {
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
  static double OLCLastTime = 0;
  double dtLog = 5.0;
  double dtSnail = 2.0;
  double dtStats = 60.0;
  double dtOLC = 5.0;

  if(Basic->Time <= LogLastTime) {
    LogLastTime = Basic->Time;
  }
  if(Basic->Time <= SnailLastTime)  {
    SnailLastTime = Basic->Time;
  }
  if(Basic->Time <= StatsLastTime) {
    StatsLastTime = Basic->Time;
  }
  if(Basic->Time <= OLCLastTime) {
    OLCLastTime = Basic->Time;
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
      double balt = -1;
      if (Basic->BaroAltitudeAvailable) {
        balt = Basic->BaroAltitude;
      } else {
        balt = Basic->Altitude;
      }
      LogPoint(Basic->Latitude , Basic->Longitude , Basic->Altitude,
               balt);
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

  if (Calculated->Flying) {
    if (Basic->Time - StatsLastTime >= dtStats) {
      flightstats.Altitude.
        least_squares_update(Basic->Time/3600.0, 
                             Calculated->NavAltitude);
      StatsLastTime += dtStats;
      if (StatsLastTime< Basic->Time-dtStats) {
        StatsLastTime = Basic->Time-dtStats;
      }
    }

    if (Basic->Time - OLCLastTime >= dtOLC) {
      bool restart;
      restart = olc.addPoint(Basic->Longitude, 
			     Basic->Latitude, 
			     Calculated->NavAltitude,
			     Calculated->WaypointBearing,
			     Basic->Time);
      
      if (restart && EnableOLC) {
	Calculated->ValidFinish = false;
	StartTask(Basic, Calculated, false, false);
	Calculated->ValidStart = true;
      }
      OLCLastTime += dtOLC;
    }
  }
}


//////////////////////////////////////////////////////////
// Final glide through terrain and footprint calculations

static void ExitFinalGlideThroughTerrain() {
  UnlockTerrainDataCalculations();
}


double FinalGlideThroughTerrain(double bearing, NMEA_INFO *Basic, 
                                DERIVED_INFO *Calculated,
                                double *retlat, double *retlon,
                                double maxrange,
				bool *outofrange) 
{
  double irange = GlidePolar::MacCreadyAltitude(MACCREADY, 
						// should this be zero?
						1.0, bearing, 
						Calculated->WindSpeed, 
						Calculated->WindBearing, 
						0, 0, true, 0);
  if (retlat && retlon) {
    *retlat = Basic->Latitude;
    *retlon = Basic->Longitude;
  }
  *outofrange = false;

  if ((irange<=0.0)||(Calculated->NavAltitude<=0)) {
    // can't make progress in this direction at the current windspeed/mc
    return 0;
  }

  double glidemaxrange = Calculated->NavAltitude/irange;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude
  double lat, lon;
  double latlast, lonlast;
  double h=0.0, dh=0.0;
//  int imax=0;
  double dhlast=0;
  double altitude;
 
  LockTerrainDataCalculations();

  // calculate terrain rounding factor


  FindLatitudeLongitude(Basic->Latitude, Basic->Longitude, 0, 
                        glidemaxrange/NUMFINALGLIDETERRAIN, &lat, &lon);

  double Xrounding = fabs(lon-Basic->Longitude)/2;
  double Yrounding = fabs(lat-Basic->Latitude)/2;
  terrain_dem_calculations.SetTerrainRounding(Xrounding, Yrounding);

  altitude = Calculated->NavAltitude;
  h =  terrain_dem_calculations.GetTerrainHeight(lat, lon); 
  dh = altitude - h - SAFETYALTITUDETERRAIN;
  if (dh<0) {
    ExitFinalGlideThroughTerrain();
    return 0;
  }

  latlast = Basic->Latitude;
  lonlast = Basic->Longitude;

  // find grid
  double dlat, dlon;
  FindLatitudeLongitude(Basic->Latitude, Basic->Longitude, bearing, 
                        glidemaxrange, &dlat, &dlon);
  dlat -= Basic->Latitude;
  dlon -= Basic->Longitude;

  for (int i=0; i<=NUMFINALGLIDETERRAIN; i++) {
    double fi = (i*1.0)/NUMFINALGLIDETERRAIN;
    // fraction of glidemaxrange

    if ((maxrange>0)&&(fi>maxrange/glidemaxrange)) {
      // early exit
      *outofrange = true;
      ExitFinalGlideThroughTerrain();
      return maxrange;
    }

    altitude = (1.0-fi)*Calculated->NavAltitude;

    // find lat, lon of point of interest

    lat = Basic->Latitude+dlat*fi;
    lon = Basic->Longitude+dlon*fi;

    // find height over terrain
    h =  terrain_dem_calculations.GetTerrainHeight(lat, lon); 

    dh = altitude - h - SAFETYALTITUDETERRAIN;

    if ((dh<=0)&&(dhlast>=0)) {
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
      double distance;
      DistanceBearing(Basic->Latitude, Basic->Longitude, lat, lon,
                      &distance, NULL);
      return distance;
    }
    dhlast = dh;
    latlast = lat;
    lonlast = lon;
  }

  *outofrange = true;
  ExitFinalGlideThroughTerrain();
  return glidemaxrange;
}


void CloseTerrain(void) {
  LockTerrainDataCalculations();
  LockTerrainDataGraphics();
  RasterTerrain::CloseTerrain();
  UnlockTerrainDataCalculations();
  UnlockTerrainDataGraphics();
}


void OpenTerrain(void) {
  LockTerrainDataCalculations();
  LockTerrainDataGraphics();
  RasterTerrain::OpenTerrain();
  terrain_dem_graphics.ClearTerrainCache();
  terrain_dem_calculations.ClearTerrainCache();
  UnlockTerrainDataCalculations();
  UnlockTerrainDataGraphics();
}


double PirkerAnalysis(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      double bearing,
                      double GlideSlope) {


//  bool maxfound = false;
//  bool first = true;
  double pmc = 0.0;
  double htarget = GlideSlope;
  double h;
  double dh= 1.0;
  double pmclast = 5.0;
  double dhlast = -1.0;
  double pmczero = 0.0;
 
  (void)Basic;

  while (pmc<10.0) {

    h = GlidePolar::MacCreadyAltitude(pmc, 
                                      1.0, // unit distance
				      bearing, 
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


double MacCreadyTimeLimit(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			  double bearing,
			  double timeremaining,
			  double hfinal) {

  // find highest Mc to achieve greatest distance in remaining time and height
  (void)Basic;

  double timetogo;
  double mc;
  double mcbest = 0.0;
  double dbest = 0.0;

  for (mc=0; mc<10.0; mc+= 0.1) {

    double hunit = GlidePolar::MacCreadyAltitude(mc, 
						 1.0, // unit distance
						 bearing, 
						 Calculated->WindSpeed, 
						 Calculated->WindBearing,
						 NULL,
						 NULL,
						 1, // final glide
						 &timetogo);
    if (timetogo>0) {
      double p = timeremaining/timetogo;    
      double hspent = hunit*p;    
      double dh = Calculated->NavAltitude-hspent-hfinal;    
      double d = 1.0*p;
      
      if ((d>dbest) && (dh>=0)) {
	mcbest = mc;
      }
    }
  }
  return mcbest;
}


/////////

DWORD lastTeamCodeUpdateTime = GetTickCount();

void CalculateOwnTeamCode(NMEA_INFO *Basic, DERIVED_INFO *Calculated) 
{
  if (!WayPointList) return;
  if (TeamCodeRefWaypoint < 0) return;
  //if (lastTeamCodeUpdateTime + 10000 > GetTickCount()) return;

	
  double distance = 0;
  double bearing = 0;
  TCHAR code[10];
	
  lastTeamCodeUpdateTime = GetTickCount();

  /*
  distance =  Distance(WayPointList[TeamCodeRefWaypoint].Latitude, 
  	WayPointList[TeamCodeRefWaypoint].Longitude,
  	Basic->Latitude, 
  	Basic->Longitude);
  	
  bearing = Bearing(WayPointList[TeamCodeRefWaypoint].Latitude, 
  	WayPointList[TeamCodeRefWaypoint].Longitude,
  	Basic->Latitude, 
  	Basic->Longitude);
  */
	
  // TODO: ask Lars why this one
  LL_to_BearRange(WayPointList[TeamCodeRefWaypoint].Latitude, 
                  WayPointList[TeamCodeRefWaypoint].Longitude,
                  Basic->Latitude, 
                  Basic->Longitude,
                  &bearing, &distance);

  GetTeamCode(code, bearing, distance);

  Calculated->TeammateBearing = bearing;
  Calculated->TeammateRange = distance;	

  //// calculate lat/long for team-mate position
  //double teammateBearing = GetTeammateBearingFromRef(TeammateCode);
  //double teammateRange = GetTeammateRangeFromRef(TeammateCode);

  //Calculated->TeammateLongitude = FindLongitude(

  wcsncpy(Calculated->OwnTeamCode, code, 5);
}


void CalculateTeammateBearingRange(NMEA_INFO *Basic, DERIVED_INFO *Calculated) 
{
  static bool InTeamSector = false;

  if (!WayPointList) return;
  if (TeamCodeRefWaypoint < 0) return;

  double ownDistance = 0;
  double ownBearing = 0;
  double mateDistance = 0;
  double mateBearing = 0;
	
  //ownBearing = Bearing(Basic->Latitude, Basic->Longitude,
  //	WayPointList[TeamCodeRefWaypoint].Latitude, 
  //	WayPointList[TeamCodeRefWaypoint].Longitude);	
  //
  //ownDistance =  Distance(Basic->Latitude, Basic->Longitude,
  //	WayPointList[TeamCodeRefWaypoint].Latitude, 
  //	WayPointList[TeamCodeRefWaypoint].Longitude);
		
  /*
  ownBearing = Bearing(WayPointList[TeamCodeRefWaypoint].Latitude, 
                       WayPointList[TeamCodeRefWaypoint].Longitude,
                       Basic->Latitude, 
                       Basic->Longitude
                       );	
  //
  ownDistance =  Distance(WayPointList[TeamCodeRefWaypoint].Latitude, 
                          WayPointList[TeamCodeRefWaypoint].Longitude,
                          Basic->Latitude, 
                          Basic->Longitude
                          );	
  */

  // TODO: ask Lars why this one

  LL_to_BearRange(WayPointList[TeamCodeRefWaypoint].Latitude, 
                  WayPointList[TeamCodeRefWaypoint].Longitude,
                  Basic->Latitude, 
                  Basic->Longitude,
                  &ownBearing, &ownDistance);

  if (TeammateCodeValid)
    {
	
      //mateBearing = Bearing(Basic->Latitude, Basic->Longitude, TeammateLatitude, TeammateLongitude);
      //mateDistance = Distance(Basic->Latitude, Basic->Longitude, TeammateLatitude, TeammateLongitude);

      CalcTeammateBearingRange(ownBearing, ownDistance, 
                               TeammateCode, 
                               &mateBearing, &mateDistance);

      // TODO ....change the result of CalcTeammateBearingRange to do this !
      if (mateBearing > 180)
        {
          mateBearing -= 180;
        }
      else
        {
          mateBearing += 180;
        }


      Calculated->TeammateBearing = mateBearing;
      Calculated->TeammateRange = mateDistance;

      FindLatitudeLongitude(Basic->Latitude, 
                            Basic->Longitude,
                            mateBearing,
                            mateDistance, 
                            &TeammateLatitude,
                            &TeammateLongitude);

      if (mateDistance < 100 && InTeamSector==false)
        {
          InTeamSector=true;
          InputEvents::processGlideComputer(GCE_TEAM_POS_REACHED);
        }
      else if (mateDistance > 300)
        {
          InTeamSector = false;
        }
    }
  else
    {
      Calculated->TeammateBearing = 0;
      Calculated->TeammateRange = 0;
    }

}

//////////


static TCHAR szCalculationsPersistFileName[MAX_PATH]= TEXT("\0");

void DeleteCalculationsPersist(void) {
  DeleteFile(szCalculationsPersistFileName);
}

void LoadCalculationsPersist(DERIVED_INFO *Calculated) {
  if (szCalculationsPersistFileName[0]==0) {
#ifdef GNAV
    LocalPath(szCalculationsPersistFileName,
              TEXT("persist/xcsoar-persist.log"));    
#else
    LocalPath(szCalculationsPersistFileName,
              TEXT("xcsoar-persist.log"));    
#endif
  }

  StartupStore(TEXT("LoadCalculationsPersist\r\n"));

  HANDLE hFile;
  DWORD dwBytesWritten;
  DWORD size, sizein;
  hFile = CreateFile(szCalculationsPersistFileName,
                     GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile!=INVALID_HANDLE_VALUE ) {
    size = sizeof(DERIVED_INFO);
    ReadFile(hFile,&sizein,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    if (sizein != size) { CloseHandle(hFile); return; }
    ReadFile(hFile,Calculated,size,&dwBytesWritten,(OVERLAPPED*)NULL);

    size = sizeof(Statistics);
    ReadFile(hFile,&sizein,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    if (sizein != size) { CloseHandle(hFile); return; }
    ReadFile(hFile,&flightstats,size,&dwBytesWritten,(OVERLAPPED*)NULL);

    size = sizeof(OLCData);
    ReadFile(hFile,&sizein,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    if (sizein != size) { CloseHandle(hFile); return; }
    ReadFile(hFile,&olc.data,size,&dwBytesWritten,(OVERLAPPED*)NULL);   

    CloseHandle(hFile);
  }
}


void SaveCalculationsPersist(DERIVED_INFO *Calculated) {
  HANDLE hFile;
  DWORD dwBytesWritten;
  DWORD size;

  if (FindFreeSpace(szCalculationsPersistFileName)<MINFREESTORAGE) return;

  StartupStore(TEXT("SaveCalculationsPersist\r\n"));

  hFile = CreateFile(szCalculationsPersistFileName,
                     GENERIC_WRITE,0,(LPSECURITY_ATTRIBUTES)NULL,
                     CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile!=INVALID_HANDLE_VALUE ) {
    size = sizeof(DERIVED_INFO);
    WriteFile(hFile,&size,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    WriteFile(hFile,Calculated,size,&dwBytesWritten,(OVERLAPPED*)NULL);
    size = sizeof(Statistics);
    WriteFile(hFile,&size,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    WriteFile(hFile,&flightstats,size,&dwBytesWritten,(OVERLAPPED*)NULL);
    size = sizeof(OLCData);
    WriteFile(hFile,&size,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    WriteFile(hFile,&olc.data,size,&dwBytesWritten,(OVERLAPPED*)NULL);
    CloseHandle(hFile);
  }

}

