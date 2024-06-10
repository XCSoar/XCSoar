// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <tchar.h>

struct InputConfig;
class Menu;

typedef void (*pt2Event)(const TCHAR *);

namespace InputEvents {

enum Mode {
  MODE_DEFAULT,
  MODE_PAN,
  MODE_INFOBOX,
  MODE_MENU
};

void
ProcessTimer() noexcept;

void
ShowMenu() noexcept;

void
HideMenu() noexcept;

Menu *
GetMenu(const TCHAR *mode) noexcept;

/**
 * Load the default input file (Data/Input/default.xci).
 */
void LoadDefaults(InputConfig &input_config);

/**
 * Throws on error.
 */
void readFile();

void
setMode(Mode mode) noexcept;

void
setMode(const TCHAR *mode) noexcept;

/**
 * Update the menu after pan mode has been enabled or disabled.
 */
void
UpdatePan() noexcept;

/**
 * Set the "flavour" of the current mode.  It is an optional string
 * that gets appended to the current mode name, separated with a
 * dot, to build a new "overlay" mode.  This may be used to replace
 * certain items in the menu.
 *
 * @param flavour the new flavour name; may be NULL
 */
void
SetFlavour(const TCHAR *flavour) noexcept;

/**
 * Is the specified flavour currently active?
 *
 * @return the current flavour mode; may be NULL
 */
[[gnu::pure]]
bool
IsFlavour(const TCHAR *flavour) noexcept;

/**
 * @return: true if current mode is MODE_DEFAULT
 */
[[gnu::pure]]
bool
IsDefault() noexcept;

/**
 * Process an event chain.
 */
void
ProcessEvent(unsigned event_id) noexcept;

/**
 * Process a hot key for the specified mode.
 */
bool
ProcessKey(Mode mode, unsigned key_code) noexcept;

bool
processKey(unsigned key) noexcept;

bool
processGesture(const TCHAR *data) noexcept;

bool
IsGesture(const TCHAR *data) noexcept;

bool
processNmea_real(unsigned key) noexcept;

bool
processGlideComputer_real(unsigned gce_id) noexcept;

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
void eventFrequencies(const TCHAR *misc);
void eventClearAirspaceWarnings(const TCHAR *misc);
void eventClearStatusMessages(const TCHAR *misc);
void eventLogger(const TCHAR *misc);
void eventMacCready(const TCHAR *misc);
void eventMainMenu(const TCHAR *misc);
void eventMarkLocation(const TCHAR *misc);
void eventPilotEvent(const TCHAR *misc);
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
void eventTraffic(const TCHAR *misc);
void eventFlarmTraffic(const TCHAR *misc);
void eventFlarmDetails(const TCHAR *misc);
void eventCredits(const TCHAR *misc);
void eventWeather(const TCHAR *misc);
void eventQuickMenu(const TCHAR *misc);
void eventFileManager(const TCHAR *misc);
void eventRunLuaFile(const TCHAR *misc);
void eventResetTask(const TCHAR *misc);
void eventLockScreen(const TCHAR *misc);
void eventExchangeFrequencies(const TCHAR *misc);
void eventUploadIGCFile(const TCHAR *misc);
void eventOrientationCruise(const TCHAR *misc);
void eventOrientationCircling(const TCHAR *misc);
// -------

} // namespace InputEvents
