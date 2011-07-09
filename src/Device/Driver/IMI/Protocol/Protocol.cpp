/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#include "Protocol.hpp"
#include "Checksum.hpp"
#include "Conversion.hpp"
#include "Communication.hpp"
#include "Operation.hpp"
#include "Device/Declaration.hpp"
#include "Device/Driver.hpp"
#include "MessageParser.hpp"
#include "Device/Port.hpp"
#include "OS/Clock.hpp"

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
  ConvertToChar(decl.PilotName, imiDecl.header.plt,
                  sizeof(imiDecl.header.plt));
  ConvertToChar(decl.AircraftType, imiDecl.header.gty,
                  sizeof(imiDecl.header.gty));
  ConvertToChar(decl.AircraftReg, imiDecl.header.gid,
                  sizeof(imiDecl.header.gid));
  ConvertToChar(decl.CompetitionId, imiDecl.header.cid,
                  sizeof(imiDecl.header.cid));
  ConvertToChar(_T("XCSOARTASK"), imiDecl.header.tskName,
                  sizeof(imiDecl.header.tskName));

  ConvertWaypoint(decl.TurnPoints[0].waypoint, imiDecl.wp[0]);

  for (unsigned i = 0; i < decl.size(); i++) {
    ConvertWaypoint(decl.TurnPoints[i].waypoint, imiDecl.wp[i + 1]);
    ConvertOZ(decl.TurnPoints[i], i == 0, i == decl.size() - 1,
              imiDecl.wp[i + 1]);
  }

  ConvertWaypoint(decl.TurnPoints[decl.size() - 1].waypoint,
              imiDecl.wp[decl.size() + 1]);

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
IMI::Disconnect(Port &port)
{
  if (!_connected)
    return true;

  if (!Send(port, MSG_CFG_BYE))
    return false;

  _connected = false;
  return true;
}
