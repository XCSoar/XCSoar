// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "K6BtPort.hpp"

#include <stdio.h>
#include <string.h>

K6BtPort::K6BtPort(std::unique_ptr<Port> _port, unsigned _baud_rate,
                   PortListener *_listener, DataHandler &_handler) noexcept
  :Port(_listener, _handler), port(std::move(_port)), baud_rate(_baud_rate)
{
}

bool
K6BtPort::SendCommand(std::byte cmd)
{
  const std::byte data[2] = { ESCAPE, cmd };
  return port->Write(std::span{data}) == sizeof(data);
}

PortState
K6BtPort::GetState() const noexcept
{
  return port->GetState();
}

bool
K6BtPort::WaitConnected(OperationEnvironment &env)
{
  if (!port->WaitConnected(env))
    return false;

  // TODO: wrap the PortHandler, move initialisation to PortStateChanged()

  /* ensure that the K6Bt is not in command mode */
  SendCommand(NOP);

  /* now that the Bluetooth connection has been established, tell our
     preferred baud rate to the K6Bt */
  SendSetBaudrate(baud_rate);

  return true;
}

std::size_t
K6BtPort::Write(std::span<const std::byte> src)
{
  /* in order to forward the buffer verbatim to the real device, we
     have to escape all ESCAPE bytes (i.e. send each of them twice) */

  const std::byte *data = (const std::byte *)src.data();
  std::size_t length = src.size();

  std::size_t total = 0;

  const std::byte *p;
  while ((p = (const std::byte *)memchr(data, (char)ESCAPE, length)) != nullptr) {
    std::size_t chunk = p - data + 1;
    std::size_t nbytes = port->Write({data, chunk});
    total += nbytes;
    if (nbytes != chunk)
      return total;

    /* write the ESCAPE byte again (but don't consider it in the
       return value) */
    port->Write({p, 1});

    ++p;

    length = data + length - p;
    data = p;
  }

  if (length > 0)
    total += port->Write({data, length});

  return total;
}

bool
K6BtPort::Drain()
{
  return port->Drain();
}

void
K6BtPort::Flush()
{
  port->Flush();

  SendCommand(FLUSH_BUFFERS | std::byte{0x3}); /* flush RX and TX buffer */
}

static constexpr int
BaudRateToK6Bt(unsigned baud_rate) noexcept
{
  switch (baud_rate) {
  case 2400:
    return 0x0;

  case 4800:
    return 0x1;

  case 9600:
    return 0x2;

  case 19200:
    return 0x3;

  case 38400:
    return 0x4;

  case 57600:
    return 0x5;

  case 115200:
    return 0x6;

  default:
    return -1;
  }
}

void
K6BtPort::SendSetBaudrate(unsigned _baud_rate)
{
  int code = BaudRateToK6Bt(_baud_rate);
  if (code < 0)
    throw std::runtime_error("Baud rate not supported by K6Bt");

  if (!SendCommand(CHANGE_BAUD_RATE | static_cast<std::byte>(code)))
    throw std::runtime_error("Failed to send CHANGE_BAUD_RATE to K6Bt");
}

void
K6BtPort::SetBaudrate(unsigned _baud_rate)
{
  if (_baud_rate == baud_rate)
    return;

  SendSetBaudrate(_baud_rate);
  baud_rate = _baud_rate;
}

unsigned
K6BtPort::GetBaudrate() const noexcept
{
  return baud_rate;
}

bool
K6BtPort::StopRxThread()
{
  return port->StopRxThread();
}

bool
K6BtPort::StartRxThread(void)
{
  return port->StartRxThread();
}

std::size_t
K6BtPort::Read(std::span<std::byte> dest)
{
  return port->Read(dest);
}

void
K6BtPort::WaitRead(std::chrono::steady_clock::duration timeout)
{
  port->WaitRead(timeout);
}
