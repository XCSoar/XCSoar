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

#include "Protection.hpp"
#include "MainWindow.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "SettingsTask.hpp"
#include "RasterTerrain.h"
#include "AirfieldDetails.h"
#include "TopologyStore.h"
#include "Dialogs.h"
#include "Device/device.hpp"
#include "Message.h"
#include "Polar/Loader.hpp"
#include "TopologyStore.h"
#include "Components.hpp"
#include "Interface.hpp"
#include "Language.hpp"
#include "LogFile.hpp"
#include "Simulator.hpp"
#include "DrawThread.hpp"
#include "CalculationThread.hpp"
#include "AirspaceGlue.hpp"
#include "WayPointParser.h"
#include "Task/TaskManager.hpp"

#if defined(__BORLANDC__)  // due to compiler bug
  #include "Waypoint/Waypoints.hpp"
  #include "Airspace/Airspaces.hpp"
  #include "Polar/Polar.hpp"
#endif

bool DevicePortChanged = false;
bool MapFileChanged = false;
bool AirspaceFileChanged = false;
bool AirfieldFileChanged = false;
bool WaypointFileChanged = false;
bool TerrainFileChanged = false;
bool TopologyFileChanged = false;
bool PolarFileChanged = false;
bool LanguageFileChanged = false;
bool StatusFileChanged = false;
bool InputFileChanged = false;

void SettingsEnter() {
  draw_thread->suspend();
  // This prevents the map and calculation threads from doing anything
  // with shared data while it is being changed (also prevents drawing)

  MapFileChanged = false;
  AirspaceFileChanged = false;
  AirfieldFileChanged = false;
  WaypointFileChanged = false;
  TerrainFileChanged = false;
  TopologyFileChanged = false;
  PolarFileChanged = false;
  LanguageFileChanged = false;
  StatusFileChanged = false;
  InputFileChanged = false;
  DevicePortChanged = false;
}

void SettingsLeave() {
  if (!globalRunningEvent.test()) return;

  XCSoarInterface::main_window.map.set_focus();

  calculation_thread->suspend();

/*
  if (MapFileChanged) { printf("MapFileChanged\n"); }
  if (AirspaceFileChanged) { printf("AirspaceFileChanged\n"); }
  if (AirfieldFileChanged) { printf("AirfieldFileChanged\n"); }
  if (WaypointFileChanged) { printf("WaypointFileChanged\n"); }
  if (TerrainFileChanged) { printf("TerrainFileChanged\n"); }
  if (TopologyFileChanged) { printf("TopologyFileChanged\n"); }
  if (PolarFileChanged) { printf("PolarFileChanged\n"); }
  if (LanguageFileChanged) { printf("LanguageFileChanged\n"); }
  if (StatusFileChanged) { printf("StatusFileChanged\n"); }
  if (InputFileChanged) { printf("InputFileChanged\n"); }
  if (DevicePortChanged) { printf("DevicePortChanged\n"); }
*/

  if(MapFileChanged) {
    AirspaceFileChanged = true;
    AirfieldFileChanged = true;
    WaypointFileChanged = true;
    TerrainFileChanged = true;
    TopologyFileChanged = true;
  }

  if((WaypointFileChanged) || (TerrainFileChanged) || (AirfieldFileChanged)) {

    XCSoarInterface::CreateProgressDialog(gettext(TEXT("Loading Terrain File...")));
    XCSoarInterface::SetProgressStepSize(2);

    // re-load terrain
    terrain.CloseTerrain();
    terrain.OpenTerrain();

    // re-load waypoints
    ReadWaypoints(way_points, &terrain);
    ReadAirfieldFile();

    // re-set home
    if (WaypointFileChanged || TerrainFileChanged) {
      SetHome(way_points, &terrain, XCSoarInterface::SetSettingsComputer(),
          WaypointFileChanged);
    }

    terrain.ServiceFullReload(XCSoarInterface::Basic().Location);
  }

  if (TopologyFileChanged) {
    topology->Close();
    topology->Open();
  }

  if(AirspaceFileChanged) {
    CloseAirspace(airspace_database, airspace_warning);
    ReadAirspace(airspace_database, &terrain, XCSoarInterface::Basic().pressure);
  }

  if (PolarFileChanged) {
    GlidePolar gp = task_manager.get_glide_polar();
    if (LoadPolarById(XCSoarInterface::SettingsComputer(), gp)) {
      task_manager.set_glide_polar(gp);
    }
  }

  if (AirfieldFileChanged
      || AirspaceFileChanged
      || WaypointFileChanged
      || TerrainFileChanged
      || TopologyFileChanged
      ) {
    XCSoarInterface::CloseProgressDialog();
    XCSoarInterface::main_window.map.set_focus();
  }

  calculation_thread->resume();

  if(DevicePortChanged) {
    devRestart();
  }

  draw_thread->resume();
  // allow map and calculations threads to continue on their merry way
}


void SystemConfiguration(void) {
  if (!is_simulator() && XCSoarInterface::LockSettingsInFlight
      && XCSoarInterface::Basic().Flying) {
    Message::AddMessage(TEXT("Settings locked in flight"));
    return;
  }

  SettingsEnter();
  dlgConfigurationShowModal();
  SettingsLeave();
}

