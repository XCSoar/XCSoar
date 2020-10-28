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

#include "Protocol.hpp"
#include "Conversion.hpp"
#include "IGC.hpp"
#include "Communication.hpp"
#include "Operation/Operation.hpp"
#include "Device/Declaration.hpp"
#include "Device/RecordedFlight.hpp"
#include "MessageParser.hpp"
#include "Device/Port/Port.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/FileOutputStream.hxx"
#include "time/BrokenDateTime.hpp"

#include <memory>

#include <stdlib.h>

namespace IMI
{
  bool _connected = false;
  TDeviceInfo _info;
  IMIWORD _serialNumber;
}

bool
IMI::Connect(Port &port, OperationEnvironment &env)
{
  if (_connected)
    return true;

  memset(&_info, 0, sizeof(_info));
  _serialNumber = 0;
  MessageParser::Reset();

  // check connectivity
  if (!Send(port, env, MSG_CFG_HELLO) || env.IsCancelled())
    return false;

  const TMsg *msg = Receive(port, env, 100, 0);
  if (!msg || msg->msgID != MSG_CFG_HELLO || env.IsCancelled())
    return false;

  _serialNumber = msg->sn;

  // configure baudrate
  unsigned baudRate = port.GetBaudrate();
  if (baudRate == 0)
    return false;

  if (!Send(port, env,
            MSG_CFG_STARTCONFIG, 0, 0, IMICOMM_BIGPARAM1(baudRate),
            IMICOMM_BIGPARAM2(baudRate)) || env.IsCancelled())
    return false;

  // get device info
  for (unsigned i = 0; i < 4; i++) {
    if (!Send(port, env, MSG_CFG_DEVICEINFO))
      continue;

    if (env.IsCancelled())
      return false;

    const TMsg *msg = Receive(port, env, 300, sizeof(TDeviceInfo));
    if (!msg || env.IsCancelled())
      return false;

    if (msg->msgID != MSG_CFG_DEVICEINFO)
      continue;

    if (msg->payloadSize == sizeof(TDeviceInfo)) {
      memcpy(&_info, msg->payload, sizeof(TDeviceInfo));
    } else if (msg->payloadSize == 16) {
      // old version of the structure
      memset(&_info, 0, sizeof(TDeviceInfo));
      memcpy(&_info, msg->payload, 16);
    } else {
      return false;
    }

    _connected = true;
    return true;
  }

  return false;
}

bool
IMI::DeclarationWrite(Port &port, const Declaration &decl,
                      OperationEnvironment &env)
{
  if (!_connected)
    return false;

  TDeclaration imiDecl;
  memset(&imiDecl, 0, sizeof(imiDecl));

  // idecl.date ignored - will be set by FR
  ConvertToChar(decl.pilot_name, imiDecl.header.plt,
                  sizeof(imiDecl.header.plt));
  ConvertToChar(decl.aircraft_type, imiDecl.header.gty,
                  sizeof(imiDecl.header.gty));
  ConvertToChar(decl.aircraft_registration, imiDecl.header.gid,
                  sizeof(imiDecl.header.gid));
  ConvertToChar(decl.competition_id, imiDecl.header.cid,
                  sizeof(imiDecl.header.cid));
  ConvertToChar(_T("XCSOARTASK"), imiDecl.header.tskName,
                  sizeof(imiDecl.header.tskName));

  ConvertWaypoint(decl.turnpoints[0].waypoint, imiDecl.wp[0]);

  unsigned size = decl.Size();
  for (unsigned i = 0; i < size; i++) {
    ConvertWaypoint(decl.turnpoints[i].waypoint, imiDecl.wp[i + 1]);
    ConvertOZ(decl.turnpoints[i], i == 0, i == size - 1,
              imiDecl.wp[i + 1]);
  }

  ConvertWaypoint(decl.turnpoints[size - 1].waypoint,
              imiDecl.wp[size + 1]);

  // send declaration for current task
  return SendRet(port, env, MSG_DECLARATION, &imiDecl, sizeof(imiDecl),
                 MSG_ACK_SUCCESS, 0, -1) != nullptr;
}

bool
IMI::ReadFlightList(Port &port, RecordedFlightList &flight_list,
                    OperationEnvironment &env)
{
  flight_list.clear();

  if (!_connected)
    return false;

  IMIWORD address = 0, addressStop = 0xFFFF;
  IMIBYTE count = 1, totalCount = 0;

  for (;; count++) {
    const TMsg *pMsg = SendRet(port, env,
                               MSG_FLIGHT_INFO, nullptr, 0, MSG_FLIGHT_INFO,
                               -1, totalCount, address, addressStop, 200, 6);
    if (pMsg == nullptr)
      break;

    totalCount = pMsg->parameter1;
    address = pMsg->parameter2;
    addressStop = pMsg->parameter3;

    for (unsigned i = 0; i < pMsg->payloadSize / sizeof(IMI::FlightInfo); i++) {
      const IMI::FlightInfo *fi = ((const IMI::FlightInfo*)pMsg->payload) + i;
      RecordedFlightInfo &ifi = flight_list.append();

      BrokenDateTime start = ConvertToDateTime(fi->start);
      ifi.date = start;
      ifi.start_time = start;
      ifi.end_time = ConvertToDateTime(fi->finish);
      ifi.internal.imi = fi->address;
    }

    if (pMsg->payloadSize == 0 || address == 0xFFFF)
      return true;
  }

  return false;
}

bool
IMI::FlightDownload(Port &port, const RecordedFlightInfo &flight_info,
                    Path path, OperationEnvironment &env)
{
  if (!_connected)
    return false;

  MessageParser::Reset();

  Flight flight;
  if (!FlashRead(port, &flight, flight_info.internal.imi, sizeof(flight), env))
    return false;

  FileOutputStream fos(path);
  BufferedOutputStream bos(fos);

  unsigned fixesCount = COMM_MAX_PAYLOAD_SIZE / sizeof(Fix);
  auto fixBuffer = std::make_unique<Fix[]>(fixesCount);

  WriteHeader(bos, flight.decl, flight.signature.tampered);

  int noenl = 0;
  if ((flight.decl.header.sensor & IMINO_ENL_MASK) != 0)
    noenl = 1;

  unsigned address = flight_info.internal.imi + sizeof(flight);

  unsigned fixesRemains = flight.finish.fixes;
  while (fixesRemains) {
    unsigned fixesToRead = fixesRemains;
    if (fixesToRead > fixesCount)
      fixesToRead = fixesCount;

    if (!FlashRead(port, fixBuffer.get(), address, fixesToRead * sizeof(Fix), env))
      return false;

    for (unsigned i = 0; i < fixesToRead; i++) {
      const auto &pFix = fixBuffer[i];
      if (IMIIS_FIX(pFix.id))
        WriteFix(bos, pFix, false, noenl);
    }

    address = address + fixesToRead * sizeof(Fix);
    fixesRemains -= fixesToRead;

    if (env.IsCancelled())
      // canceled by user
      return false;
  }

  WriteSignature(bos, flight.signature, flight.decl.header.sn);
  bos.Flush();
  fos.Commit();
  return true;
}

bool
IMI::Disconnect(Port &port, OperationEnvironment &env)
{
  if (!_connected)
    return true;

  if (!Send(port, env, MSG_CFG_BYE))
    return false;

  _connected = false;
  return true;
}
