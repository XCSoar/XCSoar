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

#ifndef XCSOAR_DEVICE_K6BT_PORT_HPP
#define XCSOAR_DEVICE_K6BT_PORT_HPP

#include "Port.hpp"

#include <stdint.h>

/**
 * Wraps the K6Bt protocol over an existing Port instance.
 *
 * K6Bt is a Bluetooth to RS-232 adapter from K6-Team.
 */
class K6BtPort : public Port {
  static constexpr uint8_t NOP = 0x00;
  static constexpr uint8_t ESCAPE = 0xa5;
  static constexpr uint8_t CHANGE_BAUD_RATE = 0x30;
  static constexpr uint8_t FLUSH_BUFFERS = 0x40;

  Port *port;

  unsigned baud_rate;

public:
  K6BtPort(Port *port, unsigned baud_rate,
           PortListener *listener, DataHandler &handler);

  virtual ~K6BtPort();

protected:
  bool SendCommand(uint8_t cmd);
  bool SendSetBaudrate(unsigned baud_rate);

public:
  /* virtual methods from Port */
  PortState GetState() const override;
  bool WaitConnected(OperationEnvironment &env) override;
  size_t Write(const void *data, size_t length) override;
  bool Drain() override;
  void Flush() override;
  bool SetBaudrate(unsigned baud_rate) override;
  unsigned GetBaudrate() const override;
  bool StopRxThread() override;
  bool StartRxThread() override;
  int Read(void *Buffer, size_t Size) override;
  WaitResult WaitRead(unsigned timeout_ms) override;
};

#endif
