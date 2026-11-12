// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CoInstance.hpp"
#include "Tracking/LiveTrack24/AppKey.hpp"
#include "Tracking/LiveTrack24/Client.hpp"
#include "net/http/Init.hpp"
#include "time/BrokenDateTime.hpp"
#include "Units/System.hpp"
#include "system/Args.hpp"
#include "DebugReplay.hpp"
#include "Version.hpp"
#include "co/Task.hxx"
#include "util/PrintException.hxx"

#include <cmath>
#include <cstdio>

using namespace LiveTrack24;

struct Instance : CoInstance {
  const Net::ScopeInit net_init{GetEventLoop()};
};

static Co::InvokeTask
TestTracking(int argc, char *argv[], LiveTrack24::Client &client)
{
  Args args(argc, argv, "[DRIVER] FILE USERNAME PASSWORD");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    throw std::runtime_error("CreateDebugReplay() failed");

  const std::string username = args.ExpectNextT();
  const std::string password = args.ExpectNextT();
  args.ExpectEnd();

  Session session;
  session.user = co_await client.GetUserID(username.c_str(),
                                           password.c_str(),
                                           GetAppSecret());
  session.id = GenerateSessionID();
  session.token = MakeToken(password, session.id, GetAppSecret());
  printf("User id %u, session id %u\n", session.user, session.id);

  const TrackInfo info{VehicleType::GLIDER, "Hornet", "XCSoar"};

  BrokenDate now = BrokenDate::TodayUTC();

  printf("Sending positions ");
  unsigned packet_id = 1;
  while (replay->Next()) {
    if (packet_id % 10 == 0) {
      putchar('.');
      fflush(stdout);
    }

    const MoreData &basic = replay->Basic();

    const BrokenTime time = basic.date_time_utc;
    BrokenDateTime datetime(now.year, now.month, now.day, time.hour,
                            time.minute, time.second);

    const Fix fix{
      datetime.ToTimePoint(), basic.location,
      (int)std::lround(basic.nav_altitude),
      (unsigned)std::lround(Units::ToUserUnit(basic.ground_speed,
                                              Unit::KILOMETER_PER_HOUR)),
      (unsigned)std::lround(basic.track.AsBearing().Degrees()) % 360,
    };

    co_await client.SendPosition(session, packet_id, fix,
                                 packet_id == 1 ? &info : nullptr);
    packet_id++;
  }
  printf("done\n");

  printf("Stopping tracking ... ");
  co_await client.EndTracking(session, packet_id);
  printf("done\n");
}

int
main(int argc, char *argv[])
try {
  if (!HasAppKey()) {
    fprintf(stderr, "No LiveTrack24 AppKey and AppSecret in "
            "src/Tracking/LiveTrack24/AppKey.cpp\n");
    return EXIT_FAILURE;
  }

  Instance instance;

  Client client{*Net::curl, AppInfo{GetAppKey(), XCSoar_Version, "xcsoar"}};
  instance.Run(TestTracking(argc, argv, client));
  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
