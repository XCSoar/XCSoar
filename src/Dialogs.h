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

#if !defined(XCSOAR_DIALOGS_H)
#define XCSOAR_DIALOGS_H

#include "FLARM/FlarmId.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Dialogs/ListPicker.hpp"

#include <tchar.h>
#include "Util/tstring.hpp"

struct GeoPoint;
class ComboList;
class SingleWindow;
class WndProperty;
class Waypoint;
class Waypoints;
class Airspaces;
class AbstractAirspace;
class AbstractTaskFactory;
class OrderedTaskPoint;
namespace Pages {
  struct PageLayout;
}

void StartupScreen();

void dlgAirspaceWarningShowDlg();

void
dlgAirspaceWarningInit(SingleWindow &parent);

void dlgAirspaceWarningDeInit();
bool dlgAirspaceWarningIsEmpty();
bool dlgAirspaceWarningVisible();
void dlgAirspaceDetails(const AbstractAirspace& the_airspace);
int dlgAirspaceColoursShowModal();
int dlgAirspacePatternsShowModal();
void dlgAirspaceShowModal(bool colored);
void dlgAirspaceSelect();

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
Pages::PageLayout dlgConfigPageShowModal(Pages::PageLayout _pl, bool include_empty);
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

void dlgAnalysisShowModal();
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

bool
dlgTaskTypeShowModal(SingleWindow &parent, OrderedTask** task);

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
