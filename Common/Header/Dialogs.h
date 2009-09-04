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

#if !defined(XCSOAR_DIALOGS_H)
#define XCSOAR_DIALOGS_H

#include "WayPoint.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void StartupScreen();

bool dlgAirspaceWarningShowDlg(bool force);
int dlgWayPointSelect(double lon=0.0, double lat=90.0, int type=-1, int FilterNear=0);
int dlgAirspaceColoursShowModal(void);
int dlgAirspacePatternsShowModal(void);
void dlgAirspaceShowModal(bool colored);
void dlgBasicSettingsShowModal(void);
void dlgBrightnessShowModal(void);
void dlgHelpShowModal(const TCHAR* Caption, const TCHAR* HelpText);
void dlgChecklistShowModal(void);
void dlgConfigurationShowModal(void);
void dlgVegaDemoShowModal(void);
bool dlgConfigurationVarioShowModal(void);
void dlgLoggerReplayShowModal(void);
void dlgBasicSettingsShowModal(void);
void dlgStartupShowModal(void);
void dlgTaskCalculatorShowModal(void);
void dlgWindSettingsShowModal(void);
void dlgStartTaskShowModal(bool *validStart, double Time, double Speed, double Altitude);
void dlgAnalysisShowModal(void);
void dlgStatusShowModal(int page);
void dlgSwitchesShowModal(void);
void dlgTaskWaypointShowModal(int itemindex, int type, bool addonly=false);
void dlgTaskOverviewShowModal(void);
void dlgVoiceShowModal(void);
void dlgWayPointDetailsShowModal(void);
void dlgTextEntryShowModal(TCHAR *text, int width=0);
void dlgTeamCodeShowModal(void);
void dlgStartPointShowModal(void);
void dlgWaypointEditShowModal(WAYPOINT *wpt);
void dlgWeatherShowModal(void);
void dlgAirspaceSelect(void);
void dlgTarget(void);
bool dlgTaskRules(void);
void dlgAirspaceDetails(int the_circle, int the_area);
bool dlgAirspaceWarningVisible(void);
void dlgFlarmTrafficShowModal(void);
void dlgTextEntryKeyboardShowModal(TCHAR *text, int width=0);
void dlgNumberEntryKeyboardShowModal(int *value, int width=0);

void PopupWaypointDetails();
void PopupAnalysis();
bool PopupNearestWaypointDetails(double lon, double lat,
				 double range, bool pan);
bool PopupInteriorAirspaceDetails(double lon, double lat);

int
WINAPI
MessageBoxX(
    LPCTSTR lpText,
    LPCTSTR lpCaption,
    UINT uType);

#define mrOK             2
#define mrCancel         3


#endif
