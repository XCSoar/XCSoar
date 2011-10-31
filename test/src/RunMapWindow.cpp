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

#include "MapWindow/MapWindow.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Init.hpp"
#include "ResourceLoader.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Logger/Logger.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "UtilsSystem.hpp"
#include "Profile/ProfileKeys.hpp"
#include "LocalPath.hpp"
#include "LocalTime.hpp"
#include "WaypointGlue.hpp"
#include "Device/device.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Dialogs/Dialogs.h"
#include "Logger/LoggerImpl.hpp"
#include "Audio/Sound.hpp"
#include "ButtonLabel.hpp"
#include "DeviceBlackboard.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Profile/Profile.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "LogFile.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ConfiguredFile.hpp"
#include "Operation.hpp"
#include "Look/WaypointLook.hpp"
#include "Look/AirspaceLook.hpp"
#include "Look/TaskLook.hpp"
#include "Look/AircraftLook.hpp"
#include "Look/TrafficLook.hpp"
#include "Look/MarkerLook.hpp"

#ifndef _MSC_VER
#include <algorithm>
using std::min;
#endif

#ifdef ENABLE_OPENGL
#include "MapProjectionBlackboard.hpp"
void MapProjectionBlackboard::ReadMapProjection(const MapWindowProjection &) {}
#endif

DeviceBlackboard *device_blackboard;

ProtectedTaskManager *protected_task_manager;
ProtectedAirspaceWarningManager *airspace_warnings;

void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc, const fixed alt) {}

static Waypoints way_points;

static TaskEvents task_events;

static TaskManager task_manager(task_events,
                                way_points);

static Airspaces airspace_database;

static TopographyStore *topography;
static RasterTerrain *terrain;

bool PlayResource(const TCHAR* lpName)
{
  return false;
}

unsigned
TimeLocal(int d)
{
  return d;
}

class TestWindow : public SingleWindow {
public:
  MapWindow map;
  ButtonWindow close_button;

  enum {
    ID_START = 100,
    ID_CLOSE,
  };

public:
  TestWindow(const WaypointLook &waypoint_look,
             const AirspaceLook &airspace_look,
             const TaskLook &task_look,
             const AircraftLook &aircraft_look,
             const TrafficLook &traffic_look,
             const MarkerLook &marker_look)
    :map(waypoint_look, airspace_look,
         task_look, aircraft_look, traffic_look, marker_look) {}

#ifdef USE_GDI
  static bool register_class(HINSTANCE hInstance) {
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Window::WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = _T("RunMapWindow");

    return RegisterClass(&wc);
  }
#endif /* USE_GDI */

  void set(PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height) {
    SingleWindow::set(_T("RunMapWindow"), _T("RunMapWindow"),
                      left, top, width, height);

    PixelRect rc = get_client_rect();
    map.set(*this, rc);
    map.set_way_points(&way_points);
    map.set_airspaces(&airspace_database, NULL);
    map.set_topography(topography);
    map.set_terrain(terrain);
    if (terrain != NULL)
      map.SetLocation(terrain->GetTerrainCenter());

    close_button.set(*this, _T("Close"), ID_CLOSE, 5, 5, 65, 25);
    close_button.set_font(Fonts::Map);
    close_button.bring_to_top();
  }

protected:
  virtual bool on_command(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      close();
      return true;
    }

    return TopWindow::on_command(id, code);
  }
};

static void
SetDefaults(SETTINGS_MAP &settings_map)
{
  settings_map.cruise_orientation = NORTHUP;
  settings_map.circling_orientation = NORTHUP;
  settings_map.waypoint.SetDefaults();
  settings_map.topography_enabled = true;
  settings_map.terrain.SetDefaults();
  settings_map.terrain.enable = true;
  settings_map.terrain.slope_shading = sstFixed;
}

static void
LoadFiles()
{
  NullOperationEnvironment operation;

  topography = new TopographyStore();
  LoadConfiguredTopography(*topography, operation);

  terrain = RasterTerrain::OpenTerrain(NULL, operation);

  WaypointGlue::LoadWaypoints(way_points, terrain, operation);

  TLineReader *reader = OpenConfiguredTextFile(szProfileAirspaceFile);
  if (reader != NULL) {
    AirspaceParser parser(airspace_database);
    parser.Parse(*reader, operation);
    delete reader;

    airspace_database.optimise();
  }
}

static void
GenerateBlackboard(MapWindow &map, const SETTINGS_MAP &settings_map)
{
  MoreData nmea_info;
  DerivedInfo derived_info;
  SETTINGS_COMPUTER settings_computer;

  nmea_info.Reset();
  nmea_info.clock = fixed_one;
  nmea_info.time = fixed(1297230000);
  nmea_info.connected.Update(nmea_info.clock);
  nmea_info.location.latitude = Angle::Degrees(fixed(51.2));
  nmea_info.location.longitude = Angle::Degrees(fixed(7.7));
  nmea_info.location_available.Update(nmea_info.clock);
  nmea_info.track = Angle::Degrees(fixed_90);
  nmea_info.track_available.Update(nmea_info.clock);
  nmea_info.ground_speed = fixed(50);
  nmea_info.ground_speed_available.Update(nmea_info.clock);
  nmea_info.gps_altitude = fixed(1500);
  nmea_info.gps_altitude_available.Update(nmea_info.clock);

  memset(&derived_info, 0, sizeof(derived_info));
  derived_info.terrain_valid = true;

  memset(&settings_computer, 0, sizeof(settings_computer));

  if (terrain != NULL) {
    RasterTerrain::UnprotectedLease lease(*terrain);
    do {
      lease->SetViewCenter(nmea_info.location, fixed(50000));
    } while (lease->IsDirty());
  }

  settings_computer.airspace.SetDefaults();

  map.ReadBlackboard(nmea_info, derived_info, settings_computer,
                     settings_map);
}

#ifndef ENABLE_OPENGL

class DrawThread {
public:
  static void Draw(MapWindow &map) {
    map.repaint();
    map.UpdateAll();
    map.repaint();
  }
};

#endif

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
  InitialiseDataPath();
  Profile::SetFiles(_T(""));
  Profile::Load();

  LoadFiles();

  ScreenGlobalInit screen_init;

#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

#ifdef USE_GDI
  TestWindow::register_class(hInstance);
#endif

  SETTINGS_MAP settings_map;
  settings_map.SetDefaults();
  SetDefaults(settings_map);

  WaypointLook *waypoint_look = new WaypointLook();
  waypoint_look->Initialise(settings_map.waypoint);

  AirspaceLook *airspace_look = new AirspaceLook();
  airspace_look->Initialise(settings_map.airspace);

  TaskLook *task_look = new TaskLook();
  task_look->Initialise();

  AircraftLook *aircraft_look = new AircraftLook();
  aircraft_look->Initialise();

  TrafficLook *traffic_look = new TrafficLook();
  traffic_look->Initialise();

  MarkerLook *marker_look = new MarkerLook();
  marker_look->Initialise();

  TestWindow window(*waypoint_look, *airspace_look, *task_look,
                    *aircraft_look, *traffic_look, *marker_look);
  window.set(0, 0, 640, 480);

  Graphics::Initialise();
  Graphics::InitialiseConfigured(settings_map);

  GenerateBlackboard(window.map, settings_map);
  Fonts::Initialize();
#ifndef ENABLE_OPENGL
  DrawThread::Draw(window.map);
#endif
  window.show();

  window.event_loop();
  window.reset();

  Fonts::Deinitialize();
  Graphics::Deinitialise();

  delete terrain;
  delete topography;
  delete traffic_look;
  delete aircraft_look;
  delete task_look;
  delete airspace_look;
  delete waypoint_look;

  DeinitialiseDataPath();

  return 0;
}
