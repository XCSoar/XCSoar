/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Device/Port.hpp"

/**
 * A port wrapper that dumps everything into the log file.
 */
class DumpPort : public Port {
  Port &other;

public:
  DumpPort(Port &_other):Port(*(Handler *)NULL), other(_other) {}

  virtual size_t Write(const void *data, size_t length);
  virtual void Flush();
  virtual bool SetRxTimeout(unsigned timeout_ms);
  virtual unsigned GetBaudrate() const;
  virtual unsigned SetBaudrate(unsigned baud_rate);
  virtual bool StopRxThread();
  virtual bool StartRxThread();
  virtual int Read(void *buffer, size_t size);
};

#endif
