// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Server.hpp"
#include "Assemble.hpp"
#include "Protocol.hpp"
#include "Import.hpp"
#include "util/ByteOrder.hxx"
#include "net/SocketError.hxx"
#include "net/UniqueSocketDescriptor.hxx"
#include "util/CRC16CCITT.hpp"

static UniqueSocketDescriptor
CreateBindUDP(SocketAddress address)
{
  UniqueSocketDescriptor s;
  if (!s.Create(address.GetFamily(), SOCK_DGRAM, 0))
    throw MakeSocketError("Failed to create socket");

  if (!s.Bind(address))
    throw MakeSocketError("Failed to connect socket");

  return s;
}

namespace SkyLinesTracking {

Server::Server(EventLoop &event_loop,
               SocketAddress server_address)
  :socket(event_loop, BIND_THIS_METHOD(OnSocketReady),
          CreateBindUDP(server_address).Release())
{
  socket.ScheduleRead();
}

Server::~Server()
{
  socket.Close();
}

void
Server::SendBuffer(SocketAddress address,
                   std::span<const std::byte> buffer) noexcept
{
  try {
    ssize_t nbytes = socket.GetSocket().WriteNoWait(buffer);
    if (nbytes < 0)
      throw MakeSocketError("Failed to send");
  } catch (...) {
    OnSendError(address, std::current_exception());
  }
}

void
Server::OnPing(const Client &client, unsigned id)
{
  SendPacket(client.address, MakeAck(client.key, id, 0));
}

inline void
Server::OnDatagramReceived(Client &&client,
                           void *data, size_t length)
{
  Header &header = *(Header *)data;
  if (length < sizeof(header))
    return;

  const uint16_t received_crc = FromBE16(header.crc);
  header.crc = 0;

  const uint16_t calculated_crc = UpdateCRC16CCITT(data, length, 0);
  if (received_crc != calculated_crc)
    return;

  client.key = FromBE64(header.key);

  const auto &ping = *(const PingPacket *)data;
  const auto &fix = *(const FixPacket *)data;
  const auto &traffic = *(const TrafficRequestPacket *)data;
  const auto &user_name = *(const UserNameRequestPacket *)data;
  const auto &wave = ((const WaveSubmitPacket *)data)->wave;
  const auto &thermal = ((const ThermalSubmitPacket *)data)->thermal;

  switch ((Type)FromBE16(header.type)) {
  case PING:
    if (length < sizeof(ping))
      return;

    OnPing(client, FromBE16(ping.id));
    break;

  case FIX:
    if (length < sizeof(fix))
      return;

    OnFix(client,
          ImportTimeMs(fix.time),
          fix.flags & ToBE32(FixPacket::FLAG_LOCATION)
          ? ImportGeoPoint(fix.location)
          : ::GeoPoint::Invalid(),
          fix.flags & ToBE32(FixPacket::FLAG_ALTITUDE)
          ? (int16_t)FromBE16(fix.altitude)
          : -1);
    break;

  case TRAFFIC_REQUEST:
    if (length < sizeof(traffic))
      return;

    OnTrafficRequest(client,
                     traffic.flags & ToBE32(TrafficRequestPacket::FLAG_NEAR));
    break;

  case USER_NAME_REQUEST:
    if (length < sizeof(user_name))
      return;

    OnUserNameRequest(client,
                      FromBE32(user_name.user_id));
    break;

  case WAVE_SUBMIT:
    if (length < sizeof(wave))
      return;

    OnWaveSubmit(client,
                 ImportTimeMs(wave.time),
                 ImportGeoPoint(wave.a), ImportGeoPoint(wave.b),
                 (int16_t)FromBE16(wave.bottom_altitude),
                 (int16_t)FromBE16(wave.top_altitude),
                 FromBE16(wave.lift) / 256.);
    break;

  case WAVE_REQUEST:
    OnWaveRequest(client);
    break;

  case THERMAL_SUBMIT:
    if (length < sizeof(thermal))
      return;

    OnThermalSubmit(client,
                    ImportTimeMs(thermal.time),
                    ImportGeoPoint(thermal.bottom_location),
                    (int16_t)FromBE16(thermal.bottom_altitude),
                    ImportGeoPoint(thermal.top_location),
                    (int16_t)FromBE16(thermal.top_altitude),
                    FromBE16(thermal.lift) / 256.);
    break;

  case THERMAL_REQUEST:
    OnThermalRequest(client);
    break;

  case ACK:
  case TRAFFIC_RESPONSE:
  case USER_NAME_RESPONSE:
  case WAVE_RESPONSE:
  case THERMAL_RESPONSE:
    /* these are server-to-client packets; ignore them */
    break;
  }
}

void
Server::OnSocketReady(unsigned) noexcept
try {
  // TODO: use recvmmsg() on Linux

  Client client;
  socklen_t address_size = sizeof(client.address);
  char buffer[4096];

  ssize_t nbytes = recvfrom(socket.GetSocket().Get(), buffer, sizeof(buffer),
                            MSG_DONTWAIT,
                            client.address, &address_size);
  if (nbytes < 0)
    throw MakeSocketError("Failed to receive");

  client.address.SetSize(address_size);
  // TODO: set client.key

  OnDatagramReceived(std::move(client), buffer, nbytes);
} catch (...) {
  socket.Close();
  OnError(std::current_exception());
}

}
