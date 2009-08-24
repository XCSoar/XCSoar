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

#include "ExpandMacros.hpp"
#include "XCSoar.h"
#include "Dialogs.h"
#include "GaugeFLARM.h"
#include "Logger.h"
#include "MapWindow.h"
#include "externs.h"
#include "SettingsTask.hpp"
#include "Compatibility/string.h"
#include "Utils.h"
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

bool ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size){
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
    if (!ReplayLogger::IsEnabled() && GPS_INFO.MovementDetected) {
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
    if (LockSettingsInFlight && CALCULATED_INFO.Flying) {
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
    if (!GPS_INFO.FLARM_Available) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckFLARM)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckTerrain)"))) {
    if (!CALCULATED_INFO.TerrainValid) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckTerrain)"), TEXT(""), Size);
  }
  if (_tcsstr(OutBuffer, TEXT("$(CheckAutoMc)"))) {
    if (!ValidTaskPoint(ActiveWayPoint)
        && ((AutoMcMode==0)
	    || (AutoMcMode==2))) {
      invalid = true;
    }
    ReplaceInString(OutBuffer, TEXT("$(CheckAutoMc)"), TEXT(""), Size);
  }

  CondReplaceInString(LoggerActive, OutBuffer, TEXT("$(LoggerActive)"), TEXT("Stop"), TEXT("Start"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(SnailTrailToggleName)"))) {
    switch(TrailActive) {
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
    switch(VisualGlide) {
    case 0:
      ReplaceInString(OutBuffer, TEXT("$(VisualGlideToggleName)"), TEXT("Steady"), Size);
      break;
    case 1:
	if (ExtendedVisualGlide)
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
    switch(OnAirSpace) {
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
    if (EnableTopology) val++;
    if (EnableTerrain) val += (char)2;
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
    if (AutoForceFinalGlide) {
      invalid = true;
    }
  }

  CondReplaceInString(MapWindow::IsMapFullScreen(), OutBuffer, TEXT("$(FullScreenToggleActionName)"), TEXT("Off"), TEXT("On"), Size);
  CondReplaceInString(MapWindow::isAutoZoom(), OutBuffer, TEXT("$(ZoomAutoToggleActionName)"), TEXT("Manual"), TEXT("Auto"), Size);
  CondReplaceInString(EnableTopology, OutBuffer, TEXT("$(TopologyToggleActionName)"), TEXT("Off"), TEXT("On"), Size);
  CondReplaceInString(EnableTerrain, OutBuffer, TEXT("$(TerrainToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  if (_tcsstr(OutBuffer, TEXT("$(MapLabelsToggleActionName)"))) {
    switch(MapWindow::DeclutterLabels) {
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

  CondReplaceInString(CALCULATED_INFO.AutoMacCready != 0, OutBuffer, TEXT("$(MacCreadyToggleActionName)"), TEXT("Manual"), TEXT("Auto"), Size);
  CondReplaceInString(EnableAuxiliaryInfo, OutBuffer, TEXT("$(AuxInfoToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  CondReplaceInString(UserForceDisplayMode == dmCircling, OutBuffer, TEXT("$(DispModeClimbShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(UserForceDisplayMode == dmCruise, OutBuffer, TEXT("$(DispModeCruiseShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(UserForceDisplayMode == dmNone, OutBuffer, TEXT("$(DispModeAutoShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(UserForceDisplayMode == dmFinalGlide, OutBuffer, TEXT("$(DispModeFinalShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(AltitudeMode == ALLON, OutBuffer, TEXT("$(AirspaceModeAllShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == CLIP,  OutBuffer, TEXT("$(AirspaceModeClipShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == AUTO,  OutBuffer, TEXT("$(AirspaceModeAutoShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == ALLBELOW, OutBuffer, TEXT("$(AirspaceModeBelowShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(AltitudeMode == ALLOFF, OutBuffer, TEXT("$(AirspaceModeAllOffIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(TrailActive == 0, OutBuffer, TEXT("$(SnailTrailOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(TrailActive == 2, OutBuffer, TEXT("$(SnailTrailShortShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(TrailActive == 1, OutBuffer, TEXT("$(SnailTrailLongShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(TrailActive == 3, OutBuffer, TEXT("$(SnailTrailFullShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

// VENTA3 VisualGlide
  CondReplaceInString(VisualGlide == 0, OutBuffer, TEXT("$(VisualGlideOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(VisualGlide == 1, OutBuffer, TEXT("$(VisualGlideLightShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(VisualGlide == 2, OutBuffer, TEXT("$(VisualGlideHeavyShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
// VENTA3 AirSpace
  CondReplaceInString(OnAirSpace  == 0, OutBuffer, TEXT("$(AirSpaceOffShortIndicator)"), TEXT("(*)"), TEXT(""), Size);
  CondReplaceInString(OnAirSpace  == 1, OutBuffer, TEXT("$(AirSpaceOnShortIndicator)"), TEXT("(*)"), TEXT(""), Size);

  CondReplaceInString(EnableFLARMGauge != 0, OutBuffer, TEXT("$(FlarmDispToggleActionName)"), TEXT("Off"), TEXT("On"), Size);

  return invalid;
}
