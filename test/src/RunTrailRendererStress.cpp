// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugReplay.hpp"
#include "DebugReplayIGC.hpp"
#include "DebugReplayNMEA.hpp"
#include "Computer/Settings.hpp"
#include "Computer/TraceComputer.hpp"
#include "Look/TrailLook.hpp"
#include "MapSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "ProductName.hpp"
#include "Screen/Layout.hpp"
#include "Version.hpp"
#include "system/Args.hpp"
#include "system/Path.hpp"
#include "system/StandardVersion.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#include "ui/window/Init.hpp"
#include "util/NumberParser.hpp"
#include "util/PrintException.hxx"
#include "util/StringCompare.hxx"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

static constexpr const char canonical_name[] = "RunTrailRendererStress";

struct Options {
  unsigned sample_minutes = 10;
  unsigned draws_per_sample = 5;
  /** Circling half-width: ~1.5 km map (issue #2661 / typical climb zoom). */
  double circle_radius_m = 750;
  /** Cruise half-width: ~38 km map (typical task cruise). */
  double cruise_radius_m = 19000;
  /** Default ≈ OpenVario / 5" landscape at ~130 DPI logical size. */
  unsigned width = 800;
  unsigned height = 480;
  unsigned dpi = 130;
  TrailSettings::Type trail_type = TrailSettings::Type::VARIO_1;
  TrailSettings::Length trail_length = TrailSettings::Length::FULL;
  bool trail_scaling = true;
  bool sample_end_only = true;
  bool progress = true;
};

static Options options;

struct ReplayInput {
  Path file;
  std::string driver;
};

struct Checkpoint {
  unsigned flight_minutes;
  unsigned fix_index;
};

static void
PrintStandardHelp() noexcept
{
  std::printf(
    "Usage: %s [OPTION]... <driver> <file>\n"
    "   or: %s [OPTION]... <file>\n"
    "\n"
    "Replay a flight recording, accumulate a snail trail, and benchmark\n"
    "TrailRenderer draw cost (#2661).  Draw benchmarks are deferred until\n"
    "after replay so a long flight does not redraw an ever-growing trail\n"
    "inside the replay loop.\n"
    "\n"
    "Options:\n"
    "  --sample-minutes=N    sample every N flight minutes (default: 10)\n"
    "  --draws=N             map redraws per sample (default: 5)\n"
    "  --circle-radius=M     circling half-width in metres (default: 750,\n"
    "                        ~1.5 km map width)\n"
    "  --cruise-radius=M     cruise half-width in metres (default: 19000,\n"
    "                        ~38 km map width)\n"
    "  --width=PIXELS        canvas width (default: 800)\n"
    "  --height=PIXELS       canvas height (default: 480)\n"
    "  --dpi=N               Layout DPI for spacing scale (default: 130)\n"
    "  --trail-type=TYPE     vario1, vario1_dots, vario2, altitude\n"
    "  --trail-length=LEN    off, short, long, full (default: full)\n"
    "  --no-trail-scaling    disable scaled vario trail pens\n"
    "  --timeline            benchmark at each sample point (default: end only)\n"
    "  --end-only            benchmark only after full replay (default)\n"
    "  --no-progress         do not print replay progress on stderr\n"
    "  -h, --help            display this help and exit\n"
    "  --version             output version information and exit\n"
    "\n"
    "Example:\n"
    "  %s test/data/issue-2661/short_2026-06-20_11-47.nmea OpenVario\n"
    "  %s --timeline --sample-minutes=5 flight.igc\n"
    "\n"
    "Report bugs to: <%s>\n"
    "%s home page: <%s>\n",
    canonical_name, canonical_name, canonical_name, canonical_name,
    PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

[[gnu::pure]]
static std::optional<TrailSettings::Type>
ParseTrailType(const char *value) noexcept
{
  if (StringIsEqualIgnoreCase(value, "vario1"))
    return TrailSettings::Type::VARIO_1;
  if (StringIsEqualIgnoreCase(value, "vario1_dots"))
    return TrailSettings::Type::VARIO_1_DOTS;
  if (StringIsEqualIgnoreCase(value, "vario2"))
    return TrailSettings::Type::VARIO_2;
  if (StringIsEqualIgnoreCase(value, "altitude"))
    return TrailSettings::Type::ALTITUDE;
  return std::nullopt;
}

[[gnu::pure]]
static std::optional<TrailSettings::Length>
ParseTrailLength(const char *value) noexcept
{
  if (StringIsEqualIgnoreCase(value, "off"))
    return TrailSettings::Length::OFF;
  if (StringIsEqualIgnoreCase(value, "short"))
    return TrailSettings::Length::SHORT;
  if (StringIsEqualIgnoreCase(value, "long"))
    return TrailSettings::Length::LONG;
  if (StringIsEqualIgnoreCase(value, "full"))
    return TrailSettings::Length::FULL;
  return std::nullopt;
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
                  double &value) noexcept
{
  if (!StringStartsWith(arg, prefix))
    return false;

  const char *p = arg + std::strlen(prefix);
  char *end = nullptr;
  const double parsed = ParseDouble(p, &end);
  if (end == nullptr || *end != '\0' || parsed <= 0.)
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

    if (StringIsEqual(arg, "--timeline")) {
      args.Skip();
      options.sample_end_only = false;
      continue;
    }

    if (StringIsEqual(arg, "--end-only")) {
      args.Skip();
      options.sample_end_only = true;
      continue;
    }

    if (StringIsEqual(arg, "--no-progress")) {
      args.Skip();
      options.progress = false;
      continue;
    }

    if (StringIsEqual(arg, "--no-trail-scaling")) {
      args.Skip();
      options.trail_scaling = false;
      continue;
    }

    if (ParseUnsignedOption(arg, "--sample-minutes=", options.sample_minutes) ||
        ParseUnsignedOption(arg, "--draws=", options.draws_per_sample) ||
        ParseUnsignedOption(arg, "--width=", options.width) ||
        ParseUnsignedOption(arg, "--height=", options.height) ||
        ParseUnsignedOption(arg, "--dpi=", options.dpi)) {
      args.Skip();
      continue;
    }

    if (ParseDoubleOption(arg, "--circle-radius=", options.circle_radius_m) ||
        ParseDoubleOption(arg, "--cruise-radius=", options.cruise_radius_m)) {
      args.Skip();
      continue;
    }

    if (StringStartsWith(arg, "--trail-type=")) {
      args.Skip();
      const auto type = ParseTrailType(arg + 13);
      if (!type) {
        std::fprintf(stderr, "Unknown trail type '%s'\n", arg + 13);
        std::exit(EXIT_FAILURE);
      }
      options.trail_type = *type;
      continue;
    }

    if (StringStartsWith(arg, "--trail-length=")) {
      args.Skip();
      const auto length = ParseTrailLength(arg + 15);
      if (!length) {
        std::fprintf(stderr, "Unknown trail length '%s'\n", arg + 15);
        std::exit(EXIT_FAILURE);
      }
      options.trail_length = *length;
      continue;
    }

    break;
  }
}

static ReplayInput
ParseReplayInput(Args &args)
{
  ReplayInput input;

  if (!args.IsEmpty() && StringEndsWithIgnoreCase(args.PeekNext(), ".igc"))
    input.file = args.ExpectNextPath();
  else {
    input.driver = args.ExpectNextT();
    input.file = args.ExpectNextPath();
    if (StringEndsWithIgnoreCase(input.file.c_str(), ".igc"))
      input.driver.clear();
  }

  return input;
}

static DebugReplay *
OpenReplay(const ReplayInput &input)
{
  if (input.driver.empty())
    return DebugReplayIGC::Create(input.file);

  return DebugReplayNMEA::Create(input.file, input.driver);
}

static void
PushMergeVarioSample(TraceComputer &trace_computer,
                     const MoreData &basic,
                     const DerivedInfo &calculated) noexcept
{
  if (!basic.time_available || !basic.location_available ||
      !basic.NavAltitudeAvailable() || !calculated.flight.flying)
    return;

  float vario = 0;
  if (basic.netto_vario_available)
    vario = (float)basic.netto_vario;
  else if (basic.total_energy_vario_available)
    vario = (float)basic.total_energy_vario;
  else if (basic.brutto_vario_available)
    vario = (float)basic.brutto_vario;
  else
    return;

  trace_computer.PushMergeVarioSample(basic.time.Cast<TracePoint::Time>(), vario);
}

static void
ProcessFix(TraceComputer &trace_computer,
           const ComputerSettings &settings_computer,
           const MoreData &basic,
           const DerivedInfo &calculated) noexcept
{
  PushMergeVarioSample(trace_computer, basic, calculated);
  trace_computer.Update(settings_computer, basic, calculated);
}

static WindowProjection
MakeProjection(const GeoPoint &location, double radius_m,
               PixelSize screen_size) noexcept
{
  WindowProjection projection;
  projection.SetScreenSize(screen_size);
  projection.SetScaleFromRadius(radius_m);
  projection.SetGeoLocation(location);
  projection.SetScreenOrigin(screen_size.width / 2, screen_size.height / 2);
  projection.UpdateScreenBounds();
  return projection;
}

static double
BenchmarkDrawMs(Canvas &canvas, TrailRenderer &renderer,
                TraceComputer &trace_computer,
                const WindowProjection &projection,
                const MoreData &basic, const DerivedInfo &calculated,
                const TrailSettings &trail_settings,
                unsigned draws) noexcept
{
  if (draws == 0)
    return 0.;

  const PixelPoint aircraft_pos = projection.GeoToScreen(basic.location);

  using clock = std::chrono::steady_clock;
  const auto t0 = clock::now();
  for (unsigned i = 0; i < draws; ++i)
    renderer.Draw(canvas, trace_computer, projection, {},
                  false, aircraft_pos, basic, calculated, trail_settings);
  const auto t1 = clock::now();

  return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static void
PrintSampleHeader() noexcept
{
  std::puts("minute\tstore_pts\tmerge_samples\t"
            "circle_kept\tcruise_kept\t"
            "circle_budget\tcruise_budget\t"
            "circle_ms\tcruise_ms\t"
            "circle_map_scale\tcruise_map_scale");
}

static void
PrintSample(unsigned flight_minutes, unsigned store_pts,
            unsigned merge_samples,
            unsigned circle_kept, unsigned cruise_kept,
            unsigned circle_budget, unsigned cruise_budget,
            double circle_ms, double cruise_ms,
            double circle_map_scale, double cruise_map_scale) noexcept
{
  std::printf("%u\t%u\t%u\t%u\t%u\t%u\t%u\t%.2f\t%.2f\t%.0f\t%.0f\n",
              flight_minutes, store_pts, merge_samples,
              circle_kept, cruise_kept,
              circle_budget, cruise_budget,
              circle_ms, cruise_ms,
              circle_map_scale, cruise_map_scale);
}

static unsigned
CountKeptPoints(const TraceComputer &trace_computer,
                const TrailQuery &query) noexcept
{
  TracePointVector kept;
  std::vector<TrailVarioSample> merge_vario;
  trace_computer.LockedTrailQuery(query, kept, merge_vario);
  return unsigned(kept.size());
}

static void
ReportReplayProgress(unsigned fix_count, unsigned flight_minutes,
                     unsigned trace_points) noexcept
{
  std::fprintf(stderr,
               "replaying: fix %u, minute %u, trail %u points\n",
               fix_count, flight_minutes, trace_points);
}

static unsigned
CountTracePoints(const TraceComputer &trace_computer) noexcept
{
  TracePointVector points;
  std::vector<TrailVarioSample> merge_vario;
  trace_computer.LockedCopySnapshot(points, merge_vario);
  return unsigned(points.size());
}

static void
SampleAndPrint(Canvas &canvas, TrailRenderer &renderer,
               TraceComputer &trace_computer,
               const MoreData &basic, const DerivedInfo &calculated,
               const TrailSettings &trail_settings,
               PixelSize screen_size, unsigned flight_minutes) noexcept
{
  TracePointVector points;
  std::vector<TrailVarioSample> merge_samples;
  trace_computer.LockedCopySnapshot(points, merge_samples);

  const auto circle_projection =
    MakeProjection(basic.location, options.circle_radius_m, screen_size);
  const auto cruise_projection =
    MakeProjection(basic.location, options.cruise_radius_m, screen_size);

  const TrailQuery circle_query =
    TrailRenderer::MakeTrailQuery({}, circle_projection);
  const TrailQuery cruise_query =
    TrailRenderer::MakeTrailQuery({}, cruise_projection);
  const unsigned circle_kept = CountKeptPoints(trace_computer, circle_query);
  const unsigned cruise_kept = CountKeptPoints(trace_computer, cruise_query);

  if (options.progress)
    std::fprintf(stderr,
                 "benchmarking minute %u (store %u, circle kept %u/%u, "
                 "cruise kept %u/%u)...\n",
                 flight_minutes, unsigned(points.size()),
                 circle_kept, circle_query.max_points,
                 cruise_kept, cruise_query.max_points);

  const double circle_ms =
    BenchmarkDrawMs(canvas, renderer, trace_computer, circle_projection,
                    basic, calculated, trail_settings,
                    options.draws_per_sample);
  const double cruise_ms =
    BenchmarkDrawMs(canvas, renderer, trace_computer, cruise_projection,
                    basic, calculated, trail_settings,
                    options.draws_per_sample);

  PrintSample(flight_minutes, unsigned(points.size()),
              unsigned(merge_samples.size()),
              circle_kept, cruise_kept,
              circle_query.max_points, cruise_query.max_points,
              circle_ms, cruise_ms,
              circle_projection.GetMapScale(),
              cruise_projection.GetMapScale());
}

static bool
ReplayToCheckpoint(const ReplayInput &input,
                   TraceComputer &trace_computer,
                   const ComputerSettings &settings_computer,
                   unsigned target_fixes,
                   MoreData &basic_out,
                   DerivedInfo &calculated_out) noexcept
{
  std::unique_ptr<DebugReplay> replay(OpenReplay(input));
  if (replay == nullptr)
    return false;

  trace_computer.Reset();

  unsigned fix_count = 0;
  while (replay->Next()) {
    ++fix_count;
    ProcessFix(trace_computer, settings_computer,
               replay->Basic(), replay->Calculated());

    if (fix_count >= target_fixes) {
      basic_out = replay->Basic();
      calculated_out = replay->Calculated();
      return true;
    }
  }

  if (fix_count == 0)
    return false;

  basic_out = replay->Basic();
  calculated_out = replay->Calculated();
  return fix_count >= target_fixes;
}

static std::vector<Checkpoint>
CollectCheckpoints(const ReplayInput &input,
                   const ComputerSettings &settings_computer) noexcept
{
  std::vector<Checkpoint> checkpoints;

  std::unique_ptr<DebugReplay> replay(OpenReplay(input));
  if (replay == nullptr)
    return checkpoints;

  TraceComputer trace_computer;
  std::optional<TimeStamp> flight_start;
  unsigned last_sampled_minute = 0;
  unsigned fix_count = 0;
  unsigned last_progress_minute = 0;

  while (replay->Next()) {
    ++fix_count;
    ProcessFix(trace_computer, settings_computer,
               replay->Basic(), replay->Calculated());

    const MoreData &basic = replay->Basic();
    if (!basic.time_available)
      continue;

    if (!flight_start)
      flight_start = basic.time;

    const auto elapsed = basic.time - *flight_start;
    const unsigned flight_minutes =
      unsigned(std::chrono::duration_cast<std::chrono::minutes>(elapsed).count());

    if (options.progress &&
        flight_minutes >= last_progress_minute + 5) {
      ReportReplayProgress(fix_count, flight_minutes,
                           CountTracePoints(trace_computer));
      last_progress_minute = flight_minutes;
    }

    if (flight_minutes < last_sampled_minute + options.sample_minutes)
      continue;

    last_sampled_minute = flight_minutes;
    checkpoints.push_back({flight_minutes, fix_count});
  }

  if (fix_count == 0)
    return checkpoints;

  const MoreData &basic = replay->Basic();
  unsigned flight_minutes = 0;
  if (basic.time_available && flight_start)
    flight_minutes = unsigned(std::chrono::duration_cast<std::chrono::minutes>(
      basic.time - *flight_start).count());

  if (checkpoints.empty() ||
      checkpoints.back().fix_index != fix_count)
    checkpoints.push_back({flight_minutes, fix_count});

  if (options.progress)
    ReportReplayProgress(fix_count, flight_minutes,
                         CountTracePoints(trace_computer));

  return checkpoints;
}

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "DRIVER FILE");
  ParseCommandLine(args);

  const ReplayInput input = ParseReplayInput(args);
  args.ExpectEnd();

  const PixelSize screen_size{int(options.width), int(options.height)};

  ScreenGlobalInit screen_init;
  Layout::Initialise(screen_init.GetDisplay(), screen_size,
                     100, options.dpi);

  BufferCanvas canvas;
  canvas.Create(screen_size);
  canvas.ClearWhite();

  TrailSettings trail_settings{};
  trail_settings.wind_drift_enabled = false;
  trail_settings.scaling_enabled = options.trail_scaling;
  trail_settings.type = options.trail_type;
  trail_settings.length = options.trail_length;

  TrailLook trail_look;
  trail_look.Initialise(trail_settings);

  TrailRenderer renderer(trail_look);

  ComputerSettings settings_computer{};
  settings_computer.contest.enable = false;

  PrintSampleHeader();

  if (options.sample_end_only) {
    std::unique_ptr<DebugReplay> replay(OpenReplay(input));
    if (replay == nullptr)
      return EXIT_FAILURE;

    TraceComputer trace_computer;
    std::optional<TimeStamp> flight_start;
    unsigned fix_count = 0;
    unsigned last_progress_minute = 0;

    while (replay->Next()) {
      ++fix_count;
      ProcessFix(trace_computer, settings_computer,
                 replay->Basic(), replay->Calculated());

      const MoreData &basic = replay->Basic();
      if (!basic.time_available)
        continue;

      if (!flight_start)
        flight_start = basic.time;

      const unsigned flight_minutes =
        unsigned(std::chrono::duration_cast<std::chrono::minutes>(
          basic.time - *flight_start).count());

      if (options.progress &&
          flight_minutes >= last_progress_minute + 5) {
        ReportReplayProgress(fix_count, flight_minutes,
                             CountTracePoints(trace_computer));
        last_progress_minute = flight_minutes;
      }
    }

    if (fix_count == 0) {
      std::fprintf(stderr, "%s: replay produced no fixes\n", canonical_name);
      return EXIT_FAILURE;
    }

    const MoreData &basic = replay->Basic();
    const DerivedInfo &calculated = replay->Calculated();
    unsigned flight_minutes = 0;
    if (basic.time_available && flight_start)
      flight_minutes = unsigned(std::chrono::duration_cast<std::chrono::minutes>(
        basic.time - *flight_start).count());

    if (options.progress)
      ReportReplayProgress(fix_count, flight_minutes,
                           CountTracePoints(trace_computer));

    SampleAndPrint(canvas, renderer, trace_computer,
                   basic, calculated, trail_settings,
                   screen_size, flight_minutes);
    return EXIT_SUCCESS;
  }

  const std::vector<Checkpoint> checkpoints =
    CollectCheckpoints(input, settings_computer);

  if (checkpoints.empty()) {
    std::fprintf(stderr, "%s: replay produced no fixes\n", canonical_name);
    return EXIT_FAILURE;
  }

  TraceComputer trace_computer;
  MoreData basic;
  DerivedInfo calculated;

  for (const Checkpoint &checkpoint : checkpoints) {
    if (!ReplayToCheckpoint(input, trace_computer, settings_computer,
                            checkpoint.fix_index, basic, calculated)) {
      std::fprintf(stderr, "%s: replay stopped before fix %u\n",
                   canonical_name, checkpoint.fix_index);
      return EXIT_FAILURE;
    }

    SampleAndPrint(canvas, renderer, trace_computer,
                   basic, calculated, trail_settings,
                   screen_size, checkpoint.flight_minutes);
  }

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
