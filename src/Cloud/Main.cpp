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
#include "net/IPv6Address.hxx"
#include "net/ToString.hxx"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "util/PrintException.hxx"
#include "util/Exception.hxx"
#include "util/Compiler.h"
#include "util/ScopeExit.hxx"
#include "util/EnvParser.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <optional>

#include <signal.h>

// TODO: review these settings
static constexpr double TRAFFIC_RANGE = 50000;
static constexpr double THERMAL_RANGE = 50000;

/** Max |target − client| altitude [m] when relaying traffic to a client. */
static constexpr int MAX_TRAFFIC_ALTITUDE_SEPARATION = 5000;

static constexpr std::chrono::steady_clock::duration MAX_TRAFFIC_AGE =
  std::chrono::minutes(15);
static constexpr std::chrono::steady_clock::duration MAX_OGN_TRAFFIC_AGE =
  std::chrono::minutes(2);
static constexpr std::chrono::steady_clock::duration MAX_THERMAL_AGE =
  std::chrono::minutes(30);

static constexpr std::chrono::steady_clock::duration REQUEST_EXPIRY =
  std::chrono::minutes(5);

static constexpr std::chrono::steady_clock::duration TRAFFIC_PUSH_INTERVAL =
  std::chrono::seconds(15);

static constexpr unsigned MAX_TRAFFIC_TARGETS_PER_RESPONSE = 64;

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

/**
 * Whether @p target is traffic relevant to a client at @p client_location
 * and @p client_altitude: within #TRAFFIC_RANGE horizontally and within
 * #MAX_TRAFFIC_ALTITUDE_SEPARATION vertically when both altitudes are known.
 */
[[gnu::pure]]
static bool
IsTrafficRelevantToClient(const GeoPoint &client_location,
                          int client_altitude,
                          const GeoPoint &target_location,
                          int target_altitude,
                          bool target_altitude_valid) noexcept
{
  if (!target_location.IsValid())
    return false;

  if (client_location.Distance(target_location) > TRAFFIC_RANGE)
    return false;

  if (client_altitude < 0) {
    /* Without own altitude, drop targets that report altitude (e.g.
       high-altitude ADSB) instead of relaying the full horizontal set. */
    return !target_altitude_valid;
  }

  if (!target_altitude_valid)
    return false;

  const int separation = target_altitude - client_altitude;
  return separation <= MAX_TRAFFIC_ALTITUDE_SEPARATION &&
    separation >= -MAX_TRAFFIC_ALTITUDE_SEPARATION;
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
      std::string host{GetEnvString("XCS_CLOUD_OGN_HOST", "aprs.glidernet.org")};
      const unsigned port = unsigned(
        GetEnvInt("XCS_CLOUD_OGN_PORT", 10152, 1, 65535));
      std::string user{GetEnvString("XCS_CLOUD_OGN_USER", "N0CALL")};
      std::string pass{GetEnvString("XCS_CLOUD_OGN_PASS", "-1")};

      const bool user_configured = user != "N0CALL";

      cout << "OGN\tenabled\thost=" << host << "\tport=" << port
           << "\tuser=" << (user_configured ? "configured" : "default")
           << endl;

      ogn_client = std::make_unique<OGNClient>(
        event_loop, cares_channel, *this,
        std::move(host), port, std::move(user), std::move(pass));
      ogn_client->Start();
      ScheduleOgnExpire();
    } else
      cout << "OGN\tdisabled" << endl;
  }

  ~CloudServer() noexcept
  {
    if (ogn_client != nullptr)
      ogn_client->Stop();
  }

  void Load();
  void Save();

  void SaveSafely() noexcept {
    try {
      Save();
    } catch (...) {
      cerr << "Failed to save database: "
           << GetFullMessage(std::current_exception()) << endl;
    }
  }

private:
  void OnSaveTimer() noexcept {
    SaveSafely();
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

  void SendTrafficCallsign(SocketAddress address, uint64_t key,
                           uint32_t pilot_id,
                           const std::string &callsign) noexcept;

  void SendNearTrafficSnapshot(CloudClient &client,
                               const char *reason) noexcept;

  void MaybeSendNearTrafficSnapshot(CloudClient &client,
                                    const char *reason,
                                    bool force = false) noexcept;

  template<typename F>
  void ForEachClientNearTraffic(
    const GeoPoint &target_location, int target_altitude,
    bool target_altitude_valid,
    std::optional<uint64_t> exclude_key, F &&f) noexcept(
    noexcept(f(std::declval<CloudClient &>()))) {
    for (const auto &i : clients.QueryWithinRange(target_location,
                                                  TRAFFIC_RANGE)) {
      if (exclude_key && i->key == *exclude_key)
        continue;

      if (!IsTrafficRelevantToClient(i->location, i->altitude,
                                     target_location, target_altitude,
                                     target_altitude_valid))
        continue;

      f(*i);
    }
  }

  /* OGNAprsHandler */
  void OnAprsLine(std::string_view line) noexcept override;

protected:
  /* virtual methods from class SkyLinesTracking::Server */
  void OnFix(const Client &client,
             std::chrono::milliseconds time_of_day,
             const ::GeoPoint &location, int altitude,
             unsigned track_deg, bool track_valid) override;

  void OnTrafficRequest(const Client &client,
                        bool near) override;

  void OnUserNameRequest(const Client &client,
                         uint32_t user_id) override;

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
    SaveSafely();
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
  if (!p.valid) {
    if (GetEnvBool("XCS_CLOUD_DEBUG"))
      cerr << "OGN\tignore\t" << line << endl;
    return;
  }

  if (!IsForwardableOgnTraffic(p, line)) {
    if (GetEnvBool("XCS_CLOUD_DEBUG"))
      cerr << "OGN\tground-station\t" << p.station_id << endl;
    return;
  }

  try {
    OGNTrafficEntry &t =
      ogn_traffic.Upsert(p.station_id, p.location, p.altitude,
                         p.altitude_valid,
                         p.track_deg, p.track_valid,
                         p.flarm_id, p.flarm_valid,
                         p.aircraft_type, p.address_type, p.callsign);

    if (GetEnvBool("XCS_CLOUD_DEBUG")) {
      cout << "OGN\ttraffic\t" << p.station_id << '\t'
           << t.location << '\t' << t.altitude << "m\t"
           << "pilot_id=" << t.pilot_id;
      if (p.aircraft_type != 0)
        cout << "\ttype=" << p.aircraft_type;
      if (p.track_valid)
        cout << "\ttrack=" << p.track_deg;
      if (!t.callsign.empty())
        cout << "\tcallsign=" << t.callsign;
      cout << endl;
    }

    PushOgnTraffic(t);
  } catch (...) {
    cerr << "OGN\talloc-error\t" << p.station_id << endl;
  }
}

void
CloudServer::SendTrafficCallsign(SocketAddress address, uint64_t key,
                                 uint32_t pilot_id,
                                 const std::string &callsign) noexcept
{
  if (callsign.empty())
    return;

  SendUserNameResponse(*this, address, key, pilot_id, callsign);

  if (GetEnvBool("XCS_CLOUD_DEBUG"))
    cerr << "USER_NAME\tpush\tpilot_id=" << pilot_id
         << "\tname=" << callsign << endl;
}

void
CloudServer::PushOgnTraffic(const OGNTrafficEntry &t) noexcept
{
  const auto now = std::chrono::steady_clock::now();
  const auto min_stamp = now - MAX_OGN_TRAFFIC_AGE;
  if (t.stamp < min_stamp)
    return;

  ForEachClientNearTraffic(t.location, t.altitude, t.altitude_valid, {},
                           [&](CloudClient &i) {
                             MaybeSendNearTrafficSnapshot(i, "TRAFFIC_OGN");
                           });
}

void
CloudServer::MaybeSendNearTrafficSnapshot(CloudClient &client,
                                          const char *reason,
                                          bool force) noexcept
{
  const auto now = std::chrono::steady_clock::now();
  if (!force && now < client.last_traffic_push + TRAFFIC_PUSH_INTERVAL)
    return;

  client.last_traffic_push = now;
  SendNearTrafficSnapshot(client, reason);
}

void
CloudServer::SendNearTrafficSnapshot(CloudClient &client,
                                       const char *reason) noexcept
{
  const auto now = std::chrono::steady_clock::now();
  const auto min_stamp = now - MAX_TRAFFIC_AGE;
  const auto min_ogn_stamp = now - MAX_OGN_TRAFFIC_AGE;

  TrafficResponseSender s(*this, client.address, client.key);

  unsigned n = 0;
  unsigned n_cloud = 0;
  unsigned n_ogn = 0;
  for (const auto &traffic : clients.QueryWithinRange(client.location,
                                                      TRAFFIC_RANGE)) {
    if (traffic.get() == &client)
      continue;

    if (traffic->stamp < min_stamp)
      continue;

    if (!IsTrafficRelevantToClient(client.location, client.altitude,
                                   traffic->location, traffic->altitude,
                                   traffic->altitude >= 0))
      continue;

    if (n >= MAX_TRAFFIC_TARGETS_PER_RESPONSE)
      break;

    s.Add(traffic->id, 0, //TODO: time?
          traffic->location, traffic->altitude,
          TrafficRecordExtensions::FromOgn(traffic->track_deg,
                                           traffic->track_valid,
                                           traffic->aircraft_type,
                                           0, false,
                                           traffic->altitude >= 0));
    ++n_cloud;
    ++n;
  }

  if (n < MAX_TRAFFIC_TARGETS_PER_RESPONSE) {
    const uint32_t time_ms = MsUtcMidnight();
    for (const auto &og :
         ogn_traffic.QueryWithinRange(client.location, TRAFFIC_RANGE)) {
      if (og->stamp < min_ogn_stamp)
        continue;

      if (!IsForwardableOgnTraffic(*og))
        continue;

      if (!IsTrafficRelevantToClient(client.location, client.altitude,
                                     og->location, og->altitude,
                                     og->altitude_valid))
        continue;

      if (n >= MAX_TRAFFIC_TARGETS_PER_RESPONSE)
        break;

      s.Add(og->pilot_id, time_ms, og->location, og->altitude,
            TrafficRecordExtensions::FromOgn(*og));
      SendTrafficCallsign(client.address, client.key,
                          og->pilot_id, og->callsign);
      ++n_ogn;
      ++n;
    }
  }

  s.Flush();

  if (n > 0 || std::strcmp(reason, "TRAFFIC_FIX") == 0) {
    cout << reason << '\t' << client.address << '\t'
         << std::hex << client.key << std::dec << '\t'
         << client.id << '\t' << client.location << '\t'
         << "alt=" << client.altitude << "m\t"
         << "cloud=" << n_cloud << "\togn=" << n_ogn
         << "\ttotal=" << n << endl;
  }
}

void
CloudServer::OnFix(const Client &c,
                   std::chrono::milliseconds time_of_day,
                   const ::GeoPoint &location, int altitude,
                   unsigned track_deg, bool track_valid)
{
  (void)time_of_day; // TODO: use this parameter

  CloudClient *client;
  if (location.IsValid()) {
    bool was_empty = clients.empty();

    client = &clients.Make(c.address, c.key, location, altitude,
                           track_deg, track_valid);

    cout << "FIX\t"
         << client->address << '\t'
         << std::hex << client->key << std::dec << '\t'
         << client->id << '\t'
         << client->location << '\t'
         << client->altitude << 'm';
    if (track_valid)
      cout << "\ttrack=" << track_deg;
    cout << endl;

    if (was_empty)
      ScheduleExpire();
  } else {
    client = clients.Find(c.key);
    if (client != nullptr)
      clients.Refresh(*client, c.address);
  }

  if (client == nullptr || !location.IsValid())
    return;

  /* Always push a full nearby snapshot to the client that sent the FIX. */
  MaybeSendNearTrafficSnapshot(*client, "TRAFFIC_FIX", true);

  /* Push full snapshots to other nearby clients (same as OGN updates). */
  ForEachClientNearTraffic(location, altitude, altitude >= 0, c.key,
                           [&](CloudClient &i) {
                             MaybeSendNearTrafficSnapshot(i, "TRAFFIC_FIX", true);
                           });
}

void
CloudServer::OnTrafficRequest(const Client &c, bool near)
{
  if (!near) {
    if (GetEnvBool("XCS_CLOUD_DEBUG"))
      cerr << "TRAFFIC_REQUEST\tignored\tnot-near\t"
           << std::hex << c.key << std::dec << endl;
    return;
  }

  auto *client = clients.Find(c.key);
  if (client == nullptr) {
    cerr << "TRAFFIC_REQUEST\trejected\tunknown-client\t"
         << ToString(c.address) << '\t'
         << std::hex << c.key << std::dec << endl;
    return;
  }

  MaybeSendNearTrafficSnapshot(*client, "TRAFFIC_REQUEST", true);
}

void
CloudServer::OnUserNameRequest(const Client &c, uint32_t user_id)
{
  const OGNTrafficEntry *traffic = ogn_traffic.FindByPilotId(user_id);
  if (traffic == nullptr || traffic->callsign.empty())
    return;

  SendTrafficCallsign(c.address, c.key, user_id, traffic->callsign);
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

    if (n >= 256)
      break;

    s.Add(thermal->Pack());
    ++n;
  }

  s.Flush();

  cout << "THERMAL_REQUEST\t" << client->address << '\t'
       << std::hex << client->key << std::dec << '\t'
       << client->id << '\t' << client->location << '\t'
       << "count=" << n << endl;
}

void
CloudServer::Load()
{
  FileReader fr(db_path);
  Deserialiser s(fr);
  CloudData::Load(s);

  cout << "DB\tloaded\tclients="
       << std::distance(clients.begin(), clients.end())
       << "\tthermals="
       << std::distance(thermals.begin(), thermals.end()) << endl;
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
    cerr << "Optional env: XCS_CLOUD_DEBUG=1 enables verbose debug logging."
         << endl;
    return EXIT_FAILURE;
  }

  const Path db_path(argv[1]);

  EventLoop event_loop;
  SignalMonitorInit(event_loop);
  AtScopeExit() { SignalMonitorFinish(); };

  const bool enable_ogn = GetEnvBool("XCS_CLOUD_OGN");

  std::unique_ptr<CloudServer> server;
  const char *bind_mode;
  try {
    server = std::make_unique<CloudServer>(db_path, event_loop,
                                           IPv6Address(CloudServer::GetDefaultPort()),
                                           enable_ogn);
    bind_mode = "ipv6-dual-stack";
  } catch (...) {
    cerr << "IPv6 bind failed, falling back to IPv4" << endl;
    server = std::make_unique<CloudServer>(db_path, event_loop,
                                           IPv4Address(CloudServer::GetDefaultPort()),
                                           enable_ogn);
    bind_mode = "ipv4";
  }

  cout << "START\tport=" << CloudServer::GetDefaultPort()
       << "\tbind=" << bind_mode
       << "\tdb=" << db_path.c_str()
       << "\tdebug=" << (GetEnvBool("XCS_CLOUD_DEBUG") ? "1" : "0") << endl;

  try {
    server->Load();
  } catch (const std::runtime_error &e) {
    cerr << "Failed to load database" << endl;
    PrintException(e);
  }

  event_loop.Run();

  server->Save();

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
