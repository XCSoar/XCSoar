/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_ANDROID_PORT_HPP
#define XCSOAR_DEVICE_ANDROID_PORT_HPP

#include "Port.hpp"
#include "Thread/StoppableThread.hpp"

class PortBridge;

/**
 * A #Port implementation which transmits data over a Bluetooth RFCOMM
 * socket.
 */
class AndroidPort : public Port, private StoppableThread
{
  PortBridge *bridge;

protected:
  AndroidPort(Handler &_handler);
  virtual ~AndroidPort();

  void SetBridge(PortBridge *bridge);

public:
  bool Close();

  virtual unsigned GetBaudrate() const;
  virtual bool SetBaudrate(unsigned baud_rate);

  /* virtual methods from class Port */
  virtual bool SetRxTimeout(unsigned Timeout);
  virtual void Flush();
  virtual size_t Write(const void *data, size_t length);
  virtual int Read(void *Buffer, size_t Size);
  virtual WaitResult WaitRead(unsigned timeout_ms);
  virtual bool StopRxThread();
  virtual bool StartRxThread();

protected:
  /**
   * Entry point for the receive thread
   */
  virtual void Run();
};

#endif
