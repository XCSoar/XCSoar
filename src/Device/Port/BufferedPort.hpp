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

#ifndef XCSOAR_DEVICE_BUFFERED_PORT_HPP
#define XCSOAR_DEVICE_BUFFERED_PORT_HPP

#include "Port.hpp"
#include "Thread/Mutex.hpp"
#include "Util/FifoBuffer.hpp"

#ifdef HAVE_POSIX
#include "Thread/Cond.hpp"
#else
#include "Thread/Trigger.hpp"
#endif

#include <stdint.h>

/**
 * An abstract #Port implementation which manages incoming data in a
 * FIFO buffer.  This buffer can be fed from another thread.  Derive
 * from this class and call DataReceived() (or use the Port::Handler
 * base class) whenever you get some data from the device.
 */
class BufferedPort : public Port, protected Port::Handler {
  /**
   * Protects the buffer and the flags.
   */
  Mutex mutex;

#ifdef HAVE_POSIX
  Cond cond;
#else
  /**
   * Emitted by DataReceived() after data has been placed into the
   * buffer.
   */
  Trigger data_trigger;

  /**
   * Emitted by Read() when data from the buffer has been consumed.
   */
  Trigger consumed_trigger;

  Trigger exited_trigger;
#endif

  FifoBuffer<uint8_t, 1024> buffer;

  bool running;

  bool waiting, closing;

public:
  BufferedPort(Port::Handler &_handler);

#ifndef NDEBUG
  virtual ~BufferedPort();
#endif

protected:
  void BeginClose();
  void EndClose();

public:
  /* virtual methods from class Port */
  virtual void Flush();
  virtual int Read(void *Buffer, size_t Size);
  virtual WaitResult WaitRead(unsigned timeout_ms);
  virtual bool StopRxThread();
  virtual bool StartRxThread();

protected:
  /* virtual methods from class Port::Handler */
  virtual void DataReceived(const void *data, size_t length);
};

#endif
