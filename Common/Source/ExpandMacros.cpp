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

#include "ButtonLabel.h"
#include "XCSoar.h"
#include "Language.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Logger.h"
#include "ReplayLogger.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Compatibility/string.h"
#include "InfoBoxManager.h"
#include "SettingsUser.hpp"
#include <stdlib.h>

static void ReplaceInString(TCHAR *String, const TCHAR *ToReplace,
                            const TCHAR *ReplaceWith, size_t Size){
  TCHAR TmpBuf[MAX_PATH];
  int   iR, iW;
  TCHAR *pC;

  while((pC = _tcsstr(String, ToReplace)) != NULL){
    iR = _tcsclen(ToReplace);
    iW = _tcsclen(ReplaceWith);
    _tcscpy(TmpBuf, pC + iR);
    _tcscpy(pC, ReplaceWith);
    _tcscat(pC, TmpBuf);
  }

}

static void CondReplaceInString(bool Condition, TCHAR *Buffer,
                                const TCHAR *Macro, const TCHAR *TrueText,
                                const TCHAR *FalseText, size_t Size){
  if (Condition)
    ReplaceInString(Buffer, Macro, TrueText, Size);
  else
    ReplaceInString(Buffer, Macro, FalseText, Size);
}

bool ButtonLabel::ExpandMacros(const TCHAR *In, 
			       TCHAR *OutBuffer, size_t Size) {
  // ToDo, check Buffer Size
  bool invalid = false;
  _tcsncpy(OutBuffer, In, Size);
  OutBuffer[Size-1] = '\0';

  if (_tcsstr(OutBuffer, TEXT("$(")) == NULL) return false;

  if (TaskAborted) {
    if (_tcsstr(OutBuffer, TEXT("$(WaypointNext)"))) {
      // Waypoint\nNext
      invalid = !ValidTaskPoint(ActiveWayPoint+1);
      CondReplaceInString(!ValidTaskPoint(ActiveWayPoint+2),
                          OutBuffer,
                          TEXT("$(WaypointNext)"),
                          TEXT("Landpoint\nFurthest"),
                          TEXT("Landpoint\nNext"), Size);

    } else
    if (_tcsstr(OutBuffer, TEXT("$(WaypointPrevious)"))) {
      // Waypoint\nNext
      invalid = !ValidTaskPoint(ActiveWayPoint-1);
      CondReplaceInString(!ValidTaskPoint(ActiveWayPoint-2),
                          OutBuffer,
                          TEXT("$(WaypointPrevious)"),
                          TEXT("Landpoint\nClosest"),
                          TEXT("Landpoint\nPrevious"), Size);
    }
  } else {
    if (_tcsstr(OutBuffer, TEXT("$(WaypointNext)"))) {
      // Waypoint\nNext
      invalid = !ValidTaskPoint(ActiveWayPoint+1);
      CondReplaceInString(!ValidTaskPoint(ActiveWayPoint+2),
                          OutBuffer,
                          TEXT("$(WaypointNext)"),
                          TEXT("Waypoint\nFinish"),
                          TEXT("Waypoint\nNext"), Size);

    } else
    if (_tcsstr(OutBuffer, TEXT("$(WaypointPrevious)"))) {
      if (ActiveWayPoint==1) {
        invalid = !ValidTaskPoint(ActiveWayPoint-1);
        ReplaceInString(OutBuffer, TEXT("$(WaypointPrevious)"),
                        TEXT("Waypoint\nStart"), Size);
      } else if (EnableMultipleStartPoints) {
        invalid = !ValidTaskPoint(0);
        CondReplaceInString((ActiveWayPoint==0),
                            OutBuffer,
                            TEXT("$(WaypointPrevious)"),
                            TEXT("StartPoint\nCycle"), TEXT("Waypoint\nPrevious"), Size);
      } else {
        invalid = (ActiveWayPoint<=0);
        ReplaceInString(OutBuffer, TEXT("$(WaypointPrevious)"), TEXT("Waypoint\nPrevious"), Size);
      }
    }
  }

  if (_tcsstr(OutBuffer, TEXT("$(AdvanceArmed)"))) {
    switch (AutoAdvance) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"), TEXT("(manual)"), Size);
      invalid = true;
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"), TEXT("(auto)"), Size);
      invalid = true;
      break;
    case 2:
      if (ActiveWayPoint>0) {
        if (ValidTaskPoint(ActiveWayPoint+1)) {
          CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                              TEXT("Cancel"), TEXT("TURN"), Size);
        } else {
          ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"),
                          TEXT("(finish)"), Size);
          invalid = true;
        }
      } else {
        CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                            TEXT("Cancel"), TEXT("START"), Size);
      }
      break;
    case 3:
      if (ActiveWayPoint==0) {
        CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                            TEXT("Cancel"), TEXT("START"), Size);
      } else if (ActiveWayPoint==1) {
        CondReplaceInString(AdvanceArmed, OutBuffer, TEXT("$(AdvanceArmed)"),
                            TEXT("Cancel"), TEXT("RESTART"), Size);
      } else {
        ReplaceInString(OutBuffer, TEXT("$(AdvanceArmed)"), TEXT("(auto)"), Size);
        invalid = true;
      }
      // TODO bug: no need to arm finish
    default:
      break;
    }
  }

  if (_tcsstr(OutBuffer, TEXT("$(CheckReplay)"))) {
    if (!Basic().Replay && Basic().MovementDetected) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckReplay)"), TEXT(""), Size);
  }

  if (_tcsstr(OutBuffer, TEXT("$(CheckWaypointFile)"))) {
    if (!ValidWayPoint(0)) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckWaypointFile)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckSettingsLockout)"))) {
#ifndef _SIM_
    if (XCSoarInterface::LockSettingsInFlight && Calculated().Flying) {
      invalid = true;
    }
#endif
    ReplaceInString(OutBuffer, TEXT("$(CheckSettingsLockout)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckTaskResumed)"))) {
    if (TaskAborted) {
      // TODO code: check, does this need to be set with temporary task?
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckTaskResumed)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckTask)"))) {
    if (!ValidTaskPoint(ActiveWayPoint)) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckTask)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckAirspace)"))) {
    if (!ValidAirspace()) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckAirspace)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckFLARM)"))) {
    if (!Basic().FLARM_Available) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckFLARM)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckTerrain)"))) {
    if (!Calculated().TerrainValid) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckTerrain)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckAutoMc)"))) {
    if (!ValidTaskPoint(ActiveWayPoint)
        && ((SettingsComputer().AutoMcMode==0)
	    || (SettingsComputer().AutoMcMode==2))) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckAutoMc)"), TEXT(""), Size);
  }

  CondReplaceInString(LoggerActive, OutBuffer, TEXT("$(LoggerActive)"), TEXT("Stop"), TEXT("Start"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(SnailTrailToggleName)"))) {
    switch(SettingsMap().TrailActive) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Long"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Short"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Full"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, TEXT("$(SnailTrailToggleName)"), TEXT("Off"), Size);
      break;
    }
  }
// VENTA3 VisualGlide
  if (_tcsstr(OutBuffer, TEXT("$(VisualGlideToggleName)"))) {
    switch(SettingsMap().VisualGlide) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Steady"), Size);
      break;
    case 1:
	if (SettingsMap().ExtendedVisualGlide)
		ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Moving"), Size);
	else
		ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Off"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Off"), Size);
      break;
    }
  }

// VENTA3 AirSpace event
  if (_tcsstr(OutBuffer, TEXT("$(AirSpaceToggleName)"))) {
    switch(SettingsMap().OnAirSpace) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(AirSpaceToggleName)"), TEXT("ON"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(AirSpaceToggleName)"), TEXT("OFF"), Size);
      break;
    }
  }

  if (_tcsstr(OutBuffer, TEXT("$(TerrainTopologyToggleName)"))) {
    char val;
    val = 0;
    if (SettingsMap().EnableTopology) val++;
    if (SettingsMap().EnableTerrain) val += (char)2;
    switch(val) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      TEXT("Topology\nOn"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      TEXT("Terrain\nOn"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      TEXT("Terrain\nTopology"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, TEXT("$(TerrainTopologyToggleName)"),
                      gettext(TEXT("Terrain\nOff")), Size);
      break;
    }
  }

  //////

  CondReplaceInString(TaskIsTemporary(),
		      OutBuffer, TEXT("$(TaskAbortToggleActionName)"),
		      TEXT("Resume"), TEXT("Abort"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(FinalForceToggleActionName)"))) {
    CondReplaceInString(ForceFinalGlide, OutBuffer,
                        TEXT("$(FinalForceToggleActionName)"),
                        TEXT("Unforce"),
                        TEXT("Force"), Size);
    if (SettingsComputer().AutoForceFinalGlide) {
      invalid = true;
    }
  }

  CondReplaceInString(SettingsMap().FullScreen, OutBuffer,
                      TEXT("$(FullScreenToggleActionName)"),
                      TEXT("Off"), TEXT("On"), Size);
  CondReplaceInString(SettingsMap().AutoZoom, OutBuffer, 
		      TEXT("$(ZoomAutoToggleActionName)"), 
		      TEXT("Manual"), TEXT("Auto"), Size);
  CondReplaceInString(SettingsMap().EnableTopology, OutBuffer, TEXT("$(TopologyToggleActionName)"), TEXT("Off"), TEXT("On"), Size);
  CondReplaceInString(SettingsMap().EnableTerrain, OutBuffer, TEXT("$(TerrainToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(MapLabelsToggleActionName)"))) {
    switch(SettingsMap().DeclutterLabels) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(MapLabelsToggleActionName)"),
                      TEXT("MID"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, TEXT("$(MapLabelsToggleActionName)"),
                      TEXT("OFF"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, TEXT("$(MapLabelsToggleActionName)"),
                      TEXT("ON"), Size);
      break;
    }
  }

  CondReplaceInString(SettingsComputer().AutoMacCready != 0, OutBuffer, TEXT("$(MacCreadyToggleActionName)"), TEXT("Manual"), TEXT("Auto"), Size);
  CondReplaceInString(SettingsMap().EnableAuxiliaryInfo, OutBuffer, TEXT("$(AuxInfoToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmCircling, OutBuffer, TEXT("$(DispModeClimbShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmCruise, OutBuffer, TEXT("$(DispModeCruiseShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmNone, OutBuffer, TEXT("$(DispModeAutoShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmFinalGlide, OutBuffer, TEXT("$(DispModeFinalShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(AltitudeMode == ALLON, OutBuffer, TEXT("$(AirspaceModeAllShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == CLIP,  OutBuffer, TEXT("$(AirspaceModeClipShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == AUTO,  OutBuffer, TEXT("$(AirspaceModeAutoShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == ALLBELOW, OutBuffer, TEXT("$(AirspaceModeBelowShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == ALLOFF, OutBuffer, TEXT("$(AirspaceModeAllOffIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(SettingsMap().TrailActive == 0, OutBuffer, TEXT("$(SnailTrailOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().TrailActive == 2, OutBuffer, TEXT("$(SnailTrailShortShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().TrailActive == 1, OutBuffer, TEXT("$(SnailTrailLongShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().TrailActive == 3, OutBuffer, TEXT("$(SnailTrailFullShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

// VENTA3 VisualGlide
  CondReplaceInString(SettingsMap().VisualGlide == 0, OutBuffer, TEXT("$(VisualGlideOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().VisualGlide == 1, OutBuffer, TEXT("$(VisualGlideLightShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().VisualGlide == 2, OutBuffer, TEXT("$(VisualGlideHeavyShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
// VENTA3 AirSpace
  CondReplaceInString(SettingsMap().OnAirSpace  == 0, OutBuffer, TEXT("$(AirSpaceOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(SettingsMap().OnAirSpace  == 1, OutBuffer, TEXT("$(AirSpaceOnShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(SettingsMap().EnableFLARMGauge != 0, OutBuffer, TEXT("$(FlarmDispToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  return invalid;
}
