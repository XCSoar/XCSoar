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

#include "Tracking/SkyLines/Client.hpp"
#include "Tracking/SkyLines/Handler.hpp"
#include "NMEA/Info.hpp"
#include "net/Resolver.hxx"
#include "net/AddressInfo.hxx"
#include "system/Args.hpp"
#include "event/Loop.hxx"
#include "event/FineTimerEvent.hxx"
#include "util/NumberParser.hpp"
#include "util/StringUtil.hpp"
#include "util/PrintException.hxx"
#include "DebugReplay.hpp"

#include <memory>

class Handler : public SkyLinesTracking::Handler {
  Args &args;

  EventLoop &event_loop;

  SkyLinesTracking::Client client;

  FineTimerEvent stop_timer{event_loop, BIND_THIS_METHOD(OnStopTimer)};
  FineTimerEvent next_timer{event_loop, BIND_THIS_METHOD(OnNextTimer)};

  std::unique_ptr<DebugReplay> replay;

public:
  explicit Handler(Args &_args, EventLoop &_event_loop)
    :args(_args), event_loop(_event_loop),
     client(event_loop, this) {}

  SkyLinesTracking::Client &GetClient() {
    return client;
  }

  void OnSkyLinesReady() override;

  virtual void OnAck(unsigned id) override {
    printf("received ack %u\n", id);
    event_loop.Break();
  }

  virtual void OnTraffic(unsigned pilot_id, unsigned time_of_day_ms,
                         const GeoPoint &location, int altitude) override {
    BrokenTime time = BrokenTime::FromSecondOfDay(time_of_day_ms / 1000);

    printf("received traffic pilot=%u time=%02u:%02u:%02u location=%f/%f altitude=%d\n",
           pilot_id, time.hour, time.minute, time.second,
           (double)location.longitude.Degrees(),
           (double)location.latitude.Degrees(),
           altitude);

    stop_timer.Schedule(std::chrono::seconds(1));
  }

  void OnSkyLinesError(std::exception_ptr e) override {
    PrintException(e);

    stop_timer.Cancel();
    next_timer.Cancel();
  }

private:
  void OnStopTimer() noexcept {
    event_loop.Break();
  }

  void OnNextTimer() noexcept {
    if (replay->Next()) {
      client.SendFix(replay->Basic());
      next_timer.Schedule(std::chrono::milliseconds(100));
    } else
      event_loop.Break();
  }
};

void
Handler::OnSkyLinesReady()
{
  if (args.IsEmpty() || StringIsEqual(args.PeekNext(), "fix")) {
    NMEAInfo basic;
    basic.Reset();
    basic.UpdateClock();
    basic.time = 1;
    basic.time_available.Update(basic.clock);

    client.SendFix(basic);
  } else if (StringIsEqual(args.PeekNext(), "ping")) {
    client.SendPing(1);
  } else if (StringIsEqual(args.PeekNext(), "traffic")) {
    client.SendTrafficRequest(true, true, true);
  } else {
    replay.reset(CreateDebugReplay(args));
    if (replay == nullptr)
      throw std::runtime_error("CreateDebugReplay() failed");

    next_timer.Schedule(std::chrono::seconds(0));
  }
}

int
main(int argc, char *argv[])
try {
  Args args(argc, argv, "HOST KEY");
  const char *host = args.ExpectNext();
  const char *key = args.ExpectNext();

  const auto address_list = Resolve(host,
                                    SkyLinesTracking::Client::GetDefaultPort(),
                                    0, SOCK_DGRAM);

  EventLoop event_loop;

  Handler handler(args, event_loop);

  auto &client = handler.GetClient();
  client.SetKey(ParseUint64(key, NULL, 16));
  client.Open(address_list.GetBest());

  event_loop.Run();

  return EXIT_SUCCESS;
} catch (const std::exception &e) {
  fprintf(stderr, "%s\n", e.what());
  return EXIT_FAILURE;
}
