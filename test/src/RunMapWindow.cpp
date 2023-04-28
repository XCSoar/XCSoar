// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "Profile/Keys.hpp"
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
#include "io/ConfiguredFile.hpp"
#include "io/LineReader.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "thread/Debug.hpp"

void
DeviceBlackboard::SetStartupLocation([[maybe_unused]] const GeoPoint &loc,
                                     [[maybe_unused]] const double alt) noexcept
{
}

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
  void OnResize(PixelSize new_size) noexcept override {
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
  ConsoleOperationEnvironment operation;

  topography = new TopographyStore();
  LoadConfiguredTopography(*topography, operation);

  terrain = RasterTerrain::OpenTerrain(nullptr, operation).release();

  WaypointGlue::LoadWaypoints(way_points, terrain, operation);
  WaypointGlue::SetHome(way_points, terrain, poi_settings, team_code_settings,
                        NULL, false);

  auto reader = OpenConfiguredTextFile(ProfileKeys::AirspaceFile,
                                       Charset::AUTO);
  if (reader) {
    ParseAirspaceFile(airspace_database, *reader, operation);
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
  nmea_info.clock = TimeStamp{FloatDuration{1}};
  nmea_info.time = TimeStamp{FloatDuration{1297230000}};
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
Main(TestMainWindow &main_window)
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
