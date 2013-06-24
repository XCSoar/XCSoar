/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

  FaultInjectionPort(DataHandler &_handler)
    :Port(_handler), running(true), baud_rate(DEFAULT_BAUD_RATE) {}

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

    --inject_port_fault;
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

Port::Port(DataHandler &_handler)
  :handler(_handler) {}

Port::~Port() {}

bool
Port::WaitConnected(OperationEnvironment &env)
{
  return GetState() == PortState::READY;
}

size_t
Port::Write(const char *s)
{
  return Write(s, strlen(s));
}

bool
Port::FullWrite(const void *buffer, size_t length,
                OperationEnvironment &env, unsigned timeout_ms)
{
  return Write(buffer, length) == length;
}

bool
Port::FullWriteString(const char *s,
                      OperationEnvironment &env, unsigned timeout_ms)
{
  return FullWrite(s, strlen(s), env, timeout_ms);
}

int
Port::GetChar()
{
  unsigned char ch;
  return Read(&ch, sizeof(ch)) == sizeof(ch)
    ? ch
    : EOF;
}

bool
Port::FullFlush(OperationEnvironment &env, unsigned timeout_ms,
                unsigned total_timeout_ms)
{
  Flush();
  return true;
}

bool
Port::FullRead(void *buffer, size_t length, OperationEnvironment &env,
               unsigned timeout_ms)
{
  return Read(buffer, length) == (int)length;
}

Port::WaitResult
Port::WaitRead(OperationEnvironment &env, unsigned timeout_ms)
{
  return WaitRead(timeout_ms);
}

bool
Port::ExpectString(const char *token,
                   OperationEnvironment &env, unsigned timeout_ms)
{
  if (inject_port_fault == 0)
    return false;

  --inject_port_fault;
  return true;
}

Port::WaitResult
Port::WaitForChar(const char token, OperationEnvironment &env,
                  unsigned timeout_ms)
{
  return inject_port_fault > 0 ? WaitResult::READY : WaitResult::FAILED;
}
