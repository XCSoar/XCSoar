/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "ButtonLabel.hpp"
#include "Language.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Logger/Logger.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "SettingsComputer.hpp"
#include "Components.hpp"
#include "Compatibility/string.h"
#include "SettingsUser.hpp"
#include "Simulator.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Replay/Replay.hpp"

#include <stdlib.h>

/**
 * Replaces ToReplace with ReplaceWith in String
 * @param String Buffer string
 * @param ToReplace The string that will be replaced
 * @param ReplaceWith The replacement
 * @param Size (?)
 */
static void
ReplaceInString(TCHAR *String, const TCHAR *ToReplace,
                const TCHAR *ReplaceWith, size_t Size)
{
  TCHAR TmpBuf[MAX_PATH];
  size_t iR = _tcslen(ToReplace);
  TCHAR *pC;

  while ((pC = _tcsstr(String, ToReplace)) != NULL) {
    _tcscpy(TmpBuf, pC + iR);
    _tcscpy(pC, ReplaceWith);
    _tcscat(pC, TmpBuf);
  }
}

/**
 * If Condition is true, Macro in Buffer will be replaced by TrueText,
 * otherwise by FalseText.
 * @param Condition Condition to be checked
 * @param Buffer Buffer string
 * @param Macro The string that will be replaced
 * @param TrueText The replacement if Condition is true
 * @param FalseText The replacement if Condition is false
 * @param Size (?)
 */
static void
CondReplaceInString(bool Condition, TCHAR *Buffer, const TCHAR *Macro,
                    const TCHAR *TrueText, const TCHAR *FalseText, size_t Size)
{
  if (Condition)
    ReplaceInString(Buffer, Macro, TrueText, Size);
  else
    ReplaceInString(Buffer, Macro, FalseText, Size);
}

static bool
ExpandTaskMacros(TCHAR *OutBuffer, size_t Size,
                 const DERIVED_INFO &calculated,
                 const SETTINGS_COMPUTER &settings_computer)
{
  bool invalid = false;

  if (_tcsstr(OutBuffer, _T("$(CheckTaskResumed)"))) {
    // TODO code: check, does this need to be set with temporary task?
    invalid |= calculated.common_stats.mode_abort;
    invalid |= calculated.common_stats.mode_goto;
    ReplaceInString(OutBuffer, _T("$(CheckTaskResumed)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckTask)"))) {
    if (!calculated.task_stats.task_valid)
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckTask)"), _T(""), Size);
  }

  if (!calculated.task_stats.task_valid
      || calculated.common_stats.mode_goto) {

    if (_tcsstr(OutBuffer, _T("$(WaypointNext)"))) {
      invalid = true;
      ReplaceInString(OutBuffer, _T("$(WaypointNext)"),
          _T("Next\nTurnpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPrevious)"))) {
      invalid = true;
      ReplaceInString(OutBuffer, _T("$(WaypointPrevious)"),
          _T("Previous\nTurnpoint"), Size);
    }

  } else if (calculated.common_stats.mode_abort) {

    if (_tcsstr(OutBuffer, _T("$(WaypointNext)"))) {
      invalid |= !calculated.common_stats.active_has_next;
      CondReplaceInString(calculated.common_stats.next_is_last,
                          OutBuffer,
                          _T("$(WaypointNext)"),
                          _T("Furthest\nLandpoint"),
                          _T("Next\nLandpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPrevious)"))) {
      invalid |= !calculated.common_stats.active_has_previous;
      CondReplaceInString(calculated.common_stats.previous_is_first,
                          OutBuffer,
                          _T("$(WaypointPrevious)"),
                          _T("Closest\nLandpoint"),
                          _T("Previous\nLandpoint"), Size);
    }

  } else {
    if (_tcsstr(OutBuffer, _T("$(WaypointNext)"))) {
      // Waypoint\nNext
      invalid |= !calculated.common_stats.active_has_next;
      CondReplaceInString(calculated.common_stats.next_is_last,
                          OutBuffer,
                          _T("$(WaypointNext)"),
                          _T("Finish\nTurnpoint"),
                          _T("Next\nTurnpoint"), Size);

    } else if (_tcsstr(OutBuffer, _T("$(WaypointPrevious)"))) {
      invalid |= !calculated.common_stats.active_has_previous;
      CondReplaceInString(calculated.common_stats.previous_is_first,
                          OutBuffer,
                          _T("$(WaypointPrevious)"),
                          _T("Start\nTurnpoint"),
                          _T("Previous\nTurnpoint"), Size);
    } 
#ifdef OLD_TASK // multiple start points
    else if (task.getSettings().EnableMultipleStartPoints) {
      invalid |= !task.ValidTaskPoint(0);
      CondReplaceInString((task.getActiveIndex()==0),
                          OutBuffer,
                          _T("$(WaypointPrevious)"),
                          _T("StartPoint\nCycle"), _T("Waypoint\nPrevious"), Size);
    } 
    else {
      invalid |= !calculated.common_stats.active_has_previous;
      ReplaceInString(OutBuffer, _T("$(WaypointPrevious)"), _T("Waypoint\nPrevious"), Size);
    }
#endif
  }

  if (_tcsstr(OutBuffer, _T("$(AdvanceArmed)"))) {
    TaskAdvance::TaskAdvanceState_t s =
      protected_task_manager.get_advance_state();
    switch (s) {
    case TaskAdvance::MANUAL:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"), 
                      _T("Advance\n(manual)"), Size);
      invalid = true;
      break;
    case TaskAdvance::AUTO:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"), 
                      _T("Advance\n(auto)"), Size);
      invalid = true;
      break;
    case TaskAdvance::START_ARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"), 
                      _T("Abort\nStart"), Size);
      invalid = false;
      break;
    case TaskAdvance::START_DISARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"), 
                      _T("Arm\nStart"), Size);
      invalid = false;
      break;
    case TaskAdvance::TURN_ARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"), 
                      _T("Abort\nTurn"), Size);
      invalid = false;
      break;
    case TaskAdvance::TURN_DISARMED:
      ReplaceInString(OutBuffer, _T("$(AdvanceArmed)"), 
                      _T("Arm\nTurn"), Size);
      invalid = false;
      break;
    default:
      assert(1);
    }
  }

  if (_tcsstr(OutBuffer, _T("$(CheckAutoMc)"))) {
    if (!calculated.task_stats.task_valid
        && ((settings_computer.auto_mc_mode==TaskBehaviour::AUTOMC_FINALGLIDE)
            || (settings_computer.auto_mc_mode==TaskBehaviour::AUTOMC_BOTH)))
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckAutoMc)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(TaskAbortToggleActionName)"))) {
    if (calculated.common_stats.mode_goto) {
      CondReplaceInString(calculated.common_stats.ordered_valid,
                          OutBuffer, _T("$(TaskAbortToggleActionName)"),
                          _T("Resume"), _T("Abort"), Size);
    } else 
      CondReplaceInString(calculated.common_stats.mode_abort,
                          OutBuffer, _T("$(TaskAbortToggleActionName)"),
                          _T("Resume"), _T("Abort"), Size);
  }

  return invalid;
}

bool
ButtonLabel::ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size)
{
  // ToDo, check Buffer Size
  bool invalid = false;
  _tcsncpy(OutBuffer, In, Size);
  OutBuffer[Size - 1] = '\0';

  if (_tcsstr(OutBuffer, _T("$(")) == NULL)
    return false;

  if (_tcsstr(OutBuffer, _T("$(CheckAirspace)"))) {
    if (airspace_database.empty())
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckAirspace)"), _T(""), Size);
  }

  invalid |= ExpandTaskMacros(OutBuffer, Size,
                              Calculated(), SettingsComputer());

  if (_tcsstr(OutBuffer, _T("$(CheckReplay)"))) {
    if (!Basic().gps.Replay
        && !replay.NmeaReplayEnabled()
        && Basic().gps.MovementDetected)
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckReplay)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckWaypointFile)"))) {
    invalid |= way_points.empty();
    ReplaceInString(OutBuffer, _T("$(CheckWaypointFile)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckSettingsLockout)"))) {
    if (!is_simulator() && XCSoarInterface::LockSettingsInFlight &&
        Basic().flight.Flying)
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckSettingsLockout)"), _T(""), Size);
  }

  if (_tcsstr(OutBuffer, _T("$(CheckFLARM)"))) {
    if (!Basic().flarm.FLARM_Available)
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckFLARM)"), _T(""), Size);
  }
  if (_tcsstr(OutBuffer, _T("$(CheckTerrain)"))) {
    if (!Calculated().TerrainValid)
      invalid = true;

    ReplaceInString(OutBuffer, _T("$(CheckTerrain)"), _T(""), Size);
  }

  CondReplaceInString(logger.isLoggerActive(), OutBuffer,
                      _T("$(LoggerActive)"), _T("Stop"),
                      _T("Start"), Size);

  if (_tcsstr(OutBuffer, _T("$(SnailTrailToggleName)"))) {
    switch(SettingsMap().TrailActive) {
    case 0:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _T("Long"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _T("Short"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _T("Full"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, _T("$(SnailTrailToggleName)"),
                      _T("Off"), Size);
      break;
    }
  }

  if (_tcsstr(OutBuffer, _T("$(VisualGlideToggleName)"))) {
    switch(SettingsMap().VisualGlide) {
    case 0:
      ReplaceInString(OutBuffer, _T("$(VisualGlideToggleName)"),
                      _T("Steady"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, _T("$(VisualGlideToggleName)"),
                      SettingsMap().ExtendedVisualGlide ?
                          _T("Moving") : _T("Off"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, _T("$(VisualGlideToggleName)"),
                      _T("Off"), Size);
      break;
    }
  }

  if (_tcsstr(OutBuffer, _T("$(AirSpaceToggleName)"))) {
    switch(SettingsMap().OnAirSpace) {
    case 0:
      ReplaceInString(OutBuffer, _T("$(AirSpaceToggleName)"),
                      _T("ON"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, _T("$(AirSpaceToggleName)"),
                      _T("OFF"), Size);
      break;
    }
  }

  if (_tcsstr(OutBuffer, _T("$(TerrainTopologyToggleName)"))) {
    char val = 0;
    if (SettingsMap().EnableTopology)
      val++;
    if (SettingsMap().EnableTerrain)
      val += (char)2;
    switch (val) {
    case 0:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _T("Topology\nOn"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _T("Terrain\nOn"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _T("Terrain\nTopology"), Size);
      break;
    case 3:
      ReplaceInString(OutBuffer, _T("$(TerrainTopologyToggleName)"),
                      _("Terrain\nOff"), Size);
      break;
    }
  }

  CondReplaceInString(SettingsMap().FullScreen, OutBuffer,
                      _T("$(FullScreenToggleActionName)"),
                      _T("Off"), _T("On"), Size);
  CondReplaceInString(SettingsMap().AutoZoom, OutBuffer,
		                  _T("$(ZoomAutoToggleActionName)"),
		                  _T("Manual"), _T("Auto"), Size);
  CondReplaceInString(SettingsMap().EnableTopology, OutBuffer,
                      _T("$(TopologyToggleActionName)"),
                      _T("Off"), _T("On"), Size);
  CondReplaceInString(SettingsMap().EnableTerrain, OutBuffer,
                      _T("$(TerrainToggleActionName)"),
                      _T("Off"), _T("On"), Size);

  if (_tcsstr(OutBuffer, _T("$(MapLabelsToggleActionName)"))) {
    switch(SettingsMap().DeclutterLabels) {
    case 0:
      ReplaceInString(OutBuffer, _T("$(MapLabelsToggleActionName)"),
                      _T("MID"), Size);
      break;
    case 1:
      ReplaceInString(OutBuffer, _T("$(MapLabelsToggleActionName)"),
                      _T("OFF"), Size);
      break;
    case 2:
      ReplaceInString(OutBuffer, _T("$(MapLabelsToggleActionName)"),
                      _T("ON"), Size);
      break;
    }
  }

  CondReplaceInString(SettingsComputer().auto_mc,
                      OutBuffer, _T("$(MacCreadyToggleActionName)"),
                      _T("Manual"), _T("Auto"), Size);
  CondReplaceInString(SettingsMap().EnableAuxiliaryInfo,
                      OutBuffer, _T("$(AuxInfoToggleActionName)"),
                      _T("Off"), _T("On"), Size);

  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmCircling,
                      OutBuffer, _T("$(DispModeClimbShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmCruise,
                      OutBuffer, _T("$(DispModeCruiseShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmNone,
                      OutBuffer, _T("$(DispModeAutoShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().UserForceDisplayMode == dmFinalGlide,
                      OutBuffer, _T("$(DispModeFinalShortIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(SettingsComputer().AltitudeMode == ALLON,
                      OutBuffer, _T("$(AirspaceModeAllShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsComputer().AltitudeMode == CLIP,
                      OutBuffer, _T("$(AirspaceModeClipShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsComputer().AltitudeMode == AUTO,
                      OutBuffer, _T("$(AirspaceModeAutoShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsComputer().AltitudeMode == ALLBELOW,
                      OutBuffer, _T("$(AirspaceModeBelowShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsComputer().AltitudeMode == ALLOFF,
                      OutBuffer, _T("$(AirspaceModeAllOffIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(SettingsMap().TrailActive == 0,
                      OutBuffer, _T("$(SnailTrailOffShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().TrailActive == 2,
                      OutBuffer, _T("$(SnailTrailShortShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().TrailActive == 1,
                      OutBuffer, _T("$(SnailTrailLongShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().TrailActive == 3,
                      OutBuffer, _T("$(SnailTrailFullShortIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(SettingsMap().VisualGlide == 0,
                      OutBuffer, _T("$(VisualGlideOffShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().VisualGlide == 1,
                      OutBuffer, _T("$(VisualGlideLightShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().VisualGlide == 2,
                      OutBuffer, _T("$(VisualGlideHeavyShortIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(SettingsMap().OnAirSpace == 0,
                      OutBuffer, _T("$(AirSpaceOffShortIndicator)"),
                      _T("(*)"), _T(""), Size);
  CondReplaceInString(SettingsMap().OnAirSpace == 1,
                      OutBuffer, _T("$(AirSpaceOnShortIndicator)"),
                      _T("(*)"), _T(""), Size);

  CondReplaceInString(SettingsMap().EnableFLARMGauge != 0,
                      OutBuffer, _T("$(FlarmDispToggleActionName)"),
                      _T("Off"), _T("On"), Size);

  return invalid;
}
