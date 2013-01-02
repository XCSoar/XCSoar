/*
Copyright_License {

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

#ifndef XCSOAR_DEVICE_DUMP_PORT_HPP
#define XCSOAR_DEVICE_DUMP_PORT_HPP

#include "Port.hpp"

/**
 * A port wrapper that dumps everything into the log file.
 */
class DumpPort : public Port {
  Port *port;

public:
  DumpPort(Port *port);

  virtual ~DumpPort();

  /* virtual methods from Port */
  virtual PortState GetState() const gcc_override;
  virtual size_t Write(const void *data, size_t length) gcc_override;
  virtual bool Drain() gcc_override;
  virtual void Flush() gcc_override;
  virtual unsigned GetBaudrate() const gcc_override;
  virtual bool SetBaudrate(unsigned baud_rate) gcc_override;
  virtual bool StopRxThread() gcc_override;
  virtual bool StartRxThread() gcc_override;
  virtual int Read(void *buffer, size_t size) gcc_override;
  virtual WaitResult WaitRead(unsigned timeout_ms) gcc_override;
};

#endif
