// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"
#include "Assemble.hpp"
#include "FLARM/Traffic.hpp"
#include "Geo/GeoPoint.hpp"
#include "Handler.hpp"
#include "Import.hpp"
#include "LogFile.hpp"
#include "Math/Angle.hpp"
#include "Protocol.hpp"
#include "event/Call.hxx"
#include "net/StaticSocketAddress.hxx"
#include "net/ToString.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "util/ByteOrder.hxx"
#include "util/CRC16CCITT.hpp"
#include "util/ConvertString.hpp"
#include "util/UTF8.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <span>
#include <string>

void
SkyLinesTracking::Client::Open(Cares::Channel &cares, const char *server)
{
  BlockingCall(GetEventLoop(), [this, &cares, server]() {
    InternalClose();

    Cares::SimpleHandler &resolver_handler = *this;
    resolver.emplace(resolver_handler, GetDefaultPort());
    resolver->Start(cares, server);
  });
}

bool
SkyLinesTracking::Client::Open(SocketAddress _address)
{
  assert(_address.IsDefined());

  Close();

  address = _address;

  UniqueSocketDescriptor socket;
  if (!socket.Create(address.GetFamily(), SOCK_DGRAM, 0)) return false;

  // TODO: bind?

  if (handler != nullptr) {
    BlockingCall(GetEventLoop(), [&socket, this]() {
      socket_event.Open(socket.Release());
      socket_event.ScheduleRead();
    });

    handler->OnSkyLinesReady();
  }

  return true;
}

void
SkyLinesTracking::Client::InternalClose() noexcept
{
  const std::lock_guard lock{mutex};
  socket_event.Close();
  resolver.reset();
}

void
SkyLinesTracking::Client::Close()
{
  BlockingCall(GetEventLoop(), [this]() { InternalClose(); });
}

void
SkyLinesTracking::Client::SendFix(const NMEAInfo &basic)
{
  assert(key != 0);

  SendPacket(ToFix(key, basic));
}

void
SkyLinesTracking::Client::SendPing(uint16_t id)
{
  assert(key != 0);

  SendPacket(MakePing(key, id));
}

void
SkyLinesTracking::Client::SendThermal(uint32_t time,
                                      ::GeoPoint bottom_location,
                                      int bottom_altitude,
                                      ::GeoPoint top_location,
                                      int top_altitude, double lift)
{
  assert(key != 0);

  SendPacket(MakeThermalSubmit(key, time, bottom_location, bottom_altitude,
                               top_location, top_altitude, lift));
}

void
SkyLinesTracking::Client::SendThermalRequest()
{
  assert(key != 0);

  SendPacket(MakeThermalRequest(key));
}

void
SkyLinesTracking::Client::SendTrafficRequest(bool followees, bool club,
                                             bool near_)
{
  assert(key != 0);

  SendPacket(MakeTrafficRequest(key, followees, club, near_));
}

void
SkyLinesTracking::Client::SendUserNameRequest(uint32_t user_id)
{
  assert(key != 0);

  SendPacket(MakeUserNameRequest(key, user_id));
}

inline void
SkyLinesTracking::Client::OnTrafficReceived(
    const TrafficResponsePacket &packet, size_t length)
{
  if (length < sizeof(packet)) {
    LogFormat(
        "SkyLines: TRAFFIC_RESPONSE packet too short (%zu < %zu), dropping",
        length, sizeof(packet));
    return;
  }

  const unsigned n = packet.traffic_count;
  LogFormat("SkyLines: TRAFFIC_RESPONSE contains %u traffic entries", n);

  const std::span<const TrafficResponsePacket::Traffic> list(
      (const TrafficResponsePacket::Traffic *)(&packet + 1), n);

  const size_t expected_length = sizeof(packet) + n * sizeof(list.front());
  if (length != expected_length) {
    LogFormat(
        "SkyLines: TRAFFIC_RESPONSE length mismatch (%zu != %zu), dropping",
        length, expected_length);
    return;
  }

  LogFormat("SkyLines: Processing %u traffic entries", n);
  for (const auto &traffic : list) {
    // Decode turn_rate from reserved (int16_t scaled by 10)
    uint16_t reserved_u16 = traffic.reserved;
    int16_t turn_rate_scaled = FromBE16(reserved_u16);
    double turn_rate = turn_rate_scaled / 10.0;

    // Decode reserved2:
    // - Lower 8 bits: aircraft_type
    // - Next 9 bits (8-16): track (0-359 degrees)
    // - Upper 16 bits (17-32): flarm_id (upper 16 bits of 24-bit id)
    uint32_t encoded = FromBE32(traffic.reserved2);
    uint8_t aircraft_type_byte = encoded & 0xFF;
    // Extract track from bits 8-16 (9 bits, 0-359)
    unsigned track = (encoded >> 8) & 0x1FF;
    // Extract the full 24-bit id from the encoded value.
    // The encoding stores the upper 16 bits of the 24-bit flarm_id in bits
    // 17-32. Extract bits 17-32 (upper 16 bits) and shift left by 8 to get
    // upper 16 bits of 24-bit ID. The lower 8 bits of flarm_id are not
    // transmitted, so we reconstruct it as best we can.
    uint32_t flarm_id_upper = (encoded >> 17) & 0xFFFF;
    uint32_t flarm_id = flarm_id_upper << 8;

    FlarmTraffic::AircraftType aircraft_type =
        static_cast<FlarmTraffic::AircraftType>(aircraft_type_byte);

    handler->OnTraffic(FromBE32(traffic.pilot_id), FromBE32(traffic.time),
                       ImportGeoPoint(traffic.location),
                       (int16_t)FromBE16(traffic.altitude), flarm_id, track,
                       turn_rate, aircraft_type);
  }
}

inline void
SkyLinesTracking::Client::OnUserNameReceived(
    const UserNameResponsePacket &packet, size_t length)
{
  if (length < sizeof(packet) || length != sizeof(packet) + packet.name_length)
    return;

  /* the name follows the UserNameResponsePacket object */
  const char *_name = (const char *)(&packet + 1);
  const std::string name(_name, packet.name_length);
  if (!ValidateUTF8(name.c_str())) return;

  UTF8ToWideConverter tname(name.c_str());
  handler->OnUserName(FromBE32(packet.user_id), tname);
}

inline void
SkyLinesTracking::Client::OnWaveReceived(const WaveResponsePacket &packet,
                                         size_t length)
{
  if (length < sizeof(packet)) return;

  const unsigned n = packet.wave_count;
  std::span<const Wave> waves((const Wave *)(&packet + 1), n);
  if (length != sizeof(packet) + waves.size() * sizeof(waves.front())) return;

  for (const auto &wave : waves)
    handler->OnWave(FromBE32(wave.time), ImportGeoPoint(wave.a),
                    ImportGeoPoint(wave.b));
}

inline void
SkyLinesTracking::Client::OnThermalReceived(
    const ThermalResponsePacket &packet, size_t length)
{
  if (length < sizeof(packet)) return;

  const unsigned n = packet.thermal_count;
  std::span<const Thermal> thermals((const Thermal *)(&packet + 1), n);
  if (length != sizeof(packet) + thermals.size() * sizeof(thermals.front()))
    return;

  for (const auto &thermal : thermals)
    handler->OnThermal(FromBE32(thermal.time),
                       AGeoPoint(ImportGeoPoint(thermal.bottom_location),
                                 FromBE16(thermal.bottom_altitude)),
                       AGeoPoint(ImportGeoPoint(thermal.top_location),
                                 FromBE16(thermal.top_altitude)),
                       FromBE16(thermal.lift) / 256.);
}

inline void
SkyLinesTracking::Client::OnDatagramReceived(void *data, size_t length)
{
  Header &header = *(Header *)data;
  if (length < sizeof(header)) {
    LogFormat("SkyLines: Packet too short (%zu < %zu), dropping", length,
              sizeof(header));
    return;
  }

  const uint16_t received_crc = FromBE16(header.crc);
  header.crc = 0;

  const uint16_t calculated_crc = UpdateCRC16CCITT(data, length, 0);
  if (received_crc != calculated_crc) {
    LogFormat("SkyLines: CRC mismatch (received=0x%04x, calculated=0x%04x), "
              "dropping packet",
              received_crc, calculated_crc);
    return;
  }

  const ACKPacket &ack = *(const ACKPacket *)data;
  const TrafficResponsePacket &traffic = *(const TrafficResponsePacket *)data;
  const UserNameResponsePacket &user_name =
      *(const UserNameResponsePacket *)data;
  const auto &wave = *(const WaveResponsePacket *)data;
  const auto &thermal = *(const ThermalResponsePacket *)data;

  const Type packet_type = (Type)FromBE16(header.type);
  LogFormat("SkyLines: Processing packet type=%u", (unsigned)packet_type);

  switch (packet_type) {
  case PING:
  case FIX:
  case TRAFFIC_REQUEST:
  case USER_NAME_REQUEST:
  case WAVE_SUBMIT:
  case WAVE_REQUEST:
  case THERMAL_SUBMIT:
  case THERMAL_REQUEST:
    break;

  case ACK:
    if (length >= sizeof(ack)) handler->OnAck(FromBE16(ack.id));
    break;

  case TRAFFIC_RESPONSE:
    LogFormat("SkyLines: Processing TRAFFIC_RESPONSE packet");
    OnTrafficReceived(traffic, length);
    break;

  case USER_NAME_RESPONSE:
    OnUserNameReceived(user_name, length);
    break;

  case WAVE_RESPONSE:
    OnWaveReceived(wave, length);
    break;

  case THERMAL_RESPONSE:
    OnThermalReceived(thermal, length);
    break;
  }
}

void
SkyLinesTracking::Client::OnSocketReady(unsigned) noexcept
{
  std::byte buffer[4096];
  ssize_t nbytes;
  StaticSocketAddress source_address;

  while ((nbytes = GetSocket().ReadNoWait(std::span{buffer}, source_address)) >
         0) {
    // Determine if this is from cloud (127.0.0.1) or SkyLines by checking
    // address
    std::string address_str = ToString(SocketAddress(address));
    const bool is_cloud = (address_str.find("127.0.0.1") != std::string::npos);
    const char *source_name = is_cloud ? "cloud" : "SkyLines";

    // Debug: log all received packets
    LogFormat("%s: Received %zd bytes from %s", source_name, nbytes,
              ToString(source_address).c_str());

    if (source_address == address) {
      LogFormat("%s: Address matches, processing packet", source_name);
      OnDatagramReceived(buffer, nbytes);
    } else {
      LogFormat("%s: Address mismatch - expected %s, got %s, dropping packet",
                source_name, address_str.c_str(),
                ToString(source_address).c_str());
    }
  }

  // TODO check for errors?
}

void
SkyLinesTracking::Client::OnResolverSuccess(
    std::forward_list<AllocatedSocketAddress> addresses) noexcept
{
  {
    const std::lock_guard lock{mutex};
    resolver.reset();
  }

  if (addresses.empty()) {
    if (handler != nullptr)
      handler->OnSkyLinesError(
          std::make_exception_ptr(std::runtime_error("No address")));
    return;
  }

  Open(addresses.front());
}

void
SkyLinesTracking::Client::OnResolverError(std::exception_ptr error) noexcept
{
  {
    const std::lock_guard lock{mutex};
    resolver.reset();
  }

  if (handler != nullptr) handler->OnSkyLinesError(std::move(error));
}
