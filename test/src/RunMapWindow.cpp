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

#define ENABLE_RESOURCE_LOADER
#define ENABLE_PROFILE
#define ENABLE_SCREEN
#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON
#define ENABLE_LOOK
#include "Main.hpp"
#include "MapWindow/MapWindow.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/Loader.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/ComputerProfile.hpp"
#include "Profile/MapProfile.hpp"
#include "Profile/Current.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "LogFile.hpp"
#include "IO/ConfiguredFile.hpp"
#include "IO/LineReader.hpp"
#include "Operation/Operation.hpp"
#include "Thread/Debug.hpp"

void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc, const double alt) {}

#ifndef NDEBUG

bool
InDrawThread()
{
  return InMainThread();
}

#endif

static Waypoints way_points;

static Airspaces airspace_database;

static TopographyStore *topography;
static RasterTerrain *terrain;

class DrawThread {
public:
#ifndef ENABLE_OPENGL
  static void Draw(MapWindow &map) {
    map.Repaint();
    map.UpdateAll();
    map.Repaint();
  }
#else
  static void UpdateAll(MapWindow &map) {
    map.UpdateAll();
  }
#endif
};

class TestMapWindow final : public MapWindow {
public:
#ifndef ENABLE_OPENGL
  bool initialised;
#endif

  TestMapWindow(const MapLook &map_look,
             const TrafficLook &traffic_look)
    :MapWindow(map_look, traffic_look)
#ifndef ENABLE_OPENGL
    , initialised(false)
#endif
  {
  }

  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) override {
    MapWindow::OnResize(new_size);

#ifndef ENABLE_OPENGL
    if (initialised)
      DrawThread::Draw(*this);
#endif
  }
};

static void
LoadFiles(PlacesOfInterestSettings &poi_settings,
          TeamCodeSettings &team_code_settings)
{
  NullOperationEnvironment operation;

  topography = new TopographyStore();
  LoadConfiguredTopography(*topography, operation);

  terrain = RasterTerrain::OpenTerrain(NULL, operation);

  WaypointGlue::LoadWaypoints(way_points, terrain, operation);
  WaypointGlue::SetHome(way_points, terrain, poi_settings, team_code_settings,
                        NULL, false);

  auto reader = OpenConfiguredTextFile(ProfileKeys::AirspaceFile,
                                       Charset::AUTO);
  if (reader) {
    AirspaceParser parser(airspace_database);
    parser.Parse(*reader, operation);
    airspace_database.Optimise();
  }
}

static void
GenerateBlackboard(MapWindow &map, const ComputerSettings &settings_computer,
                   const MapSettings &settings_map)
{
  MoreData nmea_info;
  DerivedInfo derived_info;

  nmea_info.Reset();
  nmea_info.clock = 1;
  nmea_info.time = 1297230000;
  nmea_info.alive.Update(nmea_info.clock);

  if (settings_computer.poi.home_location_available)
    nmea_info.location = settings_computer.poi.home_location;
  else {
    nmea_info.location.latitude = Angle::Degrees(51.2);
    nmea_info.location.longitude = Angle::Degrees(7.7);
  }

  nmea_info.location_available.Update(nmea_info.clock);
  nmea_info.track = Angle::Degrees(90);
  nmea_info.track_available.Update(nmea_info.clock);
  nmea_info.ground_speed = 50;
  nmea_info.ground_speed_available.Update(nmea_info.clock);
  nmea_info.gps_altitude = 1500;
  nmea_info.gps_altitude_available.Update(nmea_info.clock);

  derived_info.Reset();
  derived_info.terrain_valid = true;

  if (terrain != nullptr)
    while (terrain->UpdateTiles(nmea_info.location, 50000)) {}

  map.ReadBlackboard(nmea_info, derived_info, settings_computer,
                     settings_map);
  map.SetLocation(nmea_info.location);
  map.UpdateScreenBounds();
}

void
Main()
{
  ComputerSettings settings_computer;
  settings_computer.SetDefaults();
  Profile::Load(Profile::map, settings_computer);

  MapSettings settings_map;
  settings_map.SetDefaults();
  Profile::Load(Profile::map, settings_map);

  LoadFiles(settings_computer.poi, settings_computer.team_code);

  TestMapWindow map(look->map, look->traffic);
  map.SetWaypoints(&way_points);
  map.SetAirspaces(&airspace_database);
  map.SetTopography(topography);
  map.SetTerrain(terrain);
  if (terrain != nullptr)
    map.SetLocation(terrain->GetTerrainCenter());
  map.Create(main_window, main_window.GetClientRect());
  main_window.SetFullWindow(map);

  GenerateBlackboard(map, settings_computer, settings_map);
#ifdef ENABLE_OPENGL
  DrawThread::UpdateAll(map);
#else
  DrawThread::Draw(map);
  map.initialised = true;
#endif

  main_window.RunEventLoop();

  delete terrain;
  delete topography;
}
