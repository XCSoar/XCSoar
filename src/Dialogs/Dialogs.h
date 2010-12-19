/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#if !defined(XCSOAR_DIALOGS_H)
#define XCSOAR_DIALOGS_H

#include "InfoBoxes/InfoBoxManager.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Task/Tasks/OrderedTask.hpp"

#include <tchar.h>
#include "Util/tstring.hpp"

struct GeoPoint;
class FlarmId;
class ComboList;
class SingleWindow;
class WndProperty;
class Waypoint;
class Waypoints;
class Airspaces;
class AbstractAirspace;
class AbstractTaskFactory;
class OrderedTaskPoint;

void StartupScreen();

void dlgAirspaceWarningShowDlg(bool auto_close = false);

void
dlgAirspaceWarningInit(SingleWindow &parent);

void dlgAirspaceWarningDeInit();
bool dlgAirspaceWarningVisible();
void dlgAirspaceDetails(const AbstractAirspace& the_airspace);
int dlgAirspaceColoursShowModal();
int dlgAirspacePatternsShowModal();
void dlgAirspaceShowModal(bool colored);
void dlgAirspaceSelect();

void dlgAlternatesListShowModal(SingleWindow &parent);

const Waypoint *
dlgWayPointSelect(SingleWindow &parent,
                  const GeoPoint &location);

void dlgBasicSettingsShowModal();
void dlgBrightnessShowModal();
void dlgHelpShowModal(SingleWindow &parent, const TCHAR* Caption,
    const TCHAR* HelpText);

void dlgChecklistShowModal();
void dlgConfigurationShowModal();
void dlgConfigFontsShowModal();
void dlgConfigWaypointsShowModal();
void dlgConfigInfoboxesShowModal(InfoBoxManager::mode _mode);
void dlgVegaDemoShowModal();
bool dlgConfigurationVarioShowModal();
void dlgLoggerReplayShowModal();

/**
 * @return true on success, false if the user has pressed the "Quit"
 * button
 */
bool
dlgStartupShowModal();

bool dlgSimulatorPromptShowModal();


void
dlgTaskCalculatorShowModal(SingleWindow &parent);

void dlgWindSettingsShowModal();
void dlgStartTaskShowModal(bool *validStart, double Time, double Speed,
    double Altitude);

void
dlgAnalysisShowModal(SingleWindow &parent, int page = -1);

void dlgStatusShowModal(int page);

void dlgSwitchesShowModal();

class OrderedTask;

void
dlgTaskManagerShowModal(SingleWindow &parent);

bool
dlgTaskEditShowModal(SingleWindow &parent, OrderedTask** task);

bool
dlgTaskPropertiesShowModal(SingleWindow &parent, OrderedTask** task);

bool
dlgTaskListShowModal(SingleWindow &parent, OrderedTask** task);

/**
 *
 * @param task - not modified
 * @param task_type_returned type of task selected in UI
 * @return true if OK was clicked, false if Cancel was clicked
 */
bool
dlgTaskTypeShowModal(SingleWindow &parent, OrderedTask** task, OrderedTask::Factory_t& task_type_returned);

bool
dlgTaskPointShowModal(SingleWindow &parent, OrderedTask** task, const unsigned index);

bool
dlgTaskPointType(SingleWindow &parent, OrderedTask** task, const unsigned index);

bool
dlgTaskPointNew(SingleWindow &parent, OrderedTask** task, const unsigned index);

void dlgVoiceShowModal();

void
dlgWayPointDetailsShowModal(SingleWindow &parent, const Waypoint& waypoint);

typedef const TCHAR *(*AllowedCharactersCallback_t)(const TCHAR *value);

bool dlgTextEntryShowModal(TCHAR *text, int width,
                           AllowedCharactersCallback_t accb=NULL);
bool dlgTextEntryShowModal(tstring &text, int width,
                           AllowedCharactersCallback_t accb=NULL);

void dlgTeamCodeShowModal();
void dlgStartPointShowModal();

bool dlgWaypointEditShowModal(Waypoint &way_point);

void dlgWeatherShowModal();
void dlgTargetShowModal();
void dlgThermalAssistantShowModal();
void dlgFlarmTrafficShowModal();
void dlgFlarmTrafficDetailsShowModal(FlarmId id);
bool dlgTextEntryKeyboardShowModal(TCHAR *text, int width = 0,
                                   AllowedCharactersCallback_t accb=NULL);
void dlgNumberEntryKeyboardShowModal(int *value, int width = 0);

int
ComboPicker(SingleWindow &parent, const TCHAR *caption,
            const ComboList &combo_list,
            ListHelpCallback_t help_callback);

int dlgComboPicker(SingleWindow &parent, WndProperty *theProperty);

bool PopupNearestWaypointDetails(const Waypoints &way_points,
    const GeoPoint &location, double range, bool scalefilter = false);

#define mrOK             2
#define mrCancel         3

#endif
