/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "CoInstance.hpp"
#include "Cloud/weglide/UploadFlight.hpp"
#include "Cloud/weglide/WeGlideSettings.hpp"
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
