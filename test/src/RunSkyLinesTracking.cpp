// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Tracking/SkyLines/Client.hpp"
#include "Tracking/SkyLines/Handler.hpp"
#include "FLARM/Traffic.hpp"
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

using namespace std::chrono;

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
                         const GeoPoint &location, int altitude,
                         unsigned flarm_id = 0,
                         unsigned track = 0,
                         double turn_rate = 0,
                         FlarmTraffic::AircraftType aircraft_type = FlarmTraffic::AircraftType::UNKNOWN) override {
    auto time = BrokenTime::FromSinceMidnight(milliseconds(time_of_day_ms));

    if (flarm_id != 0) {
      printf("received traffic pilot=%u flarm_id=%u time=%02u:%02u:%02u location=%f/%f altitude=%d track=%u° turn_rate=%.1f type=%u\n",
             pilot_id, flarm_id, time.hour, time.minute, time.second,
             (double)location.longitude.Degrees(),
             (double)location.latitude.Degrees(),
             altitude, track, turn_rate, (unsigned)aircraft_type);
    } else {
      printf("received traffic pilot=%u time=%02u:%02u:%02u location=%f/%f altitude=%d track=%u° turn_rate=%.1f type=%u\n",
             pilot_id, time.hour, time.minute, time.second,
             (double)location.longitude.Degrees(),
             (double)location.latitude.Degrees(),
             altitude, track, turn_rate, (unsigned)aircraft_type);
    }

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
    basic.time = TimeStamp{FloatDuration{1}};
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
