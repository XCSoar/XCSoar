/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "OS/Args.hpp"
#include "Util/NumberParser.hpp"
#include "Util/StringUtil.hpp"
#include "DebugReplay.hpp"

#include <boost/asio/steady_timer.hpp>

#include <memory>

class Handler : public SkyLinesTracking::Handler {
  Args &args;

  SkyLinesTracking::Client client;

  boost::asio::steady_timer timer;

  std::unique_ptr<DebugReplay> replay;

public:
  explicit Handler(Args &_args, boost::asio::io_service &io_service)
    :args(_args), client(io_service, this), timer(io_service) {}

  SkyLinesTracking::Client &GetClient() {
    return client;
  }

  void OnSkyLinesReady() override;

  virtual void OnAck(unsigned id) override {
    printf("received ack %u\n", id);
    timer.get_io_service().stop();
  }

  virtual void OnTraffic(unsigned pilot_id, unsigned time_of_day_ms,
                         const GeoPoint &location, int altitude) override {
    BrokenTime time = BrokenTime::FromSecondOfDay(time_of_day_ms / 1000);

    printf("received traffic pilot=%u time=%02u:%02u:%02u location=%f/%f altitude=%d\n",
           pilot_id, time.hour, time.minute, time.second,
           (double)location.longitude.Degrees(),
           (double)location.latitude.Degrees(),
           altitude);

    ScheduleStop(std::chrono::seconds(1));
  }

  void OnSkyLinesError(const std::exception &e) override {
    fprintf(stderr, "Error: %s\n", e.what());

    timer.cancel();
  }

private:
  void ScheduleStop(boost::asio::steady_timer::duration d) {
    timer.expires_from_now(d);
    timer.async_wait([this](const boost::system::error_code &ec){
        if (!ec)
          timer.get_io_service().stop();
      });
  }

  void NextReplay(const boost::system::error_code &ec) {
    if (ec)
      return;

    if (replay->Next()) {
      client.SendFix(replay->Basic());
      ScheduleNextReplay(std::chrono::milliseconds(100));
    } else
      timer.get_io_service().stop();
  }

  void ScheduleNextReplay(boost::asio::steady_timer::duration d) {
    timer.expires_from_now(d);
    timer.async_wait(std::bind(&Handler::NextReplay, this,
                               std::placeholders::_1));
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

    ScheduleNextReplay(std::chrono::seconds(0));
  }
}

int
main(int argc, char *argv[])
try {
  Args args(argc, argv, "HOST KEY");
  const char *host = args.ExpectNext();
  const char *key = args.ExpectNext();

  boost::asio::io_service io_service;

  /* IPv4 only for now, because the official SkyLines tracking server
     doesn't support IPv6 yet */
  const boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(),
                                                    host,
                                                    SkyLinesTracking::Client::GetDefaultPortString());

  Handler handler(args, io_service);

  auto &client = handler.GetClient();
  client.SetKey(ParseUint64(key, NULL, 16));
  client.Open(query);

  io_service.run();

  return EXIT_SUCCESS;
} catch (const std::exception &e) {
  fprintf(stderr, "%s\n", e.what());
  return EXIT_FAILURE;
}
