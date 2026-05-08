// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Data.hpp"
#include "Dump.hpp"
#include "OGNAprs.hpp"
#include "OGNClient.hpp"
#include "Sender.hpp"
#include "Serialiser.hpp"
#include "Tracking/SkyLines/Server.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "util/ByteOrder.hxx"
#include "event/Loop.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "event/SignalMonitor.hxx"
#include "event/net/cares/Channel.hxx"
#include "net/IPv4Address.hxx"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "util/PrintException.hxx"
#include "util/Exception.hxx"
#include "util/Compiler.h"
#include "util/ScopeExit.hxx"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <memory>

#include <signal.h>

// TODO: review these settings
static constexpr double TRAFFIC_RANGE = 50000;
static constexpr double THERMAL_RANGE = 50000;

static constexpr std::chrono::steady_clock::duration MAX_TRAFFIC_AGE =
  std::chrono::minutes(15);
static constexpr std::chrono::steady_clock::duration MAX_OGN_TRAFFIC_AGE =
  std::chrono::minutes(2);
static constexpr std::chrono::steady_clock::duration MAX_THERMAL_AGE =
  std::chrono::minutes(30);

static constexpr std::chrono::steady_clock::duration REQUEST_EXPIRY =
  std::chrono::minutes(5);

using std::cout;
using std::cerr;
using std::endl;

static uint32_t
MsUtcMidnight() noexcept
{
  using namespace std::chrono;
  const auto ms = duration_cast<milliseconds>(
                    system_clock::now().time_since_epoch())
                    .count();
  constexpr auto day_ms = 24LL * 60 * 60 * 1000;
  return uint32_t(ms % day_ms);
}

static const char *
EnvOr(const char *key, const char *fallback) noexcept
{
  const char *v = std::getenv(key);
  return (v != nullptr && v[0] != '\0') ? v : fallback;
}

class CloudServer final
  : public SkyLinesTracking::Server,
    CloudData,
    public OGNAprsHandler {
  const AllocatedPath db_path;

  Cares::Channel cares_channel;

  CoarseTimerEvent save_timer, expire_timer;
  CoarseTimerEvent ogn_expire_timer;

  std::unique_ptr<OGNClient> ogn_client;

public:
  CloudServer(AllocatedPath &&_db_path, EventLoop &event_loop,
              SocketAddress bind_address, bool enable_ogn)
    :SkyLinesTracking::Server(event_loop, bind_address),
     db_path(std::move(_db_path)),
     cares_channel(event_loop),
     save_timer(event_loop, BIND_THIS_METHOD(OnSaveTimer)),
     expire_timer(event_loop, BIND_THIS_METHOD(OnExpireTimer)),
     ogn_expire_timer(event_loop, BIND_THIS_METHOD(OnOgnExpireTimer))
  {
#ifndef _WIN32
    SignalMonitorRegister(SIGINT, BIND_THIS_METHOD(OnQuitSignal));
    SignalMonitorRegister(SIGTERM, BIND_THIS_METHOD(OnQuitSignal));
    SignalMonitorRegister(SIGQUIT, BIND_THIS_METHOD(OnQuitSignal));

    SignalMonitorRegister(SIGHUP, BIND_THIS_METHOD(OnReloadSignal));
    SignalMonitorRegister(SIGUSR1, BIND_THIS_METHOD(OnDumpSignal));
#endif

    ScheduleSave();

    if (enable_ogn) {
      std::string host{EnvOr("XCS_CLOUD_OGN_HOST", "aprs.glidernet.org")};
      const unsigned port =
        unsigned(std::strtoul(EnvOr("XCS_CLOUD_OGN_PORT", "10152"), nullptr, 10));
      std::string user{EnvOr("XCS_CLOUD_OGN_USER", "N0CALL")};
      std::string pass{EnvOr("XCS_CLOUD_OGN_PASS", "-1")};

      ogn_client = std::make_unique<OGNClient>(
        event_loop, cares_channel, *this,
        std::move(host), port, std::move(user), std::move(pass));
      ogn_client->Start();
      ScheduleOgnExpire();
    }
  }

  ~CloudServer() noexcept
  {
    if (ogn_client != nullptr)
      ogn_client->Stop();
  }

  void Load();
  void Save();

private:
  void OnSaveTimer() noexcept {
    Save();
    ScheduleSave();
  }

  void ScheduleSave() {
    save_timer.Schedule(std::chrono::minutes(1));
  }

  void OnExpireTimer() noexcept {
    clients.Expire(GetEventLoop().SteadyNow() - std::chrono::minutes(10));
    if (!clients.empty())
      ScheduleExpire();
  }

  void ScheduleExpire() {
    expire_timer.Schedule(std::chrono::minutes(5));
  }

  void OnOgnExpireTimer() noexcept {
    ogn_traffic.Expire(GetEventLoop().SteadyNow() - MAX_OGN_TRAFFIC_AGE);
    ScheduleOgnExpire();
  }

  void ScheduleOgnExpire() noexcept {
    ogn_expire_timer.Schedule(std::chrono::minutes(1));
  }

  void PushOgnTraffic(const OGNTrafficEntry &t) noexcept;

  /* OGNAprsHandler */
  void OnAprsLine(std::string_view line) noexcept override;

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

  void OnSendError(SocketAddress address,
                   std::exception_ptr e) noexcept override {
    cerr << "Failed to send to " << address
         << ": " << GetFullMessage(e)
         << endl;
  }

  void OnError(std::exception_ptr e) override {
    cerr << GetFullMessage(e) << endl;
    GetEventLoop().Break();
  }

#ifndef _WIN32
  void OnQuitSignal() noexcept {
    GetEventLoop().Break();
  }

  void OnReloadSignal() noexcept {
    Save();
  }

  void OnDumpSignal() noexcept {
    DumpClients();
  }
#endif
};

void
CloudServer::OnAprsLine(std::string_view line) noexcept
{
  const OGNAprsParseResult p = ParseOGNAprsLine(line);
  if (!p.valid || !p.location.IsValid())
    return;

  OGNTrafficEntry &t =
    ogn_traffic.Upsert(p.station_id, p.location, p.altitude,
                       p.track_deg, p.track_valid,
                       p.flarm_id, p.flarm_valid,
                       p.aircraft_type);
  PushOgnTraffic(t);
}

void
CloudServer::PushOgnTraffic(const OGNTrafficEntry &t) noexcept
{
  const auto now = std::chrono::steady_clock::now();
  const auto min_stamp = now - MAX_TRAFFIC_AGE;
  if (t.stamp < min_stamp)
    return;

  const TrafficRecordExtensions ext =
    TrafficRecordExtensions::FromOgn(t.track_deg, t.track_valid,
                                   t.aircraft_type,
                                   t.flarm_id, t.flarm_valid);
  const uint32_t time_ms = MsUtcMidnight();
  const uint32_t pilot = OGNPilotIdFromStation(t.station_id);

  for (const auto &i : clients.QueryWithinRange(t.location, TRAFFIC_RANGE)) {
    if (now > i->wants_traffic)
      continue;

    TrafficResponseSender s(*this, i->address, i->key);
    s.Add(pilot, time_ms, t.location, t.altitude, ext);
    s.Flush();
  }
}

void
CloudServer::OnFix(const Client &c,
                   std::chrono::milliseconds time_of_day,
                   const ::GeoPoint &location, int altitude)
{
  (void)time_of_day; // TODO: use this parameter

  CloudClient *client;
  if (location.IsValid()) {
    bool was_empty = clients.empty();

    client = &clients.Make(c.address, c.key, location, altitude);

    cout << "FIX\t"
         << client->address << '\t'
         << std::hex << client->key << std::dec << '\t'
         << client->id << '\t'
         << client->location << '\t'
         << client->altitude << 'm'
         << endl;

    if (was_empty)
      ScheduleExpire();
  } else {
    client = clients.Find(c.key);
    if (client != nullptr)
      clients.Refresh(*client, c.address);
  }

  /* send this new traffic location to all interested clients
     immediately */
  const auto now = std::chrono::steady_clock::now();
  for (const auto &i : clients.QueryWithinRange(location, TRAFFIC_RANGE)) {
    if (i->key == c.key)
      /* ignore this client's own submissions - he knows them
         already */
      continue;

    if (now > i->wants_traffic)
      /* not interested (anymore) */
      continue;

    TrafficResponseSender s(*this, i->address, i->key);
    s.Add(client->id, 0, //TODO: time?
          client->location, client->altitude);
    s.Flush();
  }
}

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

  const auto now = std::chrono::steady_clock::now();

  client->wants_traffic = now + REQUEST_EXPIRY;

  const auto min_stamp = now - MAX_TRAFFIC_AGE;

  TrafficResponseSender s(*this, c.address, c.key);

  unsigned n = 0;
  for (const auto &traffic : clients.QueryWithinRange(client->location,
                                                      TRAFFIC_RANGE)) {
    if (traffic.get() == client)
      continue;

    if (traffic->stamp < min_stamp)
      /* don't send stale traffic, it's probably not there anymore */
      continue;

    s.Add(traffic->id, 0, //TODO: time?
          traffic->location, traffic->altitude);

    if (++n > 64)
      break;
  }

  if (n <= 64) {
    const uint32_t time_ms = MsUtcMidnight();
    for (const auto &og :
         ogn_traffic.QueryWithinRange(client->location, TRAFFIC_RANGE)) {
      if (og->stamp < min_stamp)
        continue;

      const TrafficRecordExtensions ext =
        TrafficRecordExtensions::FromOgn(og->track_deg, og->track_valid,
                                         og->aircraft_type,
                                         og->flarm_id, og->flarm_valid);
      s.Add(OGNPilotIdFromStation(og->station_id),
            time_ms,
            og->location, og->altitude, ext);

      if (++n > 64)
        break;
    }
  }

  s.Flush();
}

void
CloudServer::OnWaveSubmit(const Client &c,
                          [[maybe_unused]] std::chrono::milliseconds time_of_day,
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
       << client->address << '\t'
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
                             [[maybe_unused]] std::chrono::milliseconds time_of_day,
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
       << client->address << '\t'
       << std::hex << client->key << std::dec << '\t'
       << client->id << '\t'
       << top_location << '\t'
       << bottom_altitude << '-' << top_altitude << "m\t"
       << lift << "m/s"
       << endl;

  const auto &thermal =
    thermals.Make(c.key,
                  AGeoPoint(bottom_location, bottom_altitude),
                  AGeoPoint(top_location, top_altitude),
                  lift);

  /* send this new thermal to all interested clients immediately */
  const auto now = std::chrono::steady_clock::now();
  for (const auto &i : clients.QueryWithinRange(bottom_location,
                                                THERMAL_RANGE)) {
    if (i->key == c.key)
      /* ignore this client's own submissions - he knows them
         already */
      continue;

    if (now > i->wants_thermals)
      /* not interested (anymore) */
      continue;

    ThermalResponseSender s(*this, i->address, i->key);
    s.Add(thermal.Pack());
    s.Flush();
  }
}

void
CloudServer::OnThermalRequest(const Client &c)
{
  auto *client = clients.Find(c.key);
  if (client == nullptr)
    /* we don't send our data to clients who didn't sent anything to
       us yet */
    return;

  const auto now = std::chrono::steady_clock::now();

  client->wants_thermals = now + REQUEST_EXPIRY;

  const auto min_time = now - MAX_THERMAL_AGE;

  ThermalResponseSender s(*this, c.address, c.key);

  unsigned n = 0;
  for (const auto &thermal : thermals.QueryWithinRange(client->location,
                                                       THERMAL_RANGE)) {
    if (thermal->client_key == c.key)
      /* ignore this client's own submissions - he knows them
         already */
      continue;

    if (thermal->time < min_time)
      /* don't send old thermals, they're useless */
      continue;

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
    cerr << "Optional env: XCS_CLOUD_OGN=1 enables OGN/APRS-IS ingest "
            "(see XCS_CLOUD_OGN_* variables)."
         << endl;
    return EXIT_FAILURE;
  }

  const Path db_path(argv[1]);

  EventLoop event_loop;
  SignalMonitorInit(event_loop);
  AtScopeExit() { SignalMonitorFinish(); };

  const bool enable_ogn =
    std::strcmp(EnvOr("XCS_CLOUD_OGN", "0"), "1") == 0;

  CloudServer server(db_path, event_loop,
                     IPv4Address(CloudServer::GetDefaultPort()),
                     enable_ogn);

  try {
    server.Load();
  } catch (const std::runtime_error &e) {
    cerr << "Failed to load database" << endl;
    PrintException(e);
  }

  event_loop.Run();

  server.Save();

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
