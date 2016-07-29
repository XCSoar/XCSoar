/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Assemble.hpp"
#include "Protocol.hpp"
#include "Import.hpp"
#include "OS/ByteOrder.hpp"
#include "NMEA/Info.hpp"
#include "Net/StaticSocketAddress.hxx"
#include "Util/CRC.hpp"
#include "Util/ConstBuffer.hxx"

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
#include "IO/Async/IOThread.hpp"
#include "Util/UTF8.hpp"
#include "Util/ConvertString.hpp"

#include <string>

void
SkyLinesTracking::Client::SetIOThread(IOThread *_io_thread)
{
  if (socket.IsDefined() && io_thread != nullptr && handler != nullptr)
    io_thread->LockRemove(socket.ToFileDescriptor());

  io_thread = _io_thread;

  if (socket.IsDefined() && io_thread != nullptr && handler != nullptr)
    io_thread->LockAdd(socket.ToFileDescriptor(), IOThread::READ, *this);
}

void
SkyLinesTracking::Client::SetHandler(Handler *_handler)
{
  if (socket.IsDefined() && io_thread != nullptr && handler != nullptr)
    io_thread->LockRemove(socket.ToFileDescriptor());

  handler = _handler;

  if (socket.IsDefined() && io_thread != nullptr && handler != nullptr)
    io_thread->LockAdd(socket.ToFileDescriptor(), IOThread::READ, *this);
}

#endif

bool
SkyLinesTracking::Client::Open(SocketAddress _address)
{
  assert(_address.IsDefined());

  Close();

  address = _address;
  if (!socket.Create(address.GetFamily(), SOCK_DGRAM, 0))
    return false;

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  if (io_thread != nullptr && handler != nullptr)
    io_thread->LockAdd(socket.ToFileDescriptor(), IOThread::READ, *this);
#endif

  return true;
}

void
SkyLinesTracking::Client::Close()
{
  if (!socket.IsDefined())
    return;

#ifdef HAVE_SKYLINES_TRACKING_HANDLER
  if (io_thread != nullptr && handler != nullptr)
    io_thread->LockRemove(socket.ToFileDescriptor());
#endif

  socket.Close();
}

bool
SkyLinesTracking::Client::SendFix(const NMEAInfo &basic)
{
  assert(socket.IsDefined());
  assert(key != 0);

  return SendPacket(ToFix(key, basic));
}

bool
SkyLinesTracking::Client::SendPing(uint16_t id)
{
  assert(socket.IsDefined());
  assert(key != 0);

  return SendPacket(MakePing(key, id));
}

#ifdef HAVE_SKYLINES_TRACKING_HANDLER

bool
SkyLinesTracking::Client::SendTrafficRequest(bool followees, bool club,
                                             bool near)
{
  assert(socket.IsDefined());
  assert(key != 0);

  return SendPacket(MakeTrafficRequest(key, followees, club, near));
}

bool
SkyLinesTracking::Client::SendUserNameRequest(uint32_t user_id)
{
  assert(socket.IsDefined());
  assert(key != 0);

  return SendPacket(MakeUserNameRequest(key, user_id));
}

inline void
SkyLinesTracking::Client::OnTrafficReceived(const TrafficResponsePacket &packet,
                                            size_t length)
{
  if (length < sizeof(packet))
    return;

  const unsigned n = packet.traffic_count;
  const TrafficResponsePacket::Traffic *traffic =
    (const TrafficResponsePacket::Traffic *)(&packet + 1);

  if (length != sizeof(packet) + n * sizeof(*traffic))
    return;

  const TrafficResponsePacket::Traffic *end = traffic + n;
  for (; traffic != end; ++traffic)
    handler->OnTraffic(FromBE32(traffic->pilot_id),
                       FromBE32(traffic->time),
                       ImportGeoPoint(traffic->location),
                       (int16_t)FromBE16(traffic->altitude));
}

inline void
SkyLinesTracking::Client::OnUserNameReceived(const UserNameResponsePacket &packet,
                                             size_t length)
{
  if (length != sizeof(packet) + packet.name_length)
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
                                 fixed(FromBE16(thermal.bottom_altitude))),
                       AGeoPoint(ImportGeoPoint(thermal.top_location),
                                 fixed(FromBE16(thermal.top_altitude))),
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

bool
SkyLinesTracking::Client::OnSocketEvent(SocketDescriptor s, unsigned mask)
{
  if (!socket.IsDefined())
    return false;

  uint8_t buffer[4096];
  ssize_t nbytes;
  StaticSocketAddress source_address;

  while ((nbytes = socket.Read(buffer, sizeof(buffer), source_address)) > 0)
    if (source_address == address)
      OnDatagramReceived(buffer, nbytes);

  return true;
}

#endif
