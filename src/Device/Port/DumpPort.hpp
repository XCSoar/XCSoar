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

#ifndef XCSOAR_DEVICE_DUMP_PORT_HPP
#define XCSOAR_DEVICE_DUMP_PORT_HPP

#include "Port.hpp"

/**
 * A port wrapper that dumps everything into the log file.
 */
class DumpPort final : public Port {
  Port *port;

  /**
   * Dumping is enabled until this MonotonicClockMS() time stamp.  0
   * means completely disabled, unsigned(-1) means enabled forever.
   */
  unsigned until_ms;

public:
  /**
   * Initialises the new instance.  Dumping is enabled forever by
   * default.
   */
  DumpPort(Port *port);

  virtual ~DumpPort();

  /**
   * Disable dumping immediately.
   */
  void Disable() {
    until_ms = 0;
  }

  /**
   * Enable dumping forever.
   */
  void EnableForever() {
    until_ms = unsigned(-1);
  }

  /**
   * Enable dumping for a certain duration.
   */
  void EnableTemporarily(unsigned duration_ms);

  bool IsEnabled() const {
    return until_ms > 0;
  }

private:
  /**
   * Determine whether dumping is currently enabled.
   */
  bool CheckEnabled();

public:
  /* virtual methods from Port */
  PortState GetState() const override;
  bool WaitConnected(OperationEnvironment &env) override;
  size_t Write(const void *data, size_t length) override;
  bool Drain() override;
  void Flush() override;
  unsigned GetBaudrate() const override;
  bool SetBaudrate(unsigned baud_rate) override;
  bool StopRxThread() override;
  bool StartRxThread() override;
  int Read(void *buffer, size_t size) override;
  WaitResult WaitRead(unsigned timeout_ms) override;
};

#endif
