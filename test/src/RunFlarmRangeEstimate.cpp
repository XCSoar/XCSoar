// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Args.hpp"
#include "system/StandardVersion.hpp"
#include "DebugReplay.hpp"
#include "FLARM/Range.hpp"
#include "FLARM/RangeEstimate.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "ProductName.hpp"
#include "Version.hpp"
#include "util/StringCompare.hxx"

#include <cstdio>
#include <cstdlib>

/** Canonical name for `--version` (do not derive from argv[0]). */
static constexpr const char canonical_name[] = "RunFlarmRangeEstimate";

static void
PrintStandardHelp() noexcept
{
  std::printf(
      "Usage: %s [OPTION]... <driver> <file>\n"
      "\n"
      "Replay a flight recording and print the FLARM range estimate\n"
      "XCSoar collects from the traffic the FLARM reports in flight.\n"
      "\n"
      "Options:\n"
      "  --save=FILE    also write the estimate to FILE, in the format of\n"
      "                 flarm-range.txt in the data directory\n"
      "  -h, --help     display this help and exit\n"
      "  --version      output version information and exit\n"
      "\n"
      "Report bugs to: <%s>\n"
      "%s home page: <%s>\n",
      canonical_name, PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

int
main(int argc, char **argv)
{
  Args args(argc, argv, "[--save=FILE] DRIVER FILE");

  const char *save_path = nullptr;
  while (!args.IsEmpty()) {
    const char *peek = args.PeekNext();
    if (StringIsEqual(peek, "--help") || StringIsEqual(peek, "-h")) {
      PrintStandardHelp();
      return EXIT_SUCCESS;
    } else if (StringIsEqual(peek, "--version")) {
      PrintStandardVersion(canonical_name, XCSoar_Version);
      return EXIT_SUCCESS;
    } else if (const char *p = StringAfterPrefix(peek, "--save=")) {
      args.Skip();
      save_path = p;
    } else
      break;
  }

  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == nullptr)
    return EXIT_FAILURE;

  args.ExpectEnd();

  FlarmRangeEstimate estimate;
  FlarmRangeEstimator estimator;
  unsigned fixes = 0, flying = 0, with_track = 0;
  while (replay->Next()) {
    const bool is_flying = replay->Calculated().flight.flying;
    ++fixes;
    if (is_flying) {
      ++flying;
      if (replay->Basic().track_available)
        ++with_track;
    }

    estimator.Process(estimate, replay->Basic(), is_flying);
  }

  delete replay;

  std::printf("bearing  reports  p90 [m]  max [m]  minimum [m]\n");
  for (unsigned i = 0; i < FlarmRangeEstimate::SECTORS; ++i) {
    const auto &sector = estimate.sectors[i];
    const Angle bearing = Angle::FullCircle() *
      ((i + 0.5) / FlarmRangeEstimate::SECTORS);
    const auto p90 = sector.Percentile(0.9);
    std::printf("%5.0f   %8u  %7u  %7u  %7.0f\n",
                bearing.Degrees(), unsigned(sector.count),
                p90 ? *p90 : 0u, unsigned(sector.maximum),
                FlarmMinimumRange(bearing));
  }

  std::printf("fixes: %u, in flight: %u, with ground track: %u\n",
              fixes, flying, with_track);
  std::printf("total reports: %u\n", unsigned(estimate.GetCount()));

  if (save_path != nullptr) {
    FileOutputStream file{Path{save_path}};
    BufferedOutputStream os{file};
    SaveFlarmRangeEstimate(estimate, os);
    os.Flush();
    file.Commit();
  }

  return EXIT_SUCCESS;
}
