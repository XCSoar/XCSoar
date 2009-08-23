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

#ifndef XCSOAR_SETTINGS_HPP
#define XCSOAR_SETTINGS_HPP

#include "Task.h"

// changed only in config or by user interface /////////////////////////////

// local time adjustment
extern int UTCOffset;

// user controls/parameters
extern double SAFETYALTITUDEARRIVAL;
extern double SAFETYALTITUDEBREAKOFF;
extern double SAFETYALTITUDETERRAIN;
extern double SAFTEYSPEED;

// user interface options
extern bool CircleZoom;
extern bool EnableTopology;
extern bool EnableTerrain;
extern int  FinalGlideTerrain;
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
extern int MenuTimeoutMax;
extern bool EnableAutoBacklight; // VENTA4
extern bool EnableAutoSoundVolume; // VENTA4
extern bool ExtendedVisualGlide;

// external control of user interface
extern int EnableExternalTriggerCruise;

// Logger
extern int LoggerTimeStepCruise;
extern int LoggerTimeStepCircling;

// control of calculations
extern int  AutoWindMode;
extern bool EnableNavBaroAltitude;
extern bool EnableBlockSTF; // block speed to fly instead of dolphin
extern int  EnableThermalLocator;

// control of task/waypoints
extern int  AutoAdvance;
extern bool AdvanceArmed;
extern int HomeWaypoint;
extern int AirfieldsHomeWaypoint; // VENTA3

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

// changed in task/flight or by calc thread /////////////////////////////////

extern bool LoggerActive;

// polar info
extern int BallastSecsToEmpty;
extern bool BallastTimerActive;

// waypoint data
extern int Alternate1; // VENTA3
extern int Alternate2;
extern int BestAlternate;
extern int ActiveAlternate;
extern bool  OnBestAlternate;
extern bool  OnAlternate1;
extern bool  OnAlternate2;

// external control of user interface
extern bool ExternalTriggerCruise;
extern bool ExternalTriggerCircling;

// unsorted ///////////////////////////////////////////////////////////////////



#endif
