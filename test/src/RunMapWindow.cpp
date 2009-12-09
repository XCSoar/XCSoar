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

#include "MapWindow.h"
#include "Screen/TopWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Blank.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Interface.hpp"
#include "InfoBoxLayout.h"
#include "Asset.hpp"
#include "Logger.h"
#include "RasterWeather.h"
#include "RasterTerrain.h"
#include "UtilsSystem.hpp"
#include "UtilsProfile.hpp"
#include "Profile.hpp"
#include "LocalTime.hpp"
#include "LocalPath.hpp"
#include "WayPointList.hpp"
#include "WayPointParser.h"
#include "Task.h"
#include "Logger.h"
#include "wcecompat/ts_string.h"
#include "Device/device.h"
#include "InfoBoxLayout.h"
#include "InputEvents.h"
#include "TopologyStore.h"
#include "Dialogs.h"
#include "Protection.hpp"
#include "Gauge/GaugeCDI.hpp"
#include "LoggerImpl.hpp"
#include "Audio/Sound.hpp"
#include "ButtonLabel.hpp"
#include "DeviceBlackboard.hpp"
#include "AirspaceDatabase.hpp"
#include "AirspaceWarning.h"
#include "Registry.hpp"

#ifndef _MSC_VER
#include <algorithm>
using std::min;
#endif

#if defined(PNA) || defined(FIVV)
int GlobalModelType = MODELTYPE_UNKNOWN;
bool needclipping = false;
#endif

#ifdef HAVE_BLANK
int DisplayTimeOut;
#endif

DeviceBlackboard device_blackboard;

void DeviceBlackboard::SetTrackBearing(double val) {}
void DeviceBlackboard::SetSpeed(double val) {}

void
DeviceBlackboard::SetStartupLocation(const GEOPOINT &loc, const double alt) {}

int dlgWaypointOutOfTerrain(const TCHAR *Message) { return 0; }
int WaypointsOutOfRange;

void Profile::StoreRegistry(void) {}

bool
FileExistsA(const char *FileName)
{
  FILE *file = fopen(FileName, "r");
  if (file != NULL) {
    fclose(file);
    return(true);
  }
  return false;
}

Trigger drawTriggerEvent(TEXT("drawTriggerEvent"),false);
Trigger targetManipEvent(TEXT("targetManip"));

WayPointList way_points;
AirspaceDatabase airspace_database;
static TopologyStore *topology;
static RasterTerrain terrain;
Logger logger;

int InfoBoxLayout::scale = 1;
double InfoBoxLayout::dscale = 1.0;
bool InfoBoxLayout::IntScaleFlag;
bool InfoBoxLayout::landscape = true;
int InfoBoxLayout::ControlWidth;

bool CommonInterface::VirtualKeys=false;
HINSTANCE CommonInterface::hInst;
void XCSoarInterface::InterfaceTimeoutReset(void) {}

void
XCSoarInterface::CreateProgressDialog(const TCHAR* text)
{
  _ftprintf(stderr, _T("%s\n"), text);
}

void XCSoarInterface::StepProgressDialog(void) {}
bool XCSoarInterface::SetProgressStepSize(int nSize) {
  return false;
}

struct DeviceDescriptor DeviceList[NUMDEV];

void InputEvents::ShowMenu() {}
bool InputEvents::processKey(int key) {
  return false;
}

void InputEvents::setMode(mode mode) {}
InputEvents::mode InputEvents::getModeID() { return MODE_DEFAULT; }

GaugeCDI::GaugeCDI(ContainerWindow &parent) {}
void GaugeCDI::Update(double TrackBearing, double WaypointBearing) {}

int
propGetScaleList(double *List, size_t Size)
{
  return 0;
}

WPARAM TranscodeKey(WPARAM wParam) {
  return wParam;
}

bool
PopupNearestWaypointDetails(const WayPointList &way_points,
                            const GEOPOINT &location,
                            double range, bool pan)
{
  return false;
}

bool
PopupInteriorAirspaceDetails(const AirspaceDatabase &airspace_database,
                             const GEOPOINT &location)
{
  return false;
}

bool
ClearAirspaceWarnings(AirspaceDatabase &airspace_database,
                      bool ack, bool allday)
{
  return false;
}

void
AirspaceWarnListClear(AirspaceDatabase &airspace_database)
{
}

Logger::Logger() {}
Logger::~Logger() {}

bool
Logger::isLoggerActive()
{
  return false;
}

bool PlayResource(const TCHAR* lpName)
{
  return false;
}

int WINAPI
MessageBoxX(LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
  return -1;
}

void
StartupStore(const TCHAR *Str, ...)
{
  va_list ap;

  va_start(ap, Str);
  _vftprintf(stderr, Str, ap);
  va_end(ap);
}

const TCHAR *
gettext(const TCHAR *text)
{
  return text;
}

int
TimeLocal(int d)
{
  return d;
}

long
CheckFreeRam(void)
{
  return 64 * 1024 * 1024;
}

bool
Logger::CheckDeclaration(void)
{
  return false;
}

void ButtonLabel::SetFont(const Font &Font) {}

class TestWindow : public TopWindow {
public:
  MapWindow map;

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
#ifdef WINDOWSPC
    wc.cbWndExtra = 0;
#else
    WNDCLASS dc;
    GetClassInfo(hInstance, TEXT("DIALOG"), &dc);
    wc.cbWndExtra = dc.cbWndExtra ;
#endif
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
    TopWindow::set(_T("RunMapWindow"), _T("RunMapWindow"),
                   left, top, width, height);

    RECT rc = get_client_rect();
    map.set(*this, rc, rc);
    map.set_way_points(&way_points);
    map.set_airspaces(&airspace_database);
    map.set_topology(topology);
    map.set_terrain(&terrain);
  }

protected:
  virtual bool on_destroy(void) {
    TopWindow::on_destroy();
    ::PostQuitMessage(0);
    return true;
  }
};

void
ActionInterface::SignalShutdown(bool force)
{
  // XXX
}

void TriggerGPSUpdate() {}

class Blackboard : public SettingsMapBlackboard {
public:
  Blackboard() {
    settings_map.DisplayOrientation = NORTHUP;
    settings_map.DisplayTextType = DISPLAYNAME;
    settings_map.EnableTopology = true;
    settings_map.EnableTerrain = true;
    settings_map.MapScale = 3.0;
  }
};

static Blackboard blackboard;

static void
LoadFiles()
{
  topology = new TopologyStore(NULL);
  topology->Open();

  terrain.OpenTerrain();

  ReadWayPoints(way_points, &terrain);

  TCHAR tpath[MAX_PATH];
  GetRegistryString(szRegistryAirspaceFile, tpath, MAX_PATH);
  if (tpath[0] != 0) {
    ExpandLocalPath(tpath);

    char path[MAX_PATH];
    unicode2ascii(tpath, path, sizeof(path));

    if (!ReadAirspace(airspace_database, path))
      StartupStore(TEXT("No airspace file 1\n"));
  }

  FindAirspaceAreaBounds(airspace_database);
  FindAirspaceCircleBounds(airspace_database);
}

static void
GenerateBlackboard(MapWindow &map)
{
  NMEA_INFO nmea_info;
  DERIVED_INFO derived_info;
  SETTINGS_COMPUTER settings_computer;

  memset(&nmea_info, 0, sizeof(nmea_info));
  nmea_info.Connected = 2;
  nmea_info.SatellitesUsed = 4;
  nmea_info.Location.Latitude = 51.2;
  nmea_info.Location.Longitude = 7.7;
  nmea_info.TrackBearing = 90;
  nmea_info.Speed = 50;
  nmea_info.Altitude = 1500;

  memset(&derived_info, 0, sizeof(derived_info));
  derived_info.TerrainValid = true;

  memset(&settings_computer, 0, sizeof(settings_computer));

  terrain.ServiceFullReload(nmea_info.Location);

  for (unsigned i = 0; i <AIRSPACECLASSCOUNT; ++i)
    settings_computer.iAirspaceMode[i] = 3;

  map.ReadBlackboard(nmea_info, derived_info, settings_computer,
                     blackboard.SettingsMap());
  map.UpdateProjection();
}

class DrawThread {
public:
  static void Draw(MapWindow &map) {
    map.DrawThreadLoop();
    map.SmartBounds(true);
    map.Idle(true);
    for (unsigned i = 0; i < 4; ++i)
      map.Idle(false);
    //while (map.Idle(false)) {};
    map.DrawThreadLoop();
  }
};

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine, int nCmdShow)
#endif
{
  InitSineTable();

  LoadFiles();

#ifdef WIN32
  CommonInterface::hInst = hInstance;

  TestWindow::register_class(hInstance);
  MapWindow::register_class(hInstance);
#endif

#ifndef WIN32
  HINSTANCE hInstance = NULL;
#endif

  MapGfx.Initialise(hInstance, blackboard.SettingsMap());

  TestWindow window;
  GenerateBlackboard(window.map);
  window.set(0, 0, 640, 480);
  DrawThread::Draw(window.map);
  window.show();

  window.event_loop(0);

  return 0;
}
