/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Form/Form.hpp"

#include <tchar.h>

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
class InfoBoxPanelConfig;

void StartupScreen();

void dlgAirspaceWarningsShowModal(SingleWindow &parent, bool auto_close = false);
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
void dlgWayPointSelectAddToLastUsed(const Waypoint &wp);

void dlgBasicSettingsShowModal();
void dlgBrightnessShowModal();
void dlgHelpShowModal(SingleWindow &parent, const TCHAR* Caption,
    const TCHAR* HelpText);

void dlgChecklistShowModal();
void dlgConfigurationShowModal();
void dlgConfigFontsShowModal();
void dlgConfigWaypointsShowModal();

void dlgVegaDemoShowModal();
bool dlgConfigurationVarioShowModal();
void dlgLoggerReplayShowModal();

/**
 * @return true on success, false if the user has pressed the "Quit"
 * button
 */
bool
dlgStartupShowModal();

void dlgWindSettingsShowModal();
void dlgStartTaskShowModal(bool *validStart, double Time, double Speed,
    double Altitude);

void
dlgAnalysisShowModal(SingleWindow &parent, int page = -1);

void dlgStatusShowModal(int page);

void dlgSwitchesShowModal();

void
dlgInfoBoxAccessShowModal(SingleWindow &parent, const int id);

class OrderedTask;

void
dlgTaskManagerShowModal(SingleWindow &parent);

bool
dlgTaskPointShowModal(SingleWindow &parent, OrderedTask** task, const unsigned index);

bool
dlgTaskPointType(SingleWindow &parent, OrderedTask** task, const unsigned index);

bool
dlgTaskOptionalStarts(SingleWindow &parent, OrderedTask** task);

bool
dlgTaskPointNew(SingleWindow &parent, OrderedTask** task, const unsigned index);

void dlgVoiceShowModal();

void
dlgWayPointDetailsShowModal(SingleWindow &parent, const Waypoint& waypoint,
                          bool allow_navigation = true);

void dlgTeamCodeShowModal();
void dlgStartPointShowModal();

bool dlgWaypointEditShowModal(Waypoint &way_point);

void dlgWeatherShowModal();

/**
 * Shows map display zoomed to target point
 * with half dialog popup to manipulate point
 *
 * @param TargetPoint if -1 then goes to active target
 * else goes to TargetPoint by default
 */
void dlgTargetShowModal(int TargetPoint = -1);
void dlgThermalAssistantShowModal();
void dlgFlarmTrafficShowModal();
void dlgFlarmTrafficDetailsShowModal(FlarmId id);

bool PopupNearestWaypointDetails(const Waypoints &way_points,
    const GeoPoint &location, double range, bool scalefilter = false);

#endif
