// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON
#define ENABLE_CMDLINE
#define USAGE "[-WxH] [OPTION]... FILE.xcm"

#include "Fonts.hpp"
#include "Main.hpp"
#include "Math/Angle.hpp"
#include "Operation/Operation.hpp"
#include "ProductName.hpp"
#include "Projection/WindowProjection.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "Version.hpp"
#include "system/Args.hpp"
#include "system/Path.hpp"
#include "system/StandardVersion.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/event/Timer.hpp"
#include "ui/window/PaintWindow.hpp"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include <fmt/format.h>

static constexpr const char canonical_name[] = "RunTerrainRenderer";

static AllocatedPath map_path;
static double radius_m = 25000;
static bool have_lat = false;
static bool have_lon = false;
static double latitude_deg = 0;
static double longitude_deg = 0;
static double step_deg = 3;
static unsigned period_ms = 50;
static unsigned seconds = 0;
static bool cpu_shade [[maybe_unused]] = false;

static void
PrintHelp() noexcept
{
  std::printf(
    "Usage: %s [-WxH] [OPTION]... FILE.xcm\n"
    "\n"
    "Load terrain from a map container and redraw TerrainRenderer\n"
    "with a rotating sun azimuth (OpenGL GPU hillshade by default).\n"
    "\n"
    "Options:\n"
    "  --radius=M     half-width in metres (default: 25000)\n"
    "  --lat=DEG      map centre latitude (default: terrain centre)\n"
    "  --lon=DEG      map centre longitude (default: terrain centre)\n"
    "  --step=DEG     sun azimuth step per tick (default: 3)\n"
    "  --period=MS    timer period (default: 50)\n"
    "  --seconds=N    close after N seconds (for profiling)\n"
    "  --cpu-shade    force CPU GenerateSlopeImage() (A/B vs GPU)\n"
    "  -h, --help     display this help and exit\n"
    "  --version      output version information and exit\n"
    "\n"
    "Report bugs to: <%s>\n"
    "%s home page: <%s>\n",
    canonical_name,
    PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

static bool
ParseUnsignedOption(const char *arg, const char *prefix,
                    unsigned &value) noexcept
{
  if (!StringStartsWith(arg, prefix))
    return false;

  const char *p = arg + std::strlen(prefix);
  char *end = nullptr;
  const unsigned parsed = ParseUnsigned(p, &end);
  if (end == nullptr || *end != '\0' || parsed == 0)
    return false;

  value = parsed;
  return true;
}

static bool
ParseDoubleOption(const char *arg, const char *prefix,
                  double &value, bool positive) noexcept
{
  if (!StringStartsWith(arg, prefix))
    return false;

  const char *p = arg + std::strlen(prefix);
  char *end = nullptr;
  const double parsed = ParseDouble(p, &end);
  if (end == nullptr || *end != '\0')
    return false;
  if (positive && parsed <= 0.)
    return false;

  value = parsed;
  return true;
}

static void
ParseCommandLine(Args &args)
{
  while (!args.IsEmpty()) {
    const char *arg = args.PeekNext();

    if (StringIsEqual(arg, "-h") || StringIsEqual(arg, "--help")) {
      args.Skip();
      PrintHelp();
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--version")) {
      args.Skip();
      PrintStandardVersion(canonical_name, XCSoar_Version);
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--cpu-shade")) {
      args.Skip();
#ifdef ENABLE_OPENGL
      cpu_shade = true;
#else
      std::fprintf(stderr, "%s: --cpu-shade requires OpenGL\n",
                   canonical_name);
      std::exit(EXIT_FAILURE);
#endif
      continue;
    }

    if (ParseDoubleOption(arg, "--radius=", radius_m, true) ||
        ParseDoubleOption(arg, "--step=", step_deg, true) ||
        ParseUnsignedOption(arg, "--period=", period_ms) ||
        ParseUnsignedOption(arg, "--seconds=", seconds)) {
      args.Skip();
      continue;
    }

    double coord = 0;
    if (ParseDoubleOption(arg, "--lat=", coord, false)) {
      args.Skip();
      latitude_deg = coord;
      have_lat = true;
      continue;
    }

    if (ParseDoubleOption(arg, "--lon=", coord, false)) {
      args.Skip();
      longitude_deg = coord;
      have_lon = true;
      continue;
    }

    if (StringStartsWith(arg, "-")) {
      std::fprintf(stderr, "%s: unrecognised or invalid option: %s\n",
                   canonical_name, arg);
      std::exit(EXIT_FAILURE);
    }

    break;
  }

  map_path = args.ExpectNextPath();
}

class TerrainWindow : public PaintWindow {
  RasterTerrain &terrain;
  TerrainRenderer renderer;
  TerrainRendererSettings settings;
  WindowProjection projection;
  GeoPoint location;
  double radius_m;
  Angle sun = Angle::Degrees(-45);
  double last_generate_ms = 0;

public:
  TerrainWindow(RasterTerrain &_terrain, GeoPoint _location,
                double _radius_m) noexcept
    :terrain(_terrain), renderer(_terrain),
     location(_location), radius_m(_radius_m)
  {
    settings.SetDefaults();
    settings.enable = true;
    settings.slope_shading = SlopeShading::SUN;
    settings.contours = Contours::OFF;
    renderer.SetSettings(settings);
#ifdef ENABLE_OPENGL
    renderer.SetQuantisationPixels(1);
    renderer.SetUseCpuHillshade(cpu_shade);
#endif
  }

  void SetSun(Angle _sun) noexcept {
    sun = _sun;
#ifdef ENABLE_OPENGL
    if (cpu_shade)
#endif
      renderer.Flush();
    Invalidate();
  }

  Angle GetSun() const noexcept {
    return sun;
  }

  void LoadTiles() noexcept {
    RebuildProjection();
    const double tile_radius = projection.GetScreenWidthMeters() / 2;
    while (terrain.UpdateTiles(location, tile_radius)) {}
  }

protected:
  void RebuildProjection() noexcept {
    const PixelSize size = GetSize();
    if (size.width < 1 || size.height < 1)
      return;

    projection.SetScreenSize(size);
    projection.SetScaleFromRadius(radius_m);
    projection.SetGeoLocation(location);
    projection.SetScreenOrigin(size.width / 2, size.height / 2);
    projection.UpdateScreenBounds();
  }

  void OnResize(PixelSize new_size) noexcept override {
    PaintWindow::OnResize(new_size);
    LoadTiles();
  }

  void OnPaint(Canvas &canvas) noexcept override {
    canvas.ClearWhite();

    if (!projection.IsValid())
      RebuildProjection();

    renderer.SetSettings(settings);

    const auto t0 = std::chrono::steady_clock::now();
    const bool ok = renderer.Generate(projection, sun);
    last_generate_ms =
      std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    if (ok)
      renderer.Draw(canvas, projection);

    canvas.Select(normal_font);
    canvas.SetTextColor(COLOR_BLACK);
#ifdef ENABLE_OPENGL
    const char *mode = renderer.IsShaderHillshade() ? "GPU" : "CPU";
#else
    const char *mode = "CPU";
#endif
    canvas.DrawText({8, 8},
                    fmt::format("{}  sun {:.0f}°  generate {:.1f} ms",
                                mode, sun.Degrees(), last_generate_ms));
  }
};

static void
Main(TestMainWindow &main_window)
{
  if (have_lat != have_lon) {
    std::fprintf(stderr, "%s: --lat and --lon must be used together\n",
                 canonical_name);
    std::exit(EXIT_FAILURE);
  }

  QuietOperationEnvironment operation;
  std::fprintf(stderr, "loading terrain from %s...\n", map_path.c_str());
  auto terrain = RasterTerrain::OpenTerrain(nullptr, map_path, operation);
  if (terrain == nullptr) {
    std::fprintf(stderr, "%s: no terrain in %s\n",
                 canonical_name, map_path.c_str());
    std::exit(EXIT_FAILURE);
  }

  GeoPoint location = have_lat
    ? GeoPoint(Angle::Degrees(longitude_deg),
               Angle::Degrees(latitude_deg))
    : terrain->GetTerrainCenter();

  std::fprintf(stderr,
               "centre lat=%.5f lon=%.5f radius=%.0f m step=%.1f°\n",
               location.latitude.Degrees(), location.longitude.Degrees(),
               radius_m, step_deg);

  WindowStyle with_border;
  with_border.Border();

  TerrainWindow view(*terrain, location, radius_m);
  view.Create(main_window, main_window.GetClientRect(), with_border);
  main_window.SetFullWindow(view);
  view.LoadTiles();

  UI::PeriodicTimer timer([&view]() {
    view.SetSun(view.GetSun() + Angle::Degrees(step_deg));
  });
  timer.Schedule(std::chrono::milliseconds(period_ms));

  UI::Timer quit_timer{[&main_window]() {
    main_window.Close();
  }};
  if (seconds > 0)
    quit_timer.Schedule(std::chrono::seconds(seconds));

  main_window.RunEventLoop();
}
