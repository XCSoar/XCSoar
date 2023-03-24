// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "net/client/WeGlide/UploadFlight.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "net/http/Init.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "json/Serialize.hxx"
#include "co/Task.hxx"
#include "system/Args.hpp"
#include "io/StdioOutputStream.hxx"
#include "util/Macros.hpp"
#include "util/PrintException.hxx"

#include <boost/json.hpp>

#include <cstdio>

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};

  boost::json::value value;

  Co::InvokeTask DoRun(const WeGlideSettings &settings,
                       uint_least32_t glider_type,
                       Path igc_path,
                       ProgressListener &progress)
  {
    value = co_await WeGlide::UploadFlight(*Net::curl, settings, glider_type,
                                           igc_path, progress);
  }
};

int
main(int argc, char *argv[])
try {
  Args args(argc, argv, "PILOT BIRTHDAY GLIDER IGCFILE");
  const unsigned pilot = atoi(args.ExpectNext());
  const char *birthday_s = args.ExpectNext();
  const unsigned glider = atoi(args.ExpectNext());
  const auto igc_path = args.ExpectNextPath();
  args.ExpectEnd();

  unsigned year, month, day;
  if (sscanf(birthday_s, "%04u-%02u-%02u", &year, &month, &day) != 3)
    throw "Failed to parse date";

  WeGlideSettings settings;
  settings.pilot_id = pilot;
  settings.pilot_birthdate = {year, month, day};

  Instance instance;
  ConsoleOperationEnvironment env;

  instance.Run(instance.DoRun(settings, glider,
                              igc_path, env));

  StdioOutputStream _stdout(stdout);
  Json::Serialize(_stdout, instance.value);
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
