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

#ifndef EXTERNS_H
#define EXTERNS_H

#include "Task.h"
#include "NMEA/Info.h"
#include "Calculations.h"

#if (EXPERIMENTAL > 0)
//JMW#include "BlueSMS.h"
#endif

extern int UTCOffset;

#if defined(PNA) || defined(FIVV)  // VENTA2- ADD GlobalEllipse
extern int	GlobalModelType;
extern TCHAR	GlobalModelName[];
extern float	GlobalEllipse;
extern TCHAR *	gmfpathname();
extern TCHAR *	gmfbasename();
extern int		GetGlobalModelName();
extern void		SmartGlobalModelType();
extern short		InstallFonts();
extern bool		CheckDataDir();
extern bool		CheckRegistryProfile();
extern void		ConvToUpper( TCHAR *);

#endif

// waypoint data
extern int HomeWaypoint;
extern int AirfieldsHomeWaypoint; // VENTA3
extern int Alternate1; // VENTA3
extern int Alternate2;
extern int BestAlternate;
extern int ActiveAlternate;
extern bool  OnBestAlternate;
extern bool  OnAlternate1;
extern bool  OnAlternate2;

extern bool GlobalRunning;

extern short ScreenSize; // VENTA6

// task data
extern START_POINT StartPoints[];
extern TASK_POINT Task[];
extern TASKSTATS_POINT TaskStats[];
extern int ActiveWayPoint;
extern bool TaskAborted;
extern int SelectedWaypoint;
extern int SectorType;
extern DWORD SectorRadius;

extern int StartLine;
extern DWORD StartRadius;
extern int FinishLine;
extern DWORD FinishRadius;
extern double AATTaskLength;
extern BOOL AATEnabled;
extern bool EnableFAIFinishHeight;
extern DWORD FinishMinHeight;
extern DWORD StartMaxHeight;
extern DWORD StartMaxHeightMargin;
extern DWORD StartMaxSpeed;
extern DWORD StartMaxSpeedMargin;
extern int OLCRules;
extern int Handicap;
extern bool EnableOLC;

// master flight data
extern NMEA_INFO GPS_INFO;
extern DERIVED_INFO CALCULATED_INFO;

// gps detection
extern BOOL GPSCONNECT;
extern BOOL VARIOCONNECT;

// units
extern double SPEEDMODIFY;
extern double LIFTMODIFY;
extern double DISTANCEMODIFY;
extern double ALTITUDEMODIFY;
extern double TASKSPEEDMODIFY;

// polar info
extern double BUGS;
extern double BALLAST;
extern int POLARID;
extern double POLAR[POLARSIZE];
extern double WEIGHTS[POLARSIZE];
extern int BallastSecsToEmpty;
extern bool BallastTimerActive;

extern bool InfoWindowActive;

// Logger
extern bool LoggerActive;
extern int LoggerTimeStepCruise;
extern int LoggerTimeStepCircling;

// user interface options
extern bool CircleZoom;
extern bool EnableTopology;
extern bool EnableTerrain;
extern int FinalGlideTerrain;
extern int AutoWindMode;
extern bool EnableNavBaroAltitude;
extern bool EnableSoundVario;
extern bool EnableSoundTask;
extern bool EnableSoundModes;
extern int SoundVolume;
extern int SoundDeadband;
extern int DisplayOrientation;
extern int DisplayTextType;


extern bool EnableCDICruise;
extern bool EnableCDICircling;
extern bool EnableVarioGauge;
extern int AutoAdvance;
extern bool AdvanceArmed;
extern bool EnableBlockSTF; // block speed to fly instead of dolphin
extern int MenuTimeoutMax;
extern int EnableThermalLocator;
//

extern bool ExternalTriggerCruise;
extern bool ExternalTriggerCircling;
extern int EnableExternalTriggerCruise;

// used in settings dialog
extern BOOL COMPORTCHANGED;
extern BOOL AIRSPACEFILECHANGED;
extern BOOL WAYPOINTFILECHANGED;
extern BOOL TERRAINFILECHANGED;
extern BOOL AIRFIELDFILECHANGED;
extern BOOL TOPOLOGYFILECHANGED;
extern BOOL POLARFILECHANGED;
extern BOOL LANGUAGEFILECHANGED;
extern BOOL STATUSFILECHANGED;
extern BOOL INPUTFILECHANGED;
extern BOOL MAPFILECHANGED;

bool Debounce();

// Team code
extern int TeamCodeRefWaypoint;
extern TCHAR TeammateCode[10];
extern double TeammateLatitude;
extern double TeammateLongitude;
extern bool TeammateCodeValid;
extern bool TeamFlarmTracking;
extern TCHAR TeamFlarmCNTarget[4]; // CN of the glider to track
extern int TeamFlarmIdTarget;    // FlarmId of the glider to track

extern bool RequestAirspaceWarningDialog;

extern int UserLevel;

#endif
