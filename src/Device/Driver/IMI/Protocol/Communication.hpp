// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"

#include <chrono>
#include <span>

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
 * @param parameter1 1st parameter for to put in the message
 * @param parameter2 2nd parameter for to put in the message
 * @param parameter3 3rd parameter for to put in the message
 */
void
Send(Port &port, OperationEnvironment &env,
     IMIBYTE msgID, std::span<const std::byte> payload={},
     IMIBYTE parameter1 = 0,
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
        IMIBYTE msgID, std::span<const std::byte> payload,
        IMIBYTE reMsgID,
        IMIWORD retPayloadSize, IMIBYTE parameter1 = 0,
        IMIWORD parameter2 = 0, IMIWORD parameter3 = 0,
        std::chrono::steady_clock::duration extra_timeout = std::chrono::milliseconds{300},
        int retry = 4);

bool
FlashRead(Port &port, void *buffer, unsigned address, unsigned size,
          OperationEnvironment &env);

} // namespace IMI
