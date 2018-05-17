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

#include "Server.hpp"
#include "Assemble.hpp"
#include "Protocol.hpp"
#include "Import.hpp"
#include "OS/ByteOrder.hpp"
#include "Util/CRC.hpp"

namespace SkyLinesTracking {

Server::Server(boost::asio::io_service &io_service,
               boost::asio::ip::udp::endpoint endpoint)
  :socket(io_service, endpoint)
{
  AsyncReceive();
}

Server::~Server()
{
  if (socket.is_open()) {
    socket.cancel();
    socket.close();
  }
}

void
Server::SendBuffer(const boost::asio::ip::udp::endpoint &endpoint,
                   boost::asio::const_buffer data)
{
  // TODO: use async_send_to()?

  try {
    socket.send_to(boost::asio::const_buffers_1(data), endpoint, 0);
  } catch (std::runtime_error e) {
    OnSendError(endpoint, std::move(e));
  }
}

void
Server::OnPing(const Client &client, unsigned id)
{
  SendPacket(client.endpoint, MakeAck(client.key, id, 0));
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
Server::OnReceive(const boost::system::error_code &ec, size_t size)
{
  // TODO: use recvmmsg() on Linux

  if (ec) {
    if (ec == boost::asio::error::operation_aborted)
      return;

    socket.close();

    OnError(boost::system::system_error(ec));
    return;
  }

  OnDatagramReceived(std::move(client_buffer), buffer, size);

  AsyncReceive();
}

void
Server::AsyncReceive()
{
  socket.async_receive_from(boost::asio::buffer(buffer, sizeof(buffer)),
                            client_buffer.endpoint,
                            std::bind(&Server::OnReceive, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

}
