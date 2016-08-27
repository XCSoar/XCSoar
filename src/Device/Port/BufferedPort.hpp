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

#ifndef XCSOAR_DEVICE_BUFFERED_PORT_HPP
#define XCSOAR_DEVICE_BUFFERED_PORT_HPP

#include "Port.hpp"
#include "IO/DataHandler.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hxx"
#include "Util/StaticFifoBuffer.hxx"

#include <stdint.h>

/**
 * An abstract #Port implementation which manages incoming data in a
 * FIFO buffer.  This buffer can be fed from another thread.  Derive
 * from this class and call DataReceived() (or use the DataHandler
 * base class) whenever you get some data from the device.
 */
class BufferedPort : public Port, protected DataHandler {
  /**
   * Protects the buffer and the flags.
   */
  Mutex mutex;

  /**
   * Emitted by DataReceived() after data has been placed into the
   * buffer.
   */
  Cond cond;

  StaticFifoBuffer<uint8_t, 16384> buffer;

  bool running;

  bool closing;

public:
  BufferedPort(PortListener *_listener, DataHandler &_handler);

protected:
  void BeginClose();
  void EndClose();

public:
  /* virtual methods from class Port */
  virtual void Flush() override;
  virtual int Read(void *Buffer, size_t Size) override;
  virtual WaitResult WaitRead(unsigned timeout_ms) override;
  virtual bool StopRxThread() override;
  virtual bool StartRxThread() override;

protected:
  /* virtual methods from class DataHandler */
  virtual void DataReceived(const void *data, size_t length) override;
};

#endif
