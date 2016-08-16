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

#include "Client.hpp"
#include "Handler.hpp"
#include "Assemble.hpp"
#include "Protocol.hpp"
#include "Import.hpp"
#include "OS/ByteOrder.hpp"
#include "Net/StaticSocketAddress.hxx"
#include "Math/Angle.hpp"
#include "Geo/GeoPoint.hpp"
#include "Util/CRC.hpp"
#include "Util/ConstBuffer.hxx"
#include "IO/Async/AsioUtil.hpp"
#include "Util/UTF8.hpp"
#include "Util/ConvertString.hpp"

#include <string>

void
SkyLinesTracking::Client::Open(boost::asio::ip::udp::resolver::query query)
{
  Close();

  const ScopeLock protect(mutex);
  resolving = true;
  resolver.async_resolve(query,
                         std::bind(&Client::OnResolved, this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
}

bool
SkyLinesTracking::Client::Open(boost::asio::ip::udp::endpoint _endpoint)
{
  Close();

  endpoint = _endpoint;

  boost::system::error_code ec;
  socket.open(endpoint.protocol(), ec);
  if (ec)
    return false;

  if (handler != nullptr) {
    AsyncReceive();
    handler->OnSkyLinesReady();
  }

  return true;
}

void
SkyLinesTracking::Client::Close()
{
  const ScopeLock protect(mutex);

  if (socket.is_open()) {
    CancelWait(socket.get_io_service(), socket);
    socket.close();
  }

  if (resolving) {
    CancelWait(socket.get_io_service(), resolver);
    resolving = false;
  }
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
                                      int top_altitude,
                                      double lift)
{
  assert(key != 0);

  SendPacket(MakeThermalSubmit(key, time,
                               bottom_location, bottom_altitude,
                               top_location, top_altitude,
                               lift));
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
SkyLinesTracking::Client::OnTrafficReceived(const TrafficResponsePacket &packet,
                                            size_t length)
{
  if (length < sizeof(packet))
    return;

  const unsigned n = packet.traffic_count;
  const ConstBuffer<TrafficResponsePacket::Traffic>
    list((const TrafficResponsePacket::Traffic *)(&packet + 1), n);

  if (length != sizeof(packet) + n * sizeof(list.front()))
    return;

  for (const auto &traffic : list)
    handler->OnTraffic(FromBE32(traffic.pilot_id),
                       FromBE32(traffic.time),
                       ImportGeoPoint(traffic.location),
                       (int16_t)FromBE16(traffic.altitude));
}

inline void
SkyLinesTracking::Client::OnUserNameReceived(const UserNameResponsePacket &packet,
                                             size_t length)
{
  if (length < sizeof(packet) || length != sizeof(packet) + packet.name_length)
    return;

  /* the name follows the UserNameResponsePacket object */
  const char *_name = (const char *)(&packet + 1);
  const std::string name(_name, packet.name_length);
  if (!ValidateUTF8(name.c_str()))
    return;

  UTF8ToWideConverter tname(name.c_str());
  handler->OnUserName(FromBE32(packet.user_id), tname);
}

inline void
SkyLinesTracking::Client::OnWaveReceived(const WaveResponsePacket &packet,
                                         size_t length)
{
  if (length < sizeof(packet))
    return;

  const unsigned n = packet.wave_count;
  ConstBuffer<Wave> waves((const Wave *)(&packet + 1), n);
  if (length != sizeof(packet) + waves.size * sizeof(waves.front()))
    return;

  for (const auto &wave : waves)
    handler->OnWave(FromBE32(wave.time),
                    ImportGeoPoint(wave.a), ImportGeoPoint(wave.b));
}

inline void
SkyLinesTracking::Client::OnThermalReceived(const ThermalResponsePacket &packet,
                                            size_t length)
{
  if (length < sizeof(packet))
    return;

  const unsigned n = packet.thermal_count;
  ConstBuffer<Thermal> thermals((const Thermal *)(&packet + 1), n);
  if (length != sizeof(packet) + thermals.size * sizeof(thermals.front()))
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
  if (length < sizeof(header))
    return;

  const uint16_t received_crc = FromBE16(header.crc);
  header.crc = 0;

  const uint16_t calculated_crc = UpdateCRC16CCITT(data, length, 0);
  if (received_crc != calculated_crc)
    return;

  const ACKPacket &ack = *(const ACKPacket *)data;
  const TrafficResponsePacket &traffic = *(const TrafficResponsePacket *)data;
  const UserNameResponsePacket &user_name =
    *(const UserNameResponsePacket *)data;
  const auto &wave = *(const WaveResponsePacket *)data;
  const auto &thermal = *(const ThermalResponsePacket *)data;

  switch ((Type)FromBE16(header.type)) {
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
    if (length >= sizeof(ack))
      handler->OnAck(FromBE16(ack.id));
    break;

  case TRAFFIC_RESPONSE:
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
SkyLinesTracking::Client::OnReceive(const boost::system::error_code &ec,
                                    size_t size)
{
  if (ec) {
    if (ec == boost::asio::error::operation_aborted)
      return;

    {
      const ScopeLock protect(mutex);
      socket.close();
    }

    if (handler != nullptr)
      handler->OnSkyLinesError(boost::system::system_error(ec));
    return;
  }

  if (sender_endpoint == endpoint)
    OnDatagramReceived(buffer, size);

  AsyncReceive();
}

void
SkyLinesTracking::Client::AsyncReceive()
{
  const ScopeLock protect(mutex);
  socket.async_receive_from(boost::asio::buffer(buffer, sizeof(buffer)),
                            sender_endpoint,
                            std::bind(&Client::OnReceive, this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void
SkyLinesTracking::Client::OnResolved(const boost::system::error_code &ec,
                                     boost::asio::ip::udp::resolver::iterator i)
{
  if (ec == boost::asio::error::operation_aborted)
    return;

  {
    const ScopeLock protect(mutex);
    resolving = false;
  }

  if (ec) {
    if (handler != nullptr)
      handler->OnSkyLinesError(boost::system::system_error(ec));
    return;
  }

  Open(*i);
}
