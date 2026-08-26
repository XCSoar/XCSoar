// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/IGCFlightTimes.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "ProductName.hpp"
#include "Version.hpp"
#include "system/Args.hpp"
#include "system/Path.hpp"
#include "system/StandardVersion.hpp"
#include "util/StringCompare.hxx"

#include <cstdio>
#include <cstdlib>

/** Canonical name for `--version` (do not derive from argv[0]). */
static constexpr const char canonical_name[] = "RunIGCFlightTimes";

/** Print usage and project URLs to stdout (for --help). */
static void
PrintStandardHelp() noexcept
{
  std::printf(
      "Usage: %s [OPTION]... FILE.igc\n"
      "\n"
      "Detect takeoff and landing in an IGC file with the same\n"
      "FlyingComputer rules as XCSoar Logbook (E-records ignored).\n"
      "\n"
      "Options:\n"
      "  -h, --help     display this help and exit\n"
      "  --version      output version information and exit\n"
      "\n"
      "Report bugs to: <%s>\n"
      "%s home page: <%s>\n",
      canonical_name, PRODUCT_BUGS_URL, PRODUCT_NAME,
      PRODUCT_WEB_SITE_URL);
}

int
main(int argc, char **argv)
{
  Args args(argc, argv, "FILE.igc");

  while (!args.IsEmpty()) {
    const char *peek = args.PeekNext();

    if (StringIsEqual(peek, "--help") || StringIsEqual(peek, "-h")) {
      args.Skip();
      PrintStandardHelp();
      return EXIT_SUCCESS;
    }

    if (StringIsEqual(peek, "--version")) {
      args.Skip();
      PrintStandardVersion(canonical_name, XCSoar_Version);
      return EXIT_SUCCESS;
    }

    break;
  }

  const Path path = args.ExpectNextPath();
  args.ExpectEnd();

  IGCFlightTimesResult result;
  if (!DetectIGCFlightTimes(path, DEFAULT_IGC_TAKEOFF_SPEED, result)) {
    fprintf(stderr, "No takeoff/landing detected\n");
    return EXIT_FAILURE;
  }

  char takeoff[32], landing[32];
  FormatISO8601(takeoff, result.takeoff_utc);
  FormatISO8601(landing, result.landing_utc);
  printf("takeoff %s\nlanding %s\n", takeoff, landing);

  const unsigned start_m =
    result.takeoff_utc.hour * 60u + result.takeoff_utc.minute;
  unsigned end_m =
    result.landing_utc.hour * 60u + result.landing_utc.minute;
  if (end_m < start_m)
    end_m += 24u * 60u;
  const unsigned dur = end_m - start_m;
  printf("display %02u:%02u - %02u:%02u  (%02u:%02u)\n",
         result.takeoff_utc.hour, result.takeoff_utc.minute,
         result.landing_utc.hour, result.landing_utc.minute,
         dur / 60, dur % 60);
  if (result.started_too_late || result.ended_too_early) {
    printf("warn:");
    if (result.started_too_late)
      printf(" started too late");
    if (result.ended_too_early)
      printf("%s ended too early",
             result.started_too_late ? "," : "");
    printf("\n");
  }
  return EXIT_SUCCESS;
}
