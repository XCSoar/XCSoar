/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Checksum.hpp"
#include "Conversion.hpp"
#include "IGC.hpp"
#include "Communication.hpp"
#include "Operation.hpp"
#include "Device/Declaration.hpp"
#include "Device/Driver.hpp"
#include "MessageParser.hpp"
#include "Device/Port/Port.hpp"
#include "OS/Clock.hpp"
#include "OS/FileUtil.hpp"

#include <cstdio>

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
  if (!Send(port, MSG_CFG_HELLO) || env.IsCancelled())
    return false;

  const TMsg *msg = Receive(port, 100, 0);
  if (!msg || msg->msgID != MSG_CFG_HELLO || env.IsCancelled())
    return false;

  _serialNumber = msg->sn;

  // configure baudrate
  unsigned baudRate = port.GetBaudrate();
  if (baudRate == 0)
    return false;

  if (!Send(port, MSG_CFG_STARTCONFIG, 0, 0, IMICOMM_BIGPARAM1(baudRate),
            IMICOMM_BIGPARAM2(baudRate)) || env.IsCancelled())
    return false;

  // get device info
  for (unsigned i = 0; i < 4; i++) {
    if (!Send(port, MSG_CFG_DEVICEINFO))
      continue;

    if (env.IsCancelled())
      return false;

    const TMsg *msg = Receive(port, 300, sizeof(TDeviceInfo));
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
IMI::DeclarationWrite(Port &port, const Declaration &decl)
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
  return SendRet(port, MSG_DECLARATION, &imiDecl, sizeof(imiDecl),
                 MSG_ACK_SUCCESS, 0, -1) != NULL;
}

bool
IMI::ReadFlightList(Port &port, RecordedFlightList &flight_list)
{
  flight_list.clear();

  if (!_connected)
    return false;

  IMIWORD address = 0, addressStop = 0xFFFF;
  IMIBYTE count = 1, totalCount = 0;

  for (;; count++) {
    const TMsg *pMsg = SendRet(port, MSG_FLIGHT_INFO, NULL, 0, MSG_FLIGHT_INFO,
                               -1, totalCount, address, addressStop, 200, 6);
    if (pMsg == NULL)
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
                    const TCHAR *path, OperationEnvironment &env)
{
  if (!_connected)
    return false;

  MessageParser::Reset();

  Flight flight;
  if (!FlashRead(port, &flight, flight_info.internal.imi, sizeof(flight)))
    return false;

  FILE *fileIGC = _tfopen(path, _T("w+b"));
  if (fileIGC == NULL)
    return false;

  unsigned fixesCount = COMM_MAX_PAYLOAD_SIZE / sizeof(Fix);
  Fix *fixBuffer = (Fix*)malloc(sizeof(Fix) * fixesCount);
  if (fixBuffer == NULL)
    return false;

  bool ok = true;

  WriteHeader(flight.decl, flight.signature.tampered, fileIGC);

  int noenl = 0;
  if ((flight.decl.header.sensor & IMINO_ENL_MASK) != 0)
    noenl = 1;

  unsigned address = flight_info.internal.imi + sizeof(flight);

  unsigned fixesRemains = flight.finish.fixes;
  while (ok && fixesRemains) {
    unsigned fixesToRead = fixesRemains;
    if (fixesToRead > fixesCount)
      fixesToRead = fixesCount;

    if (!FlashRead(port, fixBuffer, address, fixesToRead * sizeof(Fix)))
      ok = false;

    for (unsigned i = 0; ok && i < fixesToRead; i++) {
      const Fix *pFix = fixBuffer + i;
      if (IMIIS_FIX(pFix->id))
        WriteFix(*pFix, false, noenl, fileIGC);
    }

    address = address + fixesToRead * sizeof(Fix);
    fixesRemains -= fixesToRead;

    if (env.IsCancelled())
      // canceled by user
      ok = false;
  }

  free(fixBuffer);

  if (ok)
    WriteSignature(flight.signature, flight.decl.header.sn, fileIGC);

  fclose(fileIGC);

  if (!ok)
    File::Delete(path);

  return ok;
}

bool
IMI::Disconnect(Port &port)
{
  if (!_connected)
    return true;

  if (!Send(port, MSG_CFG_BYE))
    return false;

  _connected = false;
  return true;
}
