/* Copyright_License {

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

#include "Device/Port/Port.hpp"

#include <algorithm>
#include <assert.h>
#include <stdio.h>

static unsigned inject_port_fault;

class FaultInjectionPort : public Port {
public:
  enum {
    DEFAULT_BAUD_RATE = 1234,
  };

  bool running;
  unsigned baud_rate;

  FaultInjectionPort(PortListener *_listener, DataHandler &_handler)
    :Port(_listener, _handler),
     running(true),
     baud_rate(DEFAULT_BAUD_RATE) {}

  /* virtual methods from class Port */
  virtual PortState GetState() const override {
    return inject_port_fault > 0
      ? PortState::READY
      : PortState::FAILED;
  }

  virtual size_t Write(const void *data, size_t length) override {
    return length;
  }

  virtual bool Drain() override {
    return true;
  }

  virtual void Flush() override {}

  virtual unsigned GetBaudrate() const override {
    return baud_rate;
  }

  virtual bool SetBaudrate(unsigned _baud_rate) override {
    baud_rate = _baud_rate;
    return true;
  }

  virtual bool StopRxThread() override {
    running = false;
    return true;
  }

  virtual bool StartRxThread() override {
    running = true;
    return true;
  }

  virtual int Read(void *Buffer, size_t Size) override {
    if (inject_port_fault == 0)
      return -1;

    if (--inject_port_fault == 0)
      StateChanged();

    char *p = (char *)Buffer;
    std::fill_n(p, Size, ' ');
    return Size;
  }

  virtual WaitResult WaitRead(unsigned timeout_ms) override {
    return inject_port_fault > 0
      ? WaitResult::READY
      : WaitResult::FAILED;
  }
};
