// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/AirspaceParser.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Geo/GeoVector.hpp"
#include "Look/AirspaceLook.hpp"
#include "Look/TopographyLook.hpp"
#include "Operation/Operation.hpp"
#include "ProductName.hpp"
#include "Projection/WindowProjection.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Screen/Layout.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Terrain/TerrainRenderer.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "Topography/CachedTopographyRenderer.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/TopographyStore.hpp"
#include "Version.hpp"
#include "io/BufferedReader.hxx"
#include "io/FileReader.hxx"
#include "io/ZipArchive.hpp"
#include "io/ZipLineReader.hpp"
#include "system/Args.hpp"
#include "system/Path.hpp"
#include "system/StandardVersion.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#include "ui/window/Init.hpp"
#include "util/NumberParser.hpp"
#include "util/PrintException.hxx"
#include "util/StringCompare.hxx"

#include "Fonts.hpp"

#ifdef ENABLE_OPENGL
#include "ui/opengl/System.hpp"
#endif

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

static constexpr const char canonical_name[] = "RunMapRendererStress";

static constexpr double kDefaultRadiiM[] = {
  750.,
  3000.,
  19000.,
  50000.,
  150000.,
};

struct Options {
  unsigned draws_per_sample = 5;
  unsigned width = 800;
  unsigned height = 600;
  unsigned dpi = 130;
  bool draw_terrain = true;
  bool draw_topo = true;
  bool draw_labels = true;
  bool draw_airspace = true;
  bool no_cache = false;
  bool progress = true;
  bool layers = false;
  bool have_lat = false;
  bool have_lon = false;
  double latitude_deg = 0;
  double longitude_deg = 0;
  double pan_m = 0;
  std::vector<AllocatedPath> airspace_files;
  std::vector<double> radii_m;
};

static Options options;

static void
PrintStandardHelp() noexcept
{
  std::printf(
    "Usage: %s [OPTION]... FILE.xcm\n"
    "\n"
    "Load a map container and benchmark MapWindow-style terrain,\n"
    "topography, and optional OpenAir airspace draw cost.\n"
    "Shape loading (ScanVisibility) is timed separately from warm\n"
    "redraws so pan/zoom I/O is not mixed with steady-state paint.\n"
    "\n"
    "Options:\n"
    "  --draws=N             warm redraws per radius (default: 5)\n"
    "  --width=PIXELS        canvas width (default: 800; Cubie2-style)\n"
    "  --height=PIXELS       canvas height (default: 600; try 768 for\n"
    "                        1024x768)\n"
    "  --dpi=N               Layout DPI (default: 130)\n"
    "  --lat=DEG             map centre latitude (default: terrain centre)\n"
    "  --lon=DEG             map centre longitude (default: terrain centre)\n"
    "  --radius=M            half-width in metres (repeatable; default:\n"
    "                        750, 3000, 19000, 50000, 150000)\n"
    "  --pan-m=M             shift the map east by M metres before each\n"
    "                        warm redraw (0 = static; try 50 with\n"
    "                        --draws=60 for a 60 Hz pan)\n"
    "  --airspace=FILE       load OpenAir (repeatable)\n"
    "  --no-airspace         do not load or paint airspace\n"
    "  --no-terrain          skip DEM load and terrain paint\n"
    "  --no-topo             skip topography load and paint\n"
    "  --no-labels           skip topography label paint\n"
    "  --no-cache            flush renderer caches before each timed\n"
    "                        draw (software rasterize, not blit)\n"
    "  --layers              print per-layer shape counts on stderr\n"
    "  --no-progress         do not print progress on stderr\n"
    "  -h, --help            display this help and exit\n"
    "  --version             output version information and exit\n"
    "\n"
    "Example:\n"
    "  %s ~/.xcsoar/maps/ALPS_Test.xcm\n"
    "  %s --width=1024 --height=768 --airspace=fr.txt --pan-m=80 --draws=60 map.xcm\n"
    "\n"
    "Report bugs to: <%s>\n"
    "%s home page: <%s>\n",
    canonical_name, canonical_name, canonical_name,
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
ParseCommandLine(Args &args) noexcept
{
  while (!args.IsEmpty()) {
    const char *arg = args.PeekNext();

    if (StringIsEqual(arg, "-h") || StringIsEqual(arg, "--help")) {
      args.Skip();
      PrintStandardHelp();
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--version")) {
      args.Skip();
      PrintStandardVersion(canonical_name, XCSoar_Version);
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(arg, "--no-airspace")) {
      args.Skip();
      options.draw_airspace = false;
      continue;
    }

    if (StringStartsWith(arg, "--airspace=")) {
      const char *p = arg + std::strlen("--airspace=");
      if (*p == '\0') {
        std::fprintf(stderr, "%s: --airspace= needs a path\n",
                     canonical_name);
        std::exit(EXIT_FAILURE);
      }
      args.Skip();
      options.airspace_files.emplace_back(Path{p});
      continue;
    }

    if (StringIsEqual(arg, "--no-terrain")) {
      args.Skip();
      options.draw_terrain = false;
      continue;
    }

    if (StringIsEqual(arg, "--no-topo")) {
      args.Skip();
      options.draw_topo = false;
      continue;
    }

    if (StringIsEqual(arg, "--no-labels")) {
      args.Skip();
      options.draw_labels = false;
      continue;
    }

    if (StringIsEqual(arg, "--no-cache")) {
      args.Skip();
      options.no_cache = true;
      continue;
    }

    if (StringIsEqual(arg, "--layers")) {
      args.Skip();
      options.layers = true;
      continue;
    }

    if (StringIsEqual(arg, "--no-progress")) {
      args.Skip();
      options.progress = false;
      continue;
    }

    if (ParseUnsignedOption(arg, "--draws=", options.draws_per_sample) ||
        ParseUnsignedOption(arg, "--width=", options.width) ||
        ParseUnsignedOption(arg, "--height=", options.height) ||
        ParseUnsignedOption(arg, "--dpi=", options.dpi)) {
      args.Skip();
      continue;
    }

    double radius = 0;
    if (ParseDoubleOption(arg, "--radius=", radius, true)) {
      args.Skip();
      options.radii_m.push_back(radius);
      continue;
    }

    double coord = 0;
    if (ParseDoubleOption(arg, "--lat=", coord, false)) {
      args.Skip();
      options.latitude_deg = coord;
      options.have_lat = true;
      continue;
    }

    if (ParseDoubleOption(arg, "--lon=", coord, false)) {
      args.Skip();
      options.longitude_deg = coord;
      options.have_lon = true;
      continue;
    }

    double pan = 0;
    if (ParseDoubleOption(arg, "--pan-m=", pan, true)) {
      args.Skip();
      options.pan_m = pan;
      continue;
    }

    if (StringStartsWith(arg, "--")) {
      std::fprintf(stderr, "%s: unrecognised or invalid option: %s\n",
                   canonical_name, arg);
      std::exit(EXIT_FAILURE);
    }

    break;
  }
}

static void
FlushGL() noexcept
{
#ifdef ENABLE_OPENGL
  glFinish();
#endif
}

static WindowProjection
MakeProjection(const GeoPoint &location, double radius_m,
               PixelSize screen_size) noexcept
{
  WindowProjection projection;
  projection.SetScreenSize(screen_size);
  projection.SetScaleFromRadius(radius_m);
  projection.SetGeoLocation(location);
  projection.SetScreenOrigin(screen_size.width / 2,
                             screen_size.height / 2);
  projection.UpdateScreenBounds();
  return projection;
}

struct VisibilityStats {
  unsigned files = 0;
  unsigned shapes = 0;
};

static VisibilityStats
CountVisible(const TopographyStore &store, double map_scale) noexcept
{
  VisibilityStats stats;
  for (const auto &file : store) {
    if (!file.IsVisible(map_scale))
      continue;

    ++stats.files;
    const std::lock_guard lock{file.mutex};
    for ([[maybe_unused]] const auto &shape : file)
      ++stats.shapes;
  }

  return stats;
}

static unsigned
CountStoreFiles(const TopographyStore &store) noexcept
{
  unsigned n = 0;
  for ([[maybe_unused]] const auto &file : store)
    ++n;
  return n;
}

static void
PrintLayers(const TopographyStore &store, double map_scale) noexcept
{
  unsigned i = 0;
  for (const auto &file : store) {
    unsigned shapes = 0;
    {
      const std::lock_guard lock{file.mutex};
      for ([[maybe_unused]] const auto &shape : file)
        ++shapes;
    }

    std::fprintf(stderr,
                 "  layer %u: visible=%d labels=%d shapes=%u\n",
                 i,
                 file.IsVisible(map_scale) ? 1 : 0,
                 file.IsLabelVisible(map_scale) ? 1 : 0,
                 shapes);
    ++i;
  }
}

static double
ElapsedMs(std::chrono::steady_clock::time_point t0,
          std::chrono::steady_clock::time_point t1) noexcept
{
  return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static void
MaybeFlushCaches(CachedTopographyRenderer *topo,
                 TerrainRenderer *terrain_renderer,
                 AirspaceRenderer *airspace_renderer) noexcept
{
  if (!options.no_cache)
    return;

  if (topo != nullptr)
    topo->Flush();
  if (terrain_renderer != nullptr)
    terrain_renderer->Flush();
  if (airspace_renderer != nullptr)
    airspace_renderer->Flush();
}

static void
DrawTerrain(Canvas &canvas, TerrainRenderer *terrain_renderer,
            const TerrainRendererSettings *terrain_settings,
            const WindowProjection &projection,
            Angle sun_azimuth) noexcept
{
  if (terrain_renderer == nullptr || terrain_settings == nullptr)
    return;

  MaybeFlushCaches(nullptr, terrain_renderer, nullptr);
  terrain_renderer->SetSettings(*terrain_settings);
  if (terrain_renderer->Generate(projection, sun_azimuth))
    terrain_renderer->Draw(canvas, projection);
}

static void
DrawTopography(Canvas &canvas, CachedTopographyRenderer *topo,
               const WindowProjection &projection) noexcept
{
  if (topo == nullptr)
    return;

  MaybeFlushCaches(topo, nullptr, nullptr);
  topo->Draw(canvas, projection);
}

static void
DrawLabels(Canvas &canvas, CachedTopographyRenderer *topo,
           const WindowProjection &projection) noexcept
{
  if (topo == nullptr)
    return;

  LabelBlock label_block;
  topo->DrawLabels(canvas, projection, label_block);
}

static void
DrawAirspace(Canvas &canvas, AirspaceRenderer *airspace_renderer,
#ifndef ENABLE_OPENGL
             Canvas *stencil_canvas,
#endif
             const AirspaceRendererSettings *airspace_settings,
             const WindowProjection &projection) noexcept
{
  if (airspace_renderer == nullptr || airspace_settings == nullptr)
    return;

  MaybeFlushCaches(nullptr, nullptr, airspace_renderer);
  airspace_renderer->Draw(canvas,
#ifndef ENABLE_OPENGL
                          *stencil_canvas,
#endif
                          projection, *airspace_settings);
}

static void
DrawFrame(Canvas &canvas, TerrainRenderer *terrain_renderer,
          const TerrainRendererSettings *terrain_settings,
          CachedTopographyRenderer *topo, bool draw_labels,
          AirspaceRenderer *airspace_renderer,
#ifndef ENABLE_OPENGL
          Canvas *stencil_canvas,
#endif
          const AirspaceRendererSettings *airspace_settings,
          const WindowProjection &projection,
          Angle sun_azimuth) noexcept
{
  canvas.ClearWhite();
  DrawTerrain(canvas, terrain_renderer, terrain_settings,
              projection, sun_azimuth);
  DrawTopography(canvas, topo, projection);
  DrawAirspace(canvas, airspace_renderer,
#ifndef ENABLE_OPENGL
               stencil_canvas,
#endif
               airspace_settings, projection);
  if (draw_labels)
    DrawLabels(canvas, topo, projection);
}

template<typename Fn>
static double
TimeDrawsMs(unsigned draws, Fn &&fn) noexcept
{
  if (draws == 0)
    return 0.;

  FlushGL();
  const auto t0 = std::chrono::steady_clock::now();
  for (unsigned i = 0; i < draws; ++i)
    fn();
  FlushGL();
  return ElapsedMs(t0, std::chrono::steady_clock::now());
}

static unsigned
ScanAll(TopographyStore &store,
        const WindowProjection &projection) noexcept
{
  unsigned total = 0;
  unsigned n;
  while ((n = store.ScanVisibility(projection, 1024,
                                   Layout::Scale(1u))) > 0)
    total += n;
  return total;
}

static void
PrintSampleHeader() noexcept
{
  std::puts("radius_m\tmap_scale\tfiles\tshapes\t"
            "scan_ms\ttiles_ms\tfirst_ms\tframe_ms\t"
            "terrain_ms\ttopo_ms\tairspace_ms\tlabels_ms");
}

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE.xcm");
  ParseCommandLine(args);

  const auto map_path = args.ExpectNextPath();
  args.ExpectEnd();

  if (options.have_lat != options.have_lon) {
    std::fprintf(stderr, "%s: --lat and --lon must be used together\n",
                 canonical_name);
    return EXIT_FAILURE;
  }

  const bool want_airspace = options.draw_airspace &&
    !options.airspace_files.empty();
  if (!options.draw_topo && !options.draw_terrain && !want_airspace) {
    std::fprintf(stderr, "%s: nothing to benchmark\n", canonical_name);
    return EXIT_FAILURE;
  }

  if (options.radii_m.empty()) {
    for (double radius : kDefaultRadiiM)
      options.radii_m.push_back(radius);
  }

  const PixelSize screen_size{int(options.width), int(options.height)};

  ScreenGlobalInit screen_init;
  Layout::Initialise(screen_init.GetDisplay(), screen_size,
                     100, options.dpi);
  InitialiseFonts();

  BufferCanvas canvas;
  canvas.Create(screen_size);
  canvas.Begin();
  canvas.ClearWhite();

  using clock = std::chrono::steady_clock;

  ZipArchive archive(map_path);
  TopographyStore topography;
  unsigned topo_files = 0;

  if (options.draw_topo) {
    if (options.progress)
      std::fprintf(stderr, "loading topography from %s...\n",
                   map_path.c_str());

    const auto t0 = clock::now();
    ZipLineReaderA reader(archive.get(), "topology.tpl");
    topography.Load(reader, nullptr, archive.get());
    topo_files = CountStoreFiles(topography);
    if (options.progress)
      std::fprintf(stderr, "loaded %u topography layers in %.1f ms\n",
                   topo_files, ElapsedMs(t0, clock::now()));
  }

  TopographyLook topo_look;
  topo_look.Initialise();
  std::unique_ptr<CachedTopographyRenderer> topo_renderer;
  if (options.draw_topo)
    topo_renderer =
      std::make_unique<CachedTopographyRenderer>(topography, topo_look);

  std::unique_ptr<RasterTerrain> terrain;
  std::unique_ptr<TerrainRenderer> terrain_renderer;
  TerrainRendererSettings terrain_settings;
  terrain_settings.SetDefaults();
  terrain_settings.slope_shading = SlopeShading::FIXED;

  if (options.draw_terrain) {
    if (options.progress)
      std::fprintf(stderr, "loading terrain from %s...\n",
                   map_path.c_str());

    QuietOperationEnvironment operation;
    const auto t0 = clock::now();
    try {
      terrain = RasterTerrain::OpenTerrain(nullptr, map_path, operation);
      terrain_renderer = std::make_unique<TerrainRenderer>(*terrain);
#ifdef ENABLE_OPENGL
      terrain_renderer->SetQuantisationPixels(1);
#endif
      if (options.progress)
        std::fprintf(stderr, "loaded terrain overview in %.1f ms\n",
                     ElapsedMs(t0, clock::now()));
    } catch (...) {
      PrintException(std::current_exception());
      std::fprintf(stderr, "%s: continuing without terrain\n",
                   canonical_name);
      terrain.reset();
      terrain_renderer.reset();
    }
  }

  Airspaces airspace_database;
  AirspaceLook airspace_look;
  AirspaceRendererSettings airspace_settings;
  std::unique_ptr<AirspaceRenderer> airspace_renderer;
#ifndef ENABLE_OPENGL
  BufferCanvas stencil_canvas;
#endif

  if (want_airspace) {
    if (options.progress)
      std::fprintf(stderr, "loading airspace...\n");

    airspace_settings.SetDefaults();
    airspace_look.Initialise(airspace_settings, bold_font);

    const auto t0 = clock::now();
    for (const auto &path : options.airspace_files) {
    try {
      FileReader file_reader{path};
      BufferedReader buffered_reader{file_reader};
      ParseAirspaceFile(airspace_database, buffered_reader);
    } catch (...) {
      PrintException(std::current_exception());
      std::fprintf(stderr, "%s: failed to parse %s\n",
                   canonical_name, path.c_str());
      DeinitialiseFonts();
      return EXIT_FAILURE;
    }
    }
    airspace_database.Optimise();
    airspace_renderer =
      std::make_unique<AirspaceRenderer>(airspace_look);
    airspace_renderer->SetAirspaces(&airspace_database);
#ifndef ENABLE_OPENGL
    stencil_canvas.Create(screen_size);
#endif
    if (options.progress)
      std::fprintf(stderr, "loaded %u airspaces in %.1f ms\n",
                   airspace_database.GetSize(),
                   ElapsedMs(t0, clock::now()));
  }

  GeoPoint location = GeoPoint::Invalid();
  if (options.have_lat)
    location = GeoPoint(Angle::Degrees(options.longitude_deg),
                        Angle::Degrees(options.latitude_deg));
  else if (terrain)
    location = terrain->GetTerrainCenter();
  else {
    for (const auto &file : topography) {
      location = file.GetCenter();
      break;
    }
  }

  if (!location.IsValid()) {
    std::fprintf(stderr, "%s: no map centre (pass --lat/--lon)\n",
                 canonical_name);
    return EXIT_FAILURE;
  }

  if (options.progress) {
    const char *topo_bench = std::getenv("XCSOAR_BENCH_TOPO");
    const char *as_bench = std::getenv("XCSOAR_BENCH_AS");
    std::fprintf(stderr, "bench topo=%s airspace=%s\n",
                 topo_bench != nullptr && *topo_bench != '\0'
                   ? topo_bench : "full",
                 as_bench != nullptr && *as_bench != '\0'
                   ? as_bench : "geo");
    std::fprintf(stderr,
                 "centre lat=%.5f lon=%.5f canvas=%ux%u dpi=%u%s"
                 " pan_m=%.0f\n",
                 location.latitude.Degrees(),
                 location.longitude.Degrees(),
                 options.width, options.height, options.dpi,
                 options.no_cache ? " no-cache" : "",
                 options.pan_m);
  }

  const Angle sun_azimuth = Angle::Degrees(-45);
  CachedTopographyRenderer *topo = topo_renderer.get();
  TerrainRenderer *tr = terrain_renderer.get();
  const TerrainRendererSettings *ts =
    terrain_renderer ? &terrain_settings : nullptr;
  AirspaceRenderer *asr = airspace_renderer.get();
  const AirspaceRendererSettings *as =
    airspace_renderer ? &airspace_settings : nullptr;

  PrintSampleHeader();

  for (double radius_m : options.radii_m) {
    const auto projection =
      MakeProjection(location, radius_m, screen_size);
    const double map_scale = projection.GetMapScale();

    if (options.progress)
      std::fprintf(stderr,
                   "scanning radius=%.0f m (map_scale=%.0f)...\n",
                   radius_m, map_scale);

    double scan_ms = 0;
    if (options.draw_topo) {
      const auto t0 = clock::now();
      ScanAll(topography, projection);
      scan_ms = ElapsedMs(t0, clock::now());
    }

    double tiles_ms = 0;
    if (terrain) {
      const auto t0 = clock::now();
      const auto tile_radius =
        projection.GetScreenWidthMeters() / 2;
      while (terrain->UpdateTiles(location, tile_radius)) {}
      tiles_ms = ElapsedMs(t0, clock::now());
    }

    const auto visible = CountVisible(topography, map_scale);
    if (options.layers)
      PrintLayers(topography, map_scale);

    FlushGL();
    const auto t_first0 = clock::now();
    DrawFrame(canvas, tr, ts, topo, options.draw_labels,
              asr,
#ifndef ENABLE_OPENGL
              airspace_renderer ? &stencil_canvas : nullptr,
#endif
              as, projection, sun_azimuth);
    FlushGL();
    const double first_ms = ElapsedMs(t_first0, clock::now());

    const unsigned draws = options.draws_per_sample;
    GeoPoint pan_loc = location;
    const double frame_ms = TimeDrawsMs(draws, [&]() {
      WindowProjection frame_projection = projection;
      if (options.pan_m > 0.) {
        pan_loc = GeoVector(options.pan_m, Angle::Degrees(90))
          .EndPoint(pan_loc);
        frame_projection =
          MakeProjection(pan_loc, radius_m, screen_size);
        if (options.draw_topo)
          ScanAll(topography, frame_projection);
        if (terrain) {
          const auto tile_radius =
            frame_projection.GetScreenWidthMeters() / 2;
          while (terrain->UpdateTiles(pan_loc, tile_radius)) {}
        }
      }
      DrawFrame(canvas, tr, ts, topo, options.draw_labels,
                asr,
#ifndef ENABLE_OPENGL
                airspace_renderer ? &stencil_canvas : nullptr,
#endif
                as, frame_projection, sun_azimuth);
      /* GPU bandwidth (Mali-G400) is in the finish, not the submit. */
      FlushGL();
    }) / draws;

    const double terrain_ms = TimeDrawsMs(draws, [&]() {
      canvas.ClearWhite();
      DrawTerrain(canvas, tr, ts, projection, sun_azimuth);
      FlushGL();
    }) / draws;

    const double topo_ms = TimeDrawsMs(draws, [&]() {
      canvas.ClearWhite();
      DrawTopography(canvas, topo, projection);
      FlushGL();
    }) / draws;

    const double airspace_ms = TimeDrawsMs(draws, [&]() {
      canvas.ClearWhite();
      DrawAirspace(canvas, asr,
#ifndef ENABLE_OPENGL
                   airspace_renderer ? &stencil_canvas : nullptr,
#endif
                   as, projection);
      FlushGL();
    }) / draws;

    const double labels_ms = options.draw_labels
      ? TimeDrawsMs(draws, [&]() {
          canvas.ClearWhite();
          DrawLabels(canvas, topo, projection);
          FlushGL();
        }) / draws
      : 0.;

    std::printf("%.0f\t%.0f\t%u\t%u\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
                radius_m, map_scale, visible.files, visible.shapes,
                scan_ms, tiles_ms, first_ms, frame_ms,
                terrain_ms, topo_ms, airspace_ms, labels_ms);
  }

  canvas.End();
  DeinitialiseFonts();
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
