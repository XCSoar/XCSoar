// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <tchar.h>

struct InputConfig;
class Menu;

typedef void (*pt2Event)(const char *);

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
GetMenu(const char *mode) noexcept;

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
setMode(const char *mode) noexcept;

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
SetFlavour(const char *flavour) noexcept;

/**
 * Is the specified flavour currently active?
 *
 * @return the current flavour mode; may be NULL
 */
[[gnu::pure]]
bool
IsFlavour(const char *flavour) noexcept;

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
processGesture(const char *data) noexcept;

bool
IsGesture(const char *data) noexcept;

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

void eventAbortTask(const char *misc);
void eventAdjustForecastTemperature(const char *misc);
void eventAdjustVarioFilter(const char *misc);
void eventAdjustWaypoint(const char *misc);
void eventAnalysis(const char *misc);
void eventArmAdvance(const char *misc);
void eventAudioDeadband(const char *misc);
void eventBallast(const char *misc);
void eventBugs(const char *misc);
void eventCalculator(const char *misc);
void eventChecklist(const char *misc);
void eventClearAirspaceWarnings(const char *misc);
void eventAirspaceWarnings(const char *misc);
void eventClearStatusMessages(const char *misc);
void eventLogger(const char *misc);
void eventMacCready(const char *misc);
void eventMainMenu(const char *misc);
void eventMarkLocation(const char *misc);
void eventPilotEvent(const char *misc);
void eventMode(const char *misc);
void eventNearestAirspaceDetails(const char *misc);
void eventNearestWaypointDetails(const char *misc);
void eventNearestMapItems(const char *misc);
void eventNull(const char *misc);
void eventPage(const char *misc);
void eventPan(const char *misc);
void eventPlaySound(const char *misc);
void eventProfileLoad(const char *misc);
void eventProfileSave(const char *misc);
void eventRepeatStatusMessage(const char *misc);
void eventRun(const char *misc);
void eventQuickGuide(const char *misc);
void eventGestureHelp(const char *misc);
void eventScreenModes(const char *misc);
void eventDevice(const char *misc);
void eventSendNMEA(const char *misc);
void eventSendNMEAPort1(const char *misc);
void eventSendNMEAPort2(const char *misc);
void eventSetup(const char *misc);
void eventSnailTrail(const char *misc);
void eventAirSpace(const char *misc); // VENTA3
void eventSounds(const char *misc);
void eventStatus(const char *misc);
void eventStatusMessage(const char *misc);
void eventTaskLoad(const char *misc);
void eventTaskSave(const char *misc);
void eventTaskTransition(const char *misc);
void eventTerrainTopography(const char *misc);
void eventTerrainTopology(const char *misc);
void eventWaypointDetails(const char *misc);
void eventWaypointEditor(const char *misc);
void eventZoom(const char *misc);
void eventBrightness(const char *misc);
void eventDeclutterLabels(const char *misc);
void eventExit(const char *misc);
void eventFLARMRadar(const char *misc);
void eventThermalAssistant(const char *misc);
void eventBeep(const char *misc);
void eventUserDisplayModeForce(const char *misc);
void eventAirspaceDisplayMode(const char *misc);
void eventAutoLogger(const char *misc);
void eventGotoLookup(const char *misc);
void eventAddWaypoint(const char *misc);
void eventTraffic(const char *misc);
void eventFlarmTraffic(const char *misc);
void eventFlarmDetails(const char *misc);
void eventCredits(const char *misc);
void eventWeather(const char *misc);
void eventQuickMenu(const char *misc);
void eventFileManager(const char *misc);
void eventRunLuaFile(const char *misc);
void eventResetTask(const char *misc);
void eventLockScreen(const char *misc);
void eventExchangeFrequencies(const char *misc);
void eventUploadIGCFile(const char *misc);
void eventOrientationCruise(const char *misc);
void eventOrientationCircling(const char *misc);
// -------

} // namespace InputEvents
