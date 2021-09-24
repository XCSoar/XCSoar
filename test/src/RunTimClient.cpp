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
