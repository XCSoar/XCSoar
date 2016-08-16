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

#include "Data.hpp"
#include "Dump.hpp"
#include "Serialiser.hpp"
#include "Tracking/SkyLines/Server.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "Tracking/SkyLines/Export.hpp"
#include "OS/ByteOrder.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/FileReader.hxx"
#include "Util/CRC.hpp"
#include "Util/PrintException.hxx"
#include "Compiler.h"

#ifdef __linux__
#include "IO/Async/SignalListener.hpp"
#endif

#include <boost/asio/steady_timer.hpp>

#include <array>
#include <iostream>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;

class CloudServer final
  : public SkyLinesTracking::Server,
#ifdef __linux__
    SignalListener,
#endif
    CloudData
{
  const AllocatedPath db_path;

  boost::asio::steady_timer save_timer, expire_timer;

public:
  CloudServer(AllocatedPath &&_db_path, boost::asio::io_service &io_service,
              boost::asio::ip::udp::endpoint endpoint)
    :SkyLinesTracking::Server(io_service, endpoint),
#ifdef __linux__
    SignalListener(io_service),
#endif
    db_path(std::move(_db_path)),
    save_timer(io_service),
    expire_timer(io_service)
  {
#ifdef __linux__
    SignalListener::Create(SIGTERM, SIGINT, SIGHUP, SIGUSR1);
#endif

    ScheduleSave();
  }

  using SkyLinesTracking::Server::get_io_service;

  void Load();
  void Save();

private:
  void ScheduleSave() {
    save_timer.expires_from_now(std::chrono::minutes(1));
    save_timer.async_wait([this](const boost::system::error_code &ec){
        if (ec)
          return;

        Save();
        ScheduleSave();
      });
  }

  void ScheduleExpire() {
    expire_timer.expires_from_now(std::chrono::minutes(5));
    expire_timer.async_wait([this](const boost::system::error_code &ec){
        if (ec)
          return;

        clients.Expire(expire_timer.expires_at() - std::chrono::minutes(10));
        if (!clients.empty())
          ScheduleExpire();
      });
  }

protected:
  /* virtual methods from class SkyLinesTracking::Server */
  void OnFix(const Client &client,
             std::chrono::milliseconds time_of_day,
             const ::GeoPoint &location, int altitude) override;

  void OnTrafficRequest(const Client &client,
                        bool near) override;

  void OnWaveSubmit(const Client &client,
                    std::chrono::milliseconds time_of_day,
                    const ::GeoPoint &a, const ::GeoPoint &b,
                    int bottom_altitude,
                    int top_altitude,
                    double lift) override;

  void OnThermalSubmit(const Client &client,
                       std::chrono::milliseconds time_of_day,
                       const ::GeoPoint &bottom_location,
                       int bottom_altitude,
                       const ::GeoPoint &top_location,
                       int top_altitude,
                       double lift) override;

  void OnThermalRequest(const Client &client) override;

  void OnSendError(const boost::asio::ip::udp::endpoint &endpoint,
                   std::exception &&e) override {
    cerr << "Failed to send to " << endpoint
         << ": " << e.what()
         << endl;
  }

  void OnError(std::exception &&e) override {
    cerr << e.what() << endl;
    get_io_service().stop();
  }

#ifdef __linux__
  /* virtual methods from class SignalListener */
  void OnSignal(int signo) override {
    switch (signo) {
    case SIGHUP:
      Save();
      break;

    case SIGUSR1:
      DumpClients();
      break;

    default:
      get_io_service().stop();
      break;
    }
  }
#endif
};

void
CloudServer::OnFix(const Client &c,
                   std::chrono::milliseconds time_of_day,
                   const ::GeoPoint &location, int altitude)
{
  (void)time_of_day; // TODO: use this parameter

  if (location.IsValid()) {
    bool was_empty = clients.empty();

    auto &client = clients.Make(c.endpoint, c.key, location, altitude);

    cout << "FIX\t"
         << client.endpoint << '\t'
         << std::hex << client.key << std::dec << '\t'
         << client.id << '\t'
         << client.location << '\t'
         << client.altitude << 'm'
         << endl;

    if (was_empty)
      ScheduleExpire();
  } else {
    auto *client = clients.Find(c.key);
    if (client != nullptr)
      clients.Refresh(*client, c.endpoint);
  }
}

class TrafficResponseSender {
  SkyLinesTracking::Server &server;
  const boost::asio::ip::udp::endpoint &endpoint;

  static constexpr size_t MAX_TRAFFIC_SIZE = 1024;
  static constexpr size_t MAX_TRAFFIC =
    MAX_TRAFFIC_SIZE / sizeof(SkyLinesTracking::TrafficResponsePacket::Traffic);

  struct Packet {
    SkyLinesTracking::TrafficResponsePacket header;
    std::array<SkyLinesTracking::TrafficResponsePacket::Traffic, MAX_TRAFFIC> traffic;
  } data;

  unsigned n_traffic = 0;

public:
  TrafficResponseSender(SkyLinesTracking::Server &_server,
                        const SkyLinesTracking::Server::Client &client)
    :server(_server), endpoint(client.endpoint) {
    data.header.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    data.header.header.type = ToBE16(SkyLinesTracking::Type::TRAFFIC_RESPONSE);
    data.header.header.key = ToBE64(client.key);

    data.header.reserved = 0;
    data.header.reserved2 = 0;
    data.header.reserved3 = 0;
  }

  void Add(uint32_t pilot_id, uint32_t time,
           GeoPoint location, int altitude) {
    assert(n_traffic < MAX_TRAFFIC);

    auto &traffic = data.traffic[n_traffic++];
    traffic.pilot_id = ToBE32(pilot_id);
    traffic.time = ToBE32(time);
    traffic.location = SkyLinesTracking::ExportGeoPoint(location);
    traffic.altitude = ToBE16(altitude);
    traffic.reserved = 0;
    traffic.reserved2 = 0;

    if (n_traffic == MAX_TRAFFIC)
      Flush();
  }

  void Flush() {
    if (n_traffic == 0)
      return;

    size_t size = sizeof(data.header) + sizeof(data.traffic[0]) * n_traffic;

    data.header.traffic_count = n_traffic;
    n_traffic = 0;

    data.header.header.crc = 0;
    data.header.header.crc = ToBE16(UpdateCRC16CCITT(&data, size, 0));
    server.SendBuffer(endpoint, boost::asio::const_buffer(&data, size));
  }
};

void
CloudServer::OnTrafficRequest(const Client &c, bool near)
{
  if (!near)
    /* "near" is the only selection flag we know */
    return;

  auto *client = clients.Find(c.key);
  if (client == nullptr)
    /* we don't send our data to clients who didn't sent anything to
       us yet */
    return;

  TrafficResponseSender s(*this, c);

  unsigned n = 0;
  for (const auto &traffic : clients.QueryWithinRange(client->location, 50000)) {
    if (traffic.get() == client)
      continue;

    s.Add(traffic->id, 0, //TODO: time?
          traffic->location, traffic->altitude);

    if (++n > 64)
      break;
  }

  s.Flush();
}

void
CloudServer::OnWaveSubmit(const Client &c,
                          std::chrono::milliseconds time_of_day,
                          const ::GeoPoint &a, const ::GeoPoint &b,
                          int bottom_altitude,
                          int top_altitude,
                          double lift)
{
  auto *client = clients.Find(c.key);
  if (client == nullptr)
    /* we don't trust the client if he didn't sent anything to us
       yet */
    return;

  cout << "WAVE\t"
       << client->endpoint << '\t'
       << std::hex << client->key << std::dec << '\t'
       << client->id << '\t'
       << a << '\t'
       << b << '\t'
       << bottom_altitude << '-' << top_altitude << "m\t"
       << lift << "m/s"
       << endl;
}

void
CloudServer::OnThermalSubmit(const Client &c,
                             std::chrono::milliseconds time_of_day,
                             const ::GeoPoint &bottom_location,
                             int bottom_altitude,
                             const ::GeoPoint &top_location,
                             int top_altitude,
                             double lift)
{
  auto *client = clients.Find(c.key);
  if (client == nullptr)
    /* we don't trust the client if he didn't sent anything to us
       yet */
    return;

  cout << "THERMAL\t"
       << client->endpoint << '\t'
       << std::hex << client->key << std::dec << '\t'
       << client->id << '\t'
       << top_location << '\t'
       << bottom_altitude << '-' << top_altitude << "m\t"
       << lift << "m/s"
       << endl;

  thermals.Make(c.key,
                AGeoPoint(bottom_location, bottom_altitude),
                AGeoPoint(top_location, top_altitude),
                lift);
}

class ThermalResponseSender {
  SkyLinesTracking::Server &server;
  const boost::asio::ip::udp::endpoint &endpoint;

  static constexpr size_t MAX_THERMAL_SIZE = 1024;
  static constexpr size_t MAX_THERMAL =
    MAX_THERMAL_SIZE / sizeof(SkyLinesTracking::Thermal);

  struct Packet {
    SkyLinesTracking::ThermalResponsePacket header;
    std::array<SkyLinesTracking::Thermal, MAX_THERMAL> thermal;
  } data;

  unsigned n_thermal = 0;

public:
  ThermalResponseSender(SkyLinesTracking::Server &_server,
                        const SkyLinesTracking::Server::Client &client)
    :server(_server), endpoint(client.endpoint) {
    data.header.header.magic = ToBE32(SkyLinesTracking::MAGIC);
    data.header.header.type = ToBE16(SkyLinesTracking::Type::THERMAL_RESPONSE);
    data.header.header.key = ToBE64(client.key);

    data.header.reserved1 = 0;
    data.header.reserved2 = 0;
    data.header.reserved3 = 0;
  }

  void Add(SkyLinesTracking::Thermal t) {
    assert(n_thermal < MAX_THERMAL);

    data.thermal[n_thermal++] = t;

    if (n_thermal == MAX_THERMAL)
      Flush();
  }

  void Flush() {
    if (n_thermal == 0)
      return;

    size_t size = sizeof(data.header) + sizeof(data.thermal[0]) * n_thermal;

    data.header.thermal_count = n_thermal;
    n_thermal = 0;

    data.header.header.crc = 0;
    data.header.header.crc = ToBE16(UpdateCRC16CCITT(&data, size, 0));
    server.SendBuffer(endpoint, boost::asio::const_buffer(&data, size));
  }
};

void
CloudServer::OnThermalRequest(const Client &c)
{
  auto *client = clients.Find(c.key);
  if (client == nullptr)
    /* we don't send our data to clients who didn't sent anything to
       us yet */
    return;

  ThermalResponseSender s(*this, c);

  unsigned n = 0;
  for (const auto &thermal : thermals.QueryWithinRange(client->location, 50000)) {
    if (thermal->client_key == c.key)
      /* ignore this client's own submissions - he knows them
         already */
      continue;

    // TODO: check time stamp, don't send old thermals

    s.Add(thermal->Pack());

    if (++n > 256)
      break;
  }

  s.Flush();
}

void
CloudServer::Load()
{
  FileReader fr(db_path);
  Deserialiser s(fr);
  CloudData::Load(s);
}

void
CloudServer::Save()
{
  cout << "Saving data to " << db_path.c_str() << endl;

  FileOutputStream fos(db_path);

  {
    Serialiser s(fos);
    CloudData::Save(s);
    s.Flush();
  }

  fos.Commit();
}

int
main(int argc, char **argv)
try {
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " DBPATH" << endl;
    return EXIT_FAILURE;
  }

  const Path db_path(argv[1]);

  boost::asio::io_service io_service;

  const boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(),
                                                CloudServer::GetDefaultPort());

  CloudServer server(db_path, io_service, endpoint);

  try {
    server.Load();
  } catch (const std::runtime_error &e) {
    cerr << "Failed to load database" << endl;
    PrintException(e);
  }

  io_service.run();

  server.Save();

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
