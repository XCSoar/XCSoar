// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_RESOURCE_LOADER
#define ENABLE_PROFILE
#define ENABLE_SCREEN
#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON
#define ENABLE_LOOK
#define ENABLE_CMDLINE
#define USAGE "[OPTION]..."

#include "Airspace/AirspaceGlue.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "LogFile.hpp"
#include "Main.hpp"
#include "MapWindow/MapWindow.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "ProductName.hpp"
#include "Profile/ComputerProfile.hpp"
#include "Profile/Current.hpp"
#include "Profile/Keys.hpp"
#include "Profile/MapProfile.hpp"
#include "Projection/Projection.hpp"
#include "Repository/FileType.hpp"
#include "Terrain/Loader.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/Thread.hpp"
#include "Topography/TopographyGlue.hpp"
#include "Topography/TopographyStore.hpp"
#include "Version.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "io/BufferedReader.hxx"
#include "io/ConfiguredFile.hpp"
#include "io/FileReader.hxx"
#include "system/Args.hpp"
#include "system/StandardVersion.hpp"
#include "thread/Debug.hpp"
#include "ui/event/Notify.hpp"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>

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

static constexpr const char canonical_name[] = "RunMapWindow";

struct Options {
  bool benchmark = false;
};

static Options options;

static Waypoints way_points;

static Airspaces airspace_database;

static TopographyStore *topography;
static RasterTerrain *terrain;

class DrawThread {
public:
#ifndef ENABLE_OPENGL
  /** Match production DrawThread: paint only (no sync UpdateTerrain). */
  static void Draw(MapWindow &map) {
    map.Repaint();
  }
#else
  static void Draw(MapWindow &map) {
    map.Invalidate();
  }
#endif
};

class TestMapWindow final : public MapWindow {
  bool dragging = false;
  PixelPoint drag_start;
  GeoPoint drag_start_geopoint;
  Projection drag_projection;

  std::unique_ptr<TerrainThread> terrain_thread;

  /* Marshal TerrainThread completion onto the UI thread (like
     GlueMapWindow::InjectRedraw). */
  UI::Notify redraw_notify{[this]{ Redraw(); }};

public:
#ifndef ENABLE_OPENGL
  bool initialised = false;
#endif

  TestMapWindow(const MapLook &map_look,
                const TrafficLook &traffic_look) noexcept
    :MapWindow(map_look, traffic_look)
  {
  }

  ~TestMapWindow() noexcept override {
    StopTerrainThread();
  }

  void SetTerrainThreaded(RasterTerrain *_terrain) noexcept {
    StopTerrainThread();
    MapWindow::SetTerrain(_terrain);
    if (_terrain != nullptr)
      terrain_thread = std::make_unique<TerrainThread>(
        *_terrain,
        [this](){ redraw_notify.SendNotification(); });
  }

  void Redraw() noexcept {
#ifdef ENABLE_OPENGL
    Invalidate();
#else
    if (initialised)
      DrawThread::Draw(*this);
#endif
  }

  /**
   * Like GlueMapWindow::UpdateScreenBounds(): update bounds and wake
   * the TerrainThread (no synchronous UpdateTiles).
   */
  void NotifyProjection() noexcept {
    UpdateScreenBounds();
    if (terrain_thread != nullptr && VisibleProjection().IsValid())
      terrain_thread->Trigger(VisibleProjection());
  }

  void WaitTerrain() noexcept {
    if (terrain_thread != nullptr)
      terrain_thread->WaitDone();
  }

  void SetScaleRefresh(double scale) noexcept {
    SetMapScale(scale);
    NotifyProjection();
    WaitTerrain();
    Redraw();
  }

  void PanPixels(PixelPoint delta) noexcept {
    const auto &proj = VisibleProjection();
    const auto c = proj.GetScreenCenter();
    SetLocation(proj.GetGeoLocation()
                + proj.ScreenToGeo(c)
                - proj.ScreenToGeo(c + delta));
    NotifyProjection();
    Redraw();
  }

  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override {
    MapWindow::OnResize(new_size);
    if (terrain_thread != nullptr && VisibleProjection().IsValid())
      terrain_thread->Trigger(VisibleProjection());
    Redraw();
  }

  bool OnMouseDown(PixelPoint p) noexcept override {
    if (!VisibleProjection().IsValid())
      return false;

    dragging = true;
    drag_start = p;
    drag_start_geopoint = VisibleProjection().ScreenToGeo(p);
    drag_projection = VisibleProjection();
    SetCapture();
    return true;
  }

  bool OnMouseUp([[maybe_unused]] PixelPoint p) noexcept override {
    if (!dragging)
      return false;

    dragging = false;
    ReleaseCapture();

    WaitTerrain();
    Redraw();
    return true;
  }

  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override {
    if (!dragging)
      return MapWindow::OnMouseMove(p, keys);

    SetLocation(drag_projection.GetGeoLocation()
                + drag_start_geopoint
                - drag_projection.ScreenToGeo(p));
    NotifyProjection();
    Redraw();
    return true;
  }

  bool OnMouseWheel([[maybe_unused]] PixelPoint p, int delta) noexcept override {
    if (dragging || !VisibleProjection().IsValid())
      return true;

    const double scale = VisibleProjection().GetMapScale();
    if (delta > 0)
      SetMapScale(scale / 1.5);
    else if (delta < 0)
      SetMapScale(scale * 1.5);

    NotifyProjection();
    WaitTerrain();
    Redraw();
    return true;
  }

private:
  void StopTerrainThread() noexcept {
    if (terrain_thread != nullptr) {
      terrain_thread->LockStop();
      terrain_thread.reset();
    }
  }
};

static void
PrintStandardHelp() noexcept
{
  std::printf(
    "Usage: %s [OPTION]...\n"
    "\n"
    "Show a map window using the current profile (terrain, waypoints,\n"
    "airspace). Drag to pan; mouse wheel to zoom.  Terrain tiles load via\n"
    "TerrainThread (same path as GlueMapWindow).\n"
    "\n"
    "Options:\n"
    "  --benchmark           run a fixed zoom/pan route, print timings, exit\n"
    "  -WxH                  window size; must be first (e.g. -800x600)\n"
    "  -h, --help            display this help and exit\n"
    "  --version             output version information and exit\n"
    "\n"
    "Report bugs to: <%s>\n"
    "%s home page: <%s>\n",
    canonical_name, PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

static void
ParseCommandLine(Args &args)
{
  while (!args.IsEmpty()) {
    const char *arg = args.PeekNext();
    if (arg == nullptr || arg[0] != '-')
      break;

    if (StringIsEqual(arg, "-h") || StringIsEqual(arg, "--help")) {
      PrintStandardHelp();
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--version")) {
      PrintStandardVersion(canonical_name, XCSoar_Version);
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--benchmark")) {
      args.GetNext();
      options.benchmark = true;
      continue;
    }

    break;
  }
}

static long
Ms(const std::chrono::steady_clock::duration d) noexcept
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

static void
PanAtScale(TestMapWindow &map, double scale,
           unsigned steps, PixelPoint step,
           const char *label) noexcept
{
  using clock = std::chrono::steady_clock;

  const auto t0 = clock::now();
  map.SetScaleRefresh(scale);
  const auto t1 = clock::now();

  for (unsigned i = 0; i < steps; ++i) {
    /* Oscillate so the view stays inside the terrain overscan buffer
       (unidirectional pan exits a 1.5–2× buffer within a few steps). */
    const int x = (i & 2) != 0 ? -step.x : step.x;
    const int y = (i & 1) != 0 ? step.y : -step.y;
    map.PanPixels({x, y});
    map.WaitTerrain();
  }
  const auto t2 = clock::now();

  std::printf("%s: first %ld ms, pan %ld ms, total %ld ms "
              "(%u steps @ %.0f km)\n",
              label, Ms(t1 - t0), Ms(t2 - t1), Ms(t2 - t0),
              steps, scale / 1000.);
}

static void
RunBenchmark(TestMapWindow &map) noexcept
{
  using clock = std::chrono::steady_clock;

  const auto t0 = clock::now();

  /* Wide scales first so TerrainThread skip/throttle is visible before
     fine tiles are warmed by closer zooms. */
  PanAtScale(map, 300000, 24, {80, 40}, "pan_300km");
  PanAtScale(map, 150000, 24, {80, 40}, "pan_150km");
  PanAtScale(map, 100000, 24, {80, 40}, "pan_100km");
  PanAtScale(map, 50000, 24, {80, 40}, "pan_50km");
  PanAtScale(map, 10000, 40, {40, 20}, "pan_10km");

  std::printf("total: %ld ms\n", Ms(clock::now() - t0));
  std::fflush(stdout);
}

static void
LoadFiles(PlacesOfInterestSettings &poi_settings,
          TeamCodeSettings &team_code_settings)
{
  ConsoleOperationEnvironment operation;

  topography = new TopographyStore();
  LoadConfiguredTopography(*topography);

  terrain = RasterTerrain::OpenTerrain(nullptr, operation).release();

  WaypointGlue::LoadWaypoints(way_points, terrain, operation);
  WaypointGlue::SetHome(way_points, terrain, poi_settings, team_code_settings,
                        NULL, false);

  const auto paths = Profile::GetMultiplePaths(ProfileKeys::AirspaceFileList,
                                               GetFileTypePatterns(FileType::AIRSPACE));
  for (auto it = paths.begin(); it < paths.end(); it++) {
    ParseAirspaceFile(airspace_database, *it, operation);
  }

  airspace_database.Optimise();
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

  /* Warm fine tiles only for interactive mode; benchmark starts cold
     at 300 km so TerrainThread behaviour is measurable. */
  if (!options.benchmark && terrain != nullptr)
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
  map.SetTerrainThreaded(terrain);
  if (terrain != nullptr)
    map.SetLocation(terrain->GetTerrainCenter());
  map.Create(main_window, main_window.GetClientRect());
  main_window.SetFullWindow(map);

  GenerateBlackboard(map, settings_computer, settings_map);
#ifndef ENABLE_OPENGL
  map.initialised = true;
#endif

  if (options.benchmark) {
    /* Start cold at 300 km so fine-tile Trigger() is skipped until
       the closer phases — matches production TerrainThread gating. */
    map.SetMapScale(300000);
    map.NotifyProjection();
    DrawThread::Draw(map);
    RunBenchmark(map);
  } else {
    map.NotifyProjection();
    map.WaitTerrain();
    DrawThread::Draw(map);
    main_window.RunEventLoop();
  }

  delete terrain;
  delete topography;
}
