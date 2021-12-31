/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_IMI_COMMUNICATION_HPP
#define XCSOAR_IMI_COMMUNICATION_HPP

#include "Types.hpp"

#include <chrono>

class Port;
class OperationEnvironment;

namespace IMI {

/**
 * @brief Prepares and sends the message to a device
 *
 * Throws on error or cancellation.
 *
 * @param port Device handle
 * @param msgID ID of the message to send
 * @param payload Payload buffer to use for the message
 * @param payloadSize The size of the payload buffer
 * @param parameter1 1st parameter for to put in the message
 * @param parameter2 2nd parameter for to put in the message
 * @param parameter3 3rd parameter for to put in the message
 */
void
Send(Port &port, OperationEnvironment &env,
     IMIBYTE msgID, const void *payload = 0,
     IMIWORD payloadSize = 0, IMIBYTE parameter1 = 0,
     IMIWORD parameter2 = 0, IMIWORD parameter3 = 0);

/**
 * @brief Receives a message from the device
 *
 * @param port Device handle
 * @param extra_timeout Additional timeout to wait for the message
 * @param expectedPayloadSize Expected size of the message
 */
TMsg
Receive(Port &port, OperationEnvironment &env,
        std::chrono::steady_clock::duration extra_timeout,
        unsigned expectedPayloadSize);

/**
 * @brief Sends a message and waits for a confirmation from the device
 *
 * @param port Device handle
 * @param msgID ID of the message to send
 * @param payload Payload buffer to use for the message
 * @param payloadSize The size of the payload buffer
 * @param reMsgID Expected ID of the message to receive
 * @param retPayloadSize Expected size of the received message
 * @param parameter1 1st parameter for to put in the message
 * @param parameter2 2nd parameter for to put in the message
 * @param parameter3 3rd parameter for to put in the message
 * @param extra_timeout Additional timeout to wait for the message
 * @param retry Number of send retries
 */
TMsg
SendRet(Port &port, OperationEnvironment &env,
        IMIBYTE msgID, const void *payload,
        IMIWORD payloadSize, IMIBYTE reMsgID,
        IMIWORD retPayloadSize, IMIBYTE parameter1 = 0,
        IMIWORD parameter2 = 0, IMIWORD parameter3 = 0,
        std::chrono::steady_clock::duration extra_timeout = std::chrono::milliseconds{300},
        int retry = 4);

bool
FlashRead(Port &port, void *buffer, unsigned address, unsigned size,
          OperationEnvironment &env);

} // namespace IMI

#endif
