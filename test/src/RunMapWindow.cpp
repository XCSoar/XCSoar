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

#include "MapWindow.hpp"
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
#include "LocalTime.hpp"
#include "WayPointGlue.hpp"
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

#ifndef _MSC_VER
#include <algorithm>
using std::min;
#endif

#ifdef ENABLE_OPENGL
Mutex mutexBlackboard;
#include "MapProjectionBlackboard.hpp"
void MapProjectionBlackboard::ReadMapProjection(const MapWindowProjection &) {}
#endif

DeviceBlackboard device_blackboard;

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

int
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
  TestWindow() {}

  static bool register_class(HINSTANCE hInstance) {
#ifdef ENABLE_SDL
    return true;
#else /* !ENABLE_SDL */
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
#endif /* !ENABLE_SDL */
  }

  void set(int left, int top, unsigned width, unsigned height) {
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

class Blackboard : public SettingsMapBlackboard {
public:
  Blackboard() {
    settings_map.OrientationCruise = NORTHUP;
    settings_map.OrientationCircling = NORTHUP;
    settings_map.DisplayTextType = DISPLAYNAME;
    settings_map.EnableTopography = true;
    settings_map.EnableTerrain = true;
    settings_map.SlopeShadingType = sstFixed;
  }
};

static Blackboard blackboard;

static void
LoadFiles()
{
  topography = new TopographyStore();
  LoadConfiguredTopography(*topography);

  terrain = RasterTerrain::OpenTerrain(NULL);

  WayPointGlue::ReadWaypoints(way_points, terrain);

  TLineReader *reader = OpenConfiguredTextFile(szProfileAirspaceFile);
  if (reader != NULL) {
    ReadAirspace(airspace_database, *reader);
    delete reader;

    airspace_database.optimise();
  }
}

static void
GenerateBlackboard(MapWindow &map)
{
  NMEA_INFO nmea_info;
  DERIVED_INFO derived_info;
  SETTINGS_COMPUTER settings_computer;

  nmea_info.reset();
  nmea_info.Time = fixed(1297230000);
  nmea_info.Connected.update(nmea_info.Time);
  nmea_info.gps.SatellitesUsed = 4;
  nmea_info.Location.Latitude = Angle::degrees(fixed(51.2));
  nmea_info.Location.Longitude = Angle::degrees(fixed(7.7));
  nmea_info.LocationAvailable.update(nmea_info.Time);
  nmea_info.TrackBearing = Angle::degrees(fixed_90);
  nmea_info.TrackBearingAvailable.update(nmea_info.Time);
  nmea_info.GroundSpeed = fixed(50);
  nmea_info.GroundSpeedAvailable.update(nmea_info.Time);
  nmea_info.GPSAltitude = fixed(1500);
  nmea_info.GPSAltitudeAvailable.update(nmea_info.Time);

  memset(&derived_info, 0, sizeof(derived_info));
  derived_info.TerrainValid = true;

  memset(&settings_computer, 0, sizeof(settings_computer));

  if (terrain != NULL) {
    RasterTerrain::UnprotectedLease lease(*terrain);
    do {
      lease->SetViewCenter(nmea_info.Location, fixed(50000));
    } while (lease->IsDirty());
  }

  std::fill(settings_computer.DisplayAirspaces,
            settings_computer.DisplayAirspaces + AIRSPACECLASSCOUNT,
            true);

  std::fill(settings_computer.airspace_warnings.class_warnings,
            settings_computer.airspace_warnings.class_warnings + AIRSPACECLASSCOUNT,
            true);

  map.ReadBlackboard(nmea_info, derived_info, settings_computer,
                     blackboard.SettingsMap());
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
  Profile::SetFiles(_T(""));
  Profile::Load();

  LoadFiles();

  ScreenGlobalInit screen_init;

#ifdef WIN32
  ResourceLoader::Init(hInstance);

  TestWindow::register_class(hInstance);
  PaintWindow::register_class(hInstance);
#endif

  TestWindow window;
  window.set(0, 0, 640, 480);

  Graphics::Initialise();
  Graphics::InitialiseConfigured(blackboard.SettingsMap());

  GenerateBlackboard(window.map);
  Fonts::Initialize();
#ifndef ENABLE_OPENGL
  DrawThread::Draw(window.map);
#endif
  window.show();

  window.event_loop();
  window.reset();

  Fonts::Deinitialize();

  delete terrain;
  delete topography;
  WayPointGlue::Close();

  return 0;
}
