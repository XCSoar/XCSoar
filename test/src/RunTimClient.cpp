// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "net/client/tim/Client.hpp"
#include "net/client/tim/Thermal.hpp"
#include "net/http/Init.hpp"
#include "co/Task.hxx"
#include "system/Args.hpp"
#include "util/PrintException.hxx"

#include <cstdio>

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};

  std::vector<TIM::Thermal> value;

  Co::InvokeTask DoRun(std::chrono::hours max_age,
                       GeoPoint location, unsigned radius)
  {
    value = co_await TIM::GetThermals(*Net::curl, max_age, location, radius);
  }
};

int
main(int argc, char *argv[])
try {
  Args args(argc, argv, "MAX_AGE_HOURS LAT LON RADIUS");
  const std::chrono::hours max_age{args.ExpectNextInt()};
  const double lat = args.ExpectNextDouble();
  const double lon = args.ExpectNextDouble();
  const unsigned radius = args.ExpectNextInt();
  args.ExpectEnd();

  const GeoPoint location{Angle::Degrees(lon), Angle::Degrees(lat)};
  if (!location.Check())
    throw std::runtime_error("Invalid location");

  Instance instance;

  instance.Run(instance.DoRun(max_age, location, radius));

  for (const auto &i : instance.value) {
    printf("%f %f\n",
           i.location.latitude.Degrees(), i.location.longitude.Degrees());
  }

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
