/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Screen/Init.hpp"
#include "ResourceLoader.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/ComputerProfile.hpp"
#include "Profile/MapProfile.hpp"
#include "LocalPath.hpp"
#include "LocalTime.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Profile/Profile.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "LogFile.hpp"
#include "IO/ConfiguredFile.hpp"
#include "Operation/Operation.hpp"
#include "Look/MapLook.hpp"
#include "Look/TrafficLook.hpp"
#include "Thread/Debug.hpp"

void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc, const fixed alt) {}

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

unsigned
TimeLocal(int d)
{
  return d;
}

class DrawThread {
public:
#ifndef ENABLE_OPENGL
  static void Draw(MapWindow &map) {
    map.repaint();
    map.UpdateAll();
    map.repaint();
  }
#else
  static void UpdateAll(MapWindow &map) {
    map.UpdateAll();
  }
#endif
};

class TestWindow : public SingleWindow {
public:
  MapWindow map;
  ButtonWindow close_button;

#ifndef ENABLE_OPENGL
  bool initialised;
#endif

  enum {
    ID_START = 100,
    ID_CLOSE,
  };

public:
  TestWindow(const MapLook &map_look,
             const TrafficLook &traffic_look)
    :map(map_look, traffic_look)
#ifndef ENABLE_OPENGL
     , initialised(false)
#endif
  {
  }

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

  void set(PixelRect _rc) {
    TopWindowStyle style;
    style.Resizable();

    SingleWindow::set(_T("RunMapWindow"), _T("RunMapWindow"), _rc, style);

    PixelRect rc = GetClientRect();
    map.set(*this, rc);
    map.SetWaypoints(&way_points);
    map.SetAirspaces(&airspace_database);
    map.SetTopography(topography);
    map.SetTerrain(terrain);
    if (terrain != NULL)
      map.SetLocation(terrain->GetTerrainCenter());

    rc.left = 5;
    rc.top = 5;
    rc.right = rc.left + 60;
    rc.bottom = rc.top + 20;
    close_button.set(*this, _T("Close"), ID_CLOSE, rc);
    close_button.SetFont(Fonts::map);
    close_button.BringToTop();
  }

protected:
  virtual bool OnCommand(unsigned id, unsigned code) {
    switch (id) {
    case ID_CLOSE:
      Close();
      return true;
    }

    return TopWindow::OnCommand(id, code);
  }

  virtual void OnResize(UPixelScalar width, UPixelScalar height) {
    SingleWindow::OnResize(width, height);
    map.Resize(width, height);

#ifndef ENABLE_OPENGL
  if (initialised)
    DrawThread::Draw(map);
#endif
  }
};

static void
LoadFiles(ComputerSettings &settings)
{
  NullOperationEnvironment operation;

  topography = new TopographyStore();
  LoadConfiguredTopography(*topography, operation);

  terrain = RasterTerrain::OpenTerrain(NULL, operation);

  WaypointGlue::LoadWaypoints(way_points, terrain, operation);
  WaypointGlue::SetHome(way_points, terrain, settings, NULL, false);

  std::unique_ptr<TLineReader> reader(OpenConfiguredTextFile(ProfileKeys::AirspaceFile,
                                                             ConvertLineReader::AUTO));
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
  nmea_info.clock = fixed_one;
  nmea_info.time = fixed(1297230000);
  nmea_info.alive.Update(nmea_info.clock);

  if (settings_computer.poi.home_location_available)
    nmea_info.location = settings_computer.poi.home_location;
  else {
    nmea_info.location.latitude = Angle::Degrees(fixed(51.2));
    nmea_info.location.longitude = Angle::Degrees(fixed(7.7));
  }

  nmea_info.location_available.Update(nmea_info.clock);
  nmea_info.track = Angle::Degrees(fixed_90);
  nmea_info.track_available.Update(nmea_info.clock);
  nmea_info.ground_speed = fixed(50);
  nmea_info.ground_speed_available.Update(nmea_info.clock);
  nmea_info.gps_altitude = fixed(1500);
  nmea_info.gps_altitude_available.Update(nmea_info.clock);

  derived_info.Reset();
  derived_info.terrain_valid = true;

  if (terrain != NULL) {
    RasterTerrain::UnprotectedLease lease(*terrain);
    do {
      lease->SetViewCenter(nmea_info.location, fixed(50000));
    } while (lease->IsDirty());
  }

  map.ReadBlackboard(nmea_info, derived_info, settings_computer,
                     settings_map);
  map.SetLocation(nmea_info.location);
}

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

  ComputerSettings settings_computer;
  settings_computer.SetDefaults();
  Profile::Load(settings_computer);

  MapSettings settings_map;
  settings_map.SetDefaults();
  Profile::Load(settings_map);

  LoadFiles(settings_computer);

  ScreenGlobalInit screen_init;

#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

#ifdef USE_GDI
  TestWindow::register_class(hInstance);
#endif

  MapLook *map_look = new MapLook();
  map_look->Initialise(settings_map);

  TrafficLook *traffic_look = new TrafficLook();
  traffic_look->Initialise();

  TestWindow window(*map_look, *traffic_look);
  window.set(PixelRect{0, 0, 640, 480});

  GenerateBlackboard(window.map, settings_computer, settings_map);
  Fonts::Initialize();
#ifdef ENABLE_OPENGL
  DrawThread::UpdateAll(window.map);
#else
  DrawThread::Draw(window.map);
  window.initialised = true;
#endif
  window.Show();

  window.RunEventLoop();
  window.reset();

  Fonts::Deinitialize();

  delete terrain;
  delete topography;
  delete traffic_look;
  delete map_look;

  DeinitialiseDataPath();

  return 0;
}
