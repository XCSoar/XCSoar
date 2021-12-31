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
#include "Tracking/LiveTrack24/Client.hpp"
#include "net/http/Init.hpp"
#include "time/BrokenDateTime.hpp"
#include "Units/System.hpp"
#include "system/Args.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "DebugReplay.hpp"
#include "co/Task.hxx"
#include "util/PrintException.hxx"

#include <cstdio>

using namespace LiveTrack24;

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};
};

static Co::InvokeTask
TestTracking(int argc, char *argv[], LiveTrack24::Client &client)
{
  Args args(argc, argv, "[DRIVER] FILE [USERNAME [PASSWORD]]");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    throw std::runtime_error("CreateDebugReplay() failed");

  bool has_user_id;
  UserID user_id;
  tstring username, password;
  if (args.IsEmpty()) {
    username = _T("");
    password = _T("");
    has_user_id = false;
  } else {
    username = args.ExpectNextT();
    password = args.IsEmpty() ? _T("") : args.ExpectNextT();

    user_id = co_await client.GetUserID(username.c_str(), password.c_str());
    has_user_id = (user_id != 0);
  }

  SessionID session = has_user_id ?
                      GenerateSessionID(user_id) : GenerateSessionID();
  printf("Generated session id: %u\n", session);


  printf("Starting tracking ... ");
  co_await client.StartTracking(session, username.c_str(),
                                password.c_str(), 10,
                                VehicleType::GLIDER, _T("Hornet"));
  printf("done\n");

  BrokenDate now = BrokenDate::TodayUTC();

  printf("Sending positions ");
  unsigned package_id = 2;
  while (replay->Next()) {
    if (package_id % 10 == 0) {
      putchar('.');
      fflush(stdout);
    }
    const MoreData &basic = replay->Basic();

    const BrokenTime time = basic.date_time_utc;
    BrokenDateTime datetime(now.year, now.month, now.day, time.hour,
                            time.minute, time.second);

    co_await client.SendPosition(session, package_id, basic.location,
                                 (unsigned)basic.nav_altitude,
                                 (unsigned)Units::ToUserUnit(basic.ground_speed,
                                                             Unit::KILOMETER_PER_HOUR),
                                 basic.track, datetime.ToTimePoint());

    package_id++;
  }
  printf("done\n");

  printf("Stopping tracking ... ");
  co_await client.EndTracking(session, package_id);
  printf("done\n");
}

int
main(int argc, char *argv[])
try {
  Instance instance;

  Client client{*Net::curl};
  client.SetServer(_T("www.livetrack24.com"));
  instance.Run(TestTracking(argc, argv, client));
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
