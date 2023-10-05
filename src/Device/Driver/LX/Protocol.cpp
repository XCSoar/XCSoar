// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Protocol.hpp"
#include "Device/Error.hpp"
#include "Operation/Operation.hpp"

#include <cassert>

void
LX::CommandMode(Port &port, OperationEnvironment &env)
{
  /* switch to command mode, first attempt */

  SendSYN(port);

  port.FullFlush(env, std::chrono::milliseconds(50),
                 std::chrono::milliseconds(200));

  /* the port is clean now; try the SYN/ACK procedure up to three
     times */
  for (unsigned i = 0;; ++i) {
    try {
      Connect(port, env);
    } catch (const DeviceTimeout &) {
      if (i >= 100)
        throw;
      /* retry */
      continue;
    }

    /* make sure all remaining ACKs are flushed */
    port.FullFlush(env, std::chrono::milliseconds(200),
                   std::chrono::milliseconds(500));
    return;
  }
}

void
LX::CommandModeQuick(Port &port, OperationEnvironment &env)
{
  SendSYN(port);
  env.Sleep(std::chrono::milliseconds(500));
  SendSYN(port);
  env.Sleep(std::chrono::milliseconds(500));
  SendSYN(port);
  env.Sleep(std::chrono::milliseconds(500));
}

void
LX::SendPacket(Port &port, Command command,
               std::span<const std::byte> payload,
               OperationEnvironment &env,
               std::chrono::steady_clock::duration timeout)
{
  SendCommand(port, command);

  port.FullWrite(payload, env, timeout);
  port.Write(UpdateCRC8(payload, std::byte{0xff}));
}

bool
LX::ReceivePacket(Port &port, Command command,
                  std::span<std::byte> dest, OperationEnvironment &env,
                  std::chrono::steady_clock::duration first_timeout,
                  std::chrono::steady_clock::duration subsequent_timeout,
                  std::chrono::steady_clock::duration total_timeout)
{
  port.Flush();
  SendCommand(port, command);
  return
    ReadCRC(port, dest, env,
            first_timeout, subsequent_timeout, total_timeout);
}

bool
LX::ReceivePacketRetry(Port &port, Command command,
                       std::span<std::byte> dest, OperationEnvironment &env,
                       std::chrono::steady_clock::duration first_timeout,
                       std::chrono::steady_clock::duration subsequent_timeout,
                       std::chrono::steady_clock::duration total_timeout,
                       unsigned n_retries)
{
  assert(n_retries > 0);

  while (true) {
    if (ReceivePacket(port, command, dest, env,
                      first_timeout, subsequent_timeout,
                      total_timeout))
      return true;

    if (n_retries-- == 0)
      return false;

    CommandMode(port, env);

    port.Flush();
  }
}

bool
LX::ReadCRC(Port &port, std::span<std::byte> dest, OperationEnvironment &env,
            std::chrono::steady_clock::duration first_timeout,
            std::chrono::steady_clock::duration subsequent_timeout,
            std::chrono::steady_clock::duration total_timeout)
{
  port.FullRead(dest, env,
                first_timeout, subsequent_timeout,
                total_timeout);

  std::byte crc;
  port.FullRead(std::span{&crc, 1}, env,
                subsequent_timeout, subsequent_timeout,
                subsequent_timeout);

  return UpdateCRC8(dest, std::byte{0xff}) == crc;
}
