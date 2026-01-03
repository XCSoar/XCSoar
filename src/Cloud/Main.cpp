// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Data.hpp"
#include "Dump.hpp"
#include "Sender.hpp"
#include "Serialiser.hpp"
#include "OGNClient.hpp"
#include "Tracking/SkyLines/Server.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "util/ByteOrder.hxx"
#include "util/CRC16CCITT.hpp"
#include "util/SpanCast.hxx"
#include "util/ConvertString.hpp"
#include "event/Loop.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "event/SignalMonitor.hxx"
#include "event/net/cares/Channel.hxx"
#include "net/IPv4Address.hxx"
#include "net/SocketAddress.hxx"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "util/PrintException.hxx"
#include "util/Exception.hxx"
#include "util/Compiler.h"
#include "util/ScopeExit.hxx"
#include "util/StaticString.hxx"

#include <array>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <signal.h>

// TODO: review these settings
static constexpr double TRAFFIC_RANGE = 500000; // 500km

/**
 * Calculate milliseconds since midnight UTC from current system time.
 */
static uint32_t
GetTimeOfDayMs() noexcept
{
  const auto now = std::chrono::system_clock::now();
  const auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
    now.time_since_epoch()).count();
  const auto ms_since_midnight = ms_since_epoch % (24 * 60 * 60 * 1000);
  
  return (uint32_t)ms_since_midnight;
}
static constexpr double THERMAL_RANGE = 50000;

static constexpr std::chrono::steady_clock::duration MAX_TRAFFIC_AGE = std::chrono::minutes(15);
static constexpr std::chrono::steady_clock::duration MAX_THERMAL_AGE = std::chrono::minutes(30);

static constexpr std::chrono::steady_clock::duration REQUEST_EXPIRY = std::chrono::minutes(5);

using std::cout;
using std::cerr;
using std::endl;

class CloudServer final
  : public SkyLinesTracking::Server, CloudData
{
  const AllocatedPath db_path;

  CoarseTimerEvent save_timer, expire_timer, ogn_expire_timer;

  Cares::Channel cares_channel;
  OGNClient ogn_client;

public:
  CloudServer(AllocatedPath &&_db_path, EventLoop &event_loop,
              SocketAddress bind_address)
    :SkyLinesTracking::Server(event_loop, bind_address),
     db_path(std::move(_db_path)),
     save_timer(event_loop, BIND_THIS_METHOD(OnSaveTimer)),
     expire_timer(event_loop, BIND_THIS_METHOD(OnExpireTimer)),
     ogn_expire_timer(event_loop, BIND_THIS_METHOD(OnOGNExpireTimer)),
     cares_channel(event_loop),
     ogn_client(event_loop, cares_channel, ogn_traffic)
  {
#ifndef _WIN32
    SignalMonitorRegister(SIGINT, BIND_THIS_METHOD(OnQuitSignal));
    SignalMonitorRegister(SIGTERM, BIND_THIS_METHOD(OnQuitSignal));
    SignalMonitorRegister(SIGQUIT, BIND_THIS_METHOD(OnQuitSignal));

    SignalMonitorRegister(SIGHUP, BIND_THIS_METHOD(OnReloadSignal));
    SignalMonitorRegister(SIGUSR1, BIND_THIS_METHOD(OnDumpSignal));
#endif

    ScheduleSave();

    // OGN client - use full feed (port 10152)
    ogn_client.SetServer("aprs.glidernet.org", 10152);
    ogn_client.SetEnabled(true);  // Connect immediately

    ScheduleOGNExpire();
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

  void OnOGNExpireTimer() noexcept {
    ogn_traffic.Expire(GetEventLoop().SteadyNow() - MAX_TRAFFIC_AGE);
    if (!ogn_traffic.empty())
      ScheduleOGNExpire();
  }

  void ScheduleOGNExpire() {
    ogn_expire_timer.Schedule(std::chrono::minutes(5));
  }

protected:
  /* virtual methods from class SkyLinesTracking::Server */
  void OnFix(const Client &client,
             std::chrono::milliseconds time_of_day,
             const ::GeoPoint &location, int altitude) override;

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
    Save();
  }

  void OnDumpSignal() noexcept {
    DumpClients();
  }
#endif
};

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
    if (client != nullptr) {
      GeoPoint old_location = client->location;
      clients.Refresh(*client, c.address);
      // Update OGN filter if client location changed significantly
      // (Refresh only updates address, not location, so skip filter update)
      (void)old_location;
    }
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
  cout << "TRAFFIC_REQUEST_RAW\t"
       << static_cast<SocketAddress>(c.address) << '\t'
       << std::hex << c.key << std::dec << '\t'
       << "near=" << (near ? "true" : "false")
       << endl;

  if (!near)
    /* "near" is the only selection flag we know */
    return;

  auto *client = clients.Find(c.key);
  if (client == nullptr) {
    cout << "TRAFFIC_REQUEST_REJECTED\t"
         << "client not found for key="
         << std::hex << c.key << std::dec
         << endl;
    /* we don't send our data to clients who didn't sent anything to
       us yet */
    return;
  }

  const auto now = std::chrono::steady_clock::now();

  client->wants_traffic = now + REQUEST_EXPIRY;

  cout << "TRAFFIC_REQUEST\t"
       << client->address << '\t'
       << std::hex << client->key << std::dec << '\t'
       << "id=" << client->id << '\t'
       << client->location << '\t'
       << client->altitude << "m"
       << endl;

  const auto min_stamp = now - MAX_TRAFFIC_AGE;

  TrafficResponseSender s(*this, c.address, c.key);

  unsigned n = 0;

  // Add XCSoar client traffic
  for (const auto &traffic : clients.QueryWithinRange(client->location,
                                                      TRAFFIC_RANGE)) {
    if (traffic.get() == client)
      continue;

    if (traffic->stamp < min_stamp)
      /* don't send stale traffic, it's probably not there anymore */
      continue;

    // Calculate time-of-day: current time minus age of traffic
    auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - traffic->stamp).count();
    uint32_t time_of_day = GetTimeOfDayMs();
    if (age_ms < (int64_t)time_of_day) {
      time_of_day -= (uint32_t)age_ms;
    } else {
      // Handle wraparound (traffic older than current time-of-day)
      time_of_day = 0;
    }

    s.Add(traffic->id, time_of_day,
          traffic->location, traffic->altitude);

    cout << "TRAFFIC_SEND\tXCSoar\t"
         << std::hex << client->key << std::dec << '\t'
         << "id=" << traffic->id << '\t'
         << traffic->location << '\t'
         << traffic->altitude << "m"
         << endl;

    if (++n > 64)
      break;
  }

  // Add OGN traffic
  unsigned ogn_count = 0;
  unsigned ogn_expired = 0;
  
  cout << "TRAFFIC_QUERY\tOGN\t"
       << "client_location=" << client->location
       << " range=" << TRAFFIC_RANGE << "m"
       << endl;
  
  // Debug: check if container is empty
  if (ogn_traffic.empty()) {
    cout << "TRAFFIC_QUERY\tOGN\tcontainer is empty" << endl;
  }
  
  unsigned query_count = 0;
  for (const auto &ogn : ogn_traffic.QueryWithinRange(client->location,
                                                       TRAFFIC_RANGE)) {
    query_count++;
    
    // Debug: log all items found
    auto distance = client->location.Distance(ogn->location);
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
      now - ogn->stamp).count();
    cout << "TRAFFIC_FOUND\tOGN\tdevice=" << ogn->device_id.c_str()
         << " distance=" << (int)distance << "m"
         << " location=" << ogn->location
         << " alt=" << ogn->altitude << "m"
         << " stamp_age=" << age << "s"
         << endl;
    
    if (ogn->stamp < min_stamp) {
      ogn_expired++;
      cout << "TRAFFIC_FILTER\tOGN\t" << ogn->device_id.c_str()
           << " expired (age=" << age << "s)" << endl;
      continue;
    }
    
    // Debug: log all matches that will be sent
    cout << "TRAFFIC_MATCH\tOGN\t" << ogn->device_id.c_str()
         << " distance=" << (int)distance << "m"
         << " location=" << ogn->location
         << " alt=" << ogn->altitude << "m"
         << endl;

    // Calculate time-of-day: current time minus age of traffic
    auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - ogn->stamp).count();
    uint32_t time_of_day = GetTimeOfDayMs();
    if (age_ms < (int64_t)time_of_day) {
      time_of_day -= (uint32_t)age_ms;
    } else {
      // Handle wraparound (traffic older than current time-of-day)
      time_of_day = 0;
    }

    s.Add(ogn->id, time_of_day,
          ogn->location, ogn->altitude, ogn->flarm_id,
          ogn->track, ogn->turn_rate, ogn->aircraft_type);

    cout << "TRAFFIC_SEND\tOGN\t"
         << std::hex << client->key << std::dec << '\t'
         << "id=" << ogn->id << '\t'
         << "flarm_id=" << ogn->flarm_id << '\t'
         << "device=" << ogn->device_id.c_str() << '\t'
         << ogn->location << '\t'
         << ogn->altitude << "m\t"
         << ogn->track << "°\t"
         << ogn->speed << "m/s\t"
         << ogn->climb_rate << "m/s\t"
         << "turn=" << ogn->turn_rate << "deg/s\t"
         << "type=" << (unsigned)ogn->aircraft_type
         << endl;

    ogn_count++;
    if (++n > 64)
      break;
  }

  cout << "TRAFFIC_SEND\tSummary\t"
       << std::hex << client->key << std::dec << '\t'
       << "total=" << n << '\t'
       << "ogn=" << ogn_count
       << " (query_found=" << query_count
       << " expired=" << ogn_expired << ")"
       << endl;

  s.Flush();
}

void
CloudServer::OnUserNameRequest(const Client &c, uint32_t user_id)
{
  // Check if this is an OGN traffic ID (high IDs starting at 0x80000000)
  if (user_id >= 0x80000000) {
    auto *ogn = ogn_traffic.FindById(user_id);
    if (ogn != nullptr) {
      // Send USER_NAME_RESPONSE with OGN device identifier
      const tstring &device_id = ogn->device_id;
      
      // Convert device_id to UTF-8
      WideToUTF8Converter device_id_utf8(device_id.c_str());
      const size_t name_length = strlen(device_id_utf8);
      
      // Construct USER_NAME_RESPONSE packet with name data
      struct UserNameResponsePacket {
        SkyLinesTracking::Header header;
        uint32_t user_id;
        uint32_t flags;
        uint32_t club_id;
        uint8_t name_length;
        uint8_t reserved1, reserved2, reserved3;
        uint32_t reserved4, reserved5;
        char name[256];  // Max name length
      } packet;
      
      if (name_length > sizeof(packet.name))
        return;  // Name too long
      
      packet.header.magic = ToBE32(SkyLinesTracking::MAGIC);
      packet.header.crc = 0;
      packet.header.type = ToBE16(SkyLinesTracking::Type::USER_NAME_RESPONSE);
      packet.header.key = ToBE64(c.key);
      packet.user_id = ToBE32(user_id);
      packet.flags = 0;  // Not NOT_FOUND
      packet.club_id = 0;
      packet.name_length = name_length;
      packet.reserved1 = 0;
      packet.reserved2 = 0;
      packet.reserved3 = 0;
      packet.reserved4 = 0;
      packet.reserved5 = 0;
      memcpy(packet.name, device_id_utf8, name_length);
      
      // Calculate CRC over entire packet (header + name)
      const size_t packet_size = sizeof(packet.header) + sizeof(packet.user_id) +
        sizeof(packet.flags) + sizeof(packet.club_id) + sizeof(packet.name_length) +
        sizeof(packet.reserved1) + sizeof(packet.reserved2) + sizeof(packet.reserved3) +
        sizeof(packet.reserved4) + sizeof(packet.reserved5) + name_length;
      packet.header.crc = ToBE16(UpdateCRC16CCITT(
        std::span<const std::byte>(reinterpret_cast<const std::byte *>(&packet),
                                    packet_size), 0));
      
      // Send entire packet (header + name)
      SendBuffer(c.address, std::span<const std::byte>(
        reinterpret_cast<const std::byte *>(&packet), packet_size));
      
      cout << "USER_NAME_RESPONSE\t"
           << std::hex << c.key << std::dec << '\t'
           << "user_id=" << user_id << '\t'
           << "device_id=" << device_id.c_str()
           << endl;
      return;
    }
  }
  
  // Not found - send NOT_FOUND response
  struct UserNameResponsePacket {
    SkyLinesTracking::Header header;
    uint32_t user_id;
    uint32_t flags;
    uint32_t club_id;
    uint8_t name_length;
    uint8_t reserved1, reserved2, reserved3;
    uint32_t reserved4, reserved5;
  } packet;
  
  packet.header.magic = ToBE32(SkyLinesTracking::MAGIC);
  packet.header.crc = 0;
  packet.header.type = ToBE16(SkyLinesTracking::Type::USER_NAME_RESPONSE);
  packet.header.key = ToBE64(c.key);
  packet.user_id = ToBE32(user_id);
  packet.flags = ToBE32(SkyLinesTracking::UserNameResponsePacket::FLAG_NOT_FOUND);
  packet.club_id = 0;
  packet.name_length = 0;
  packet.reserved1 = 0;
  packet.reserved2 = 0;
  packet.reserved3 = 0;
  packet.reserved4 = 0;
  packet.reserved5 = 0;
  
  packet.header.crc = ToBE16(UpdateCRC16CCITT(
    ReferenceAsBytes(packet), 0));
  
  SendPacket(c.address, packet);
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
  const char *db_path_arg = nullptr;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      cerr << "Usage: " << argv[0] << " DBPATH [OPTIONS]" << endl;
      cerr << "Options:" << endl;
      cerr << "  --help, -h                Show this help message" << endl;
      return EXIT_SUCCESS;
    } else if (argv[i][0] != '-') {
      // Positional argument (DBPATH)
      if (db_path_arg == nullptr) {
        db_path_arg = argv[i];
      } else {
        cerr << "Error: Multiple DBPATH arguments" << endl;
        return EXIT_FAILURE;
      }
    } else {
      cerr << "Error: Unknown option: " << argv[i] << endl;
      cerr << "Use --help for usage information" << endl;
      return EXIT_FAILURE;
    }
  }

  if (db_path_arg == nullptr) {
    cerr << "Usage: " << argv[0] << " DBPATH [OPTIONS]" << endl;
    cerr << "Use --help for more information" << endl;
    return EXIT_FAILURE;
  }

  const Path db_path(db_path_arg);

  EventLoop event_loop;
  SignalMonitorInit(event_loop);
  AtScopeExit() { SignalMonitorFinish(); };

  CloudServer server(db_path, event_loop,
                     IPv4Address(CloudServer::GetDefaultPort()));

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
