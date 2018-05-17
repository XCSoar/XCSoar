/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef XCSOAR_INPUT_EVENTS_HPP
#define XCSOAR_INPUT_EVENTS_HPP

#include "Compiler.h"

#include <tchar.h>

struct InputConfig;
class Menu;

typedef void (*pt2Event)(const TCHAR *);

namespace InputEvents
{
  enum Mode {
    MODE_DEFAULT,
    MODE_PAN,
    MODE_INFOBOX,
    MODE_MENU
  };

  void ProcessTimer();
  void ShowMenu();
  void HideMenu();
  Menu *GetMenu(const TCHAR *mode);

  /**
   * Load the default input file (Data/Input/default.xci).
   */
  void LoadDefaults(InputConfig &input_config);

  void readFile();
  void setMode(Mode mode);
  void setMode(const TCHAR *mode);

  /**
   * Update the menu after pan mode has been enabled or disabled.
   */
  void UpdatePan();

  /**
   * Set the "flavour" of the current mode.  It is an optional string
   * that gets appended to the current mode name, separated with a
   * dot, to build a new "overlay" mode.  This may be used to replace
   * certain items in the menu.
   *
   * @param flavour the new flavour name; may be NULL
   */
  void SetFlavour(const TCHAR *flavour);

  /**
   * Is the specified flavour currently active?
   *
   * @return the current flavour mode; may be NULL
   */
  gcc_pure
  bool IsFlavour(const TCHAR *flavour);

  /**
   * @return: true if current mode is MODE_DEFAULT
   */
  gcc_pure
  bool IsDefault();

  /**
   * Process an event chain.
   */
  void ProcessEvent(unsigned event_id);

  /**
   * Process a hot key for the specified mode.
   */
  bool ProcessKey(Mode mode, unsigned key_code);

  bool processKey(unsigned key);
  bool processGesture(const TCHAR *data);
  bool IsGesture(const TCHAR *data);

  bool processNmea_real(unsigned key);
  bool processGlideComputer_real(unsigned gce_id);

  // helpers (temporary)

  void sub_TerrainTopography(int vswitch);

  void sub_PanCursor(int dx, int dy);
  void sub_AutoZoom(int vswitch);
  void sub_ScaleZoom(int vswitch);
  void sub_SetZoom(double value);

  // -------

  void eventAbortTask(const TCHAR *misc);
  void eventAdjustForecastTemperature(const TCHAR *misc);
  void eventAdjustVarioFilter(const TCHAR *misc);
  void eventAdjustWaypoint(const TCHAR *misc);
  void eventAnalysis(const TCHAR *misc);
  void eventArmAdvance(const TCHAR *misc);
  void eventAudioDeadband(const TCHAR *misc);
  void eventBallast(const TCHAR *misc);
  void eventBugs(const TCHAR *misc);
  void eventCalculator(const TCHAR *misc);
  void eventChecklist(const TCHAR *misc);
  void eventClearAirspaceWarnings(const TCHAR *misc);
  void eventClearStatusMessages(const TCHAR *misc);
  void eventLogger(const TCHAR *misc);
  void eventMacCready(const TCHAR *misc);
  void eventMainMenu(const TCHAR *misc);
  void eventMarkLocation(const TCHAR *misc);
  void eventMode(const TCHAR *misc);
  void eventNearestAirspaceDetails(const TCHAR *misc);
  void eventNearestWaypointDetails(const TCHAR *misc);
  void eventNearestMapItems(const TCHAR *misc);
  void eventNull(const TCHAR *misc);
  void eventPage(const TCHAR *misc);
  void eventPan(const TCHAR *misc);
  void eventPlaySound(const TCHAR *misc);
  void eventProfileLoad(const TCHAR *misc);
  void eventProfileSave(const TCHAR *misc);
  void eventRepeatStatusMessage(const TCHAR *misc);
  void eventRun(const TCHAR *misc);
  void eventScreenModes(const TCHAR *misc);
  void eventDevice(const TCHAR *misc);
  void eventSendNMEA(const TCHAR *misc);
  void eventSendNMEAPort1(const TCHAR *misc);
  void eventSendNMEAPort2(const TCHAR *misc);
  void eventSetup(const TCHAR *misc);
  void eventSnailTrail(const TCHAR *misc);
  void eventAirSpace(const TCHAR *misc); // VENTA3
  void eventSounds(const TCHAR *misc);
  void eventStatus(const TCHAR *misc);
  void eventStatusMessage(const TCHAR *misc);
  void eventTaskLoad(const TCHAR *misc);
  void eventTaskSave(const TCHAR *misc);
  void eventTaskTransition(const TCHAR *misc);
  void eventTerrainTopography(const TCHAR *misc);
  void eventTerrainTopology(const TCHAR *misc);
  void eventWaypointDetails(const TCHAR *misc);
  void eventWaypointEditor(const TCHAR *misc);
  void eventZoom(const TCHAR *misc);
  void eventBrightness(const TCHAR *misc);
  void eventDeclutterLabels(const TCHAR *misc);
  void eventExit(const TCHAR *misc);
  void eventFLARMRadar(const TCHAR *misc);
  void eventThermalAssistant(const TCHAR *misc);
  void eventBeep(const TCHAR *misc);
  void eventUserDisplayModeForce(const TCHAR *misc);
  void eventAirspaceDisplayMode(const TCHAR *misc);
  void eventAutoLogger(const TCHAR *misc);
  void eventGotoLookup(const TCHAR *misc);
  void eventAddWaypoint(const TCHAR *misc);
  void eventOrientation(const TCHAR *misc);
  void eventTraffic(const TCHAR *misc);
  void eventFlarmTraffic(const TCHAR *misc);
  void eventFlarmDetails(const TCHAR *misc);
  void eventCredits(const TCHAR *misc);
  void eventWeather(const TCHAR *misc);
  void eventQuickMenu(const TCHAR *misc);
  void eventFileManager(const TCHAR *misc);
  void eventRunLuaFile(const TCHAR *misc);
  void eventResetTask(const TCHAR *misc);

  // -------
};

#endif
