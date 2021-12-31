/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "io/DataHandler.hpp"
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"
#include "util/StaticFifoBuffer.hxx"

#include <cstdint>

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

  StaticFifoBuffer<std::byte, 16384> buffer;

  bool running = false;

public:
  using Port::Port;

public:
  /* virtual methods from class Port */
  void Flush() override;
  std::size_t Read(void *Buffer, std::size_t Size) override;
  void WaitRead(std::chrono::steady_clock::duration timeout) override;
  bool StopRxThread() override;
  bool StartRxThread() override;

protected:
  /* virtual methods from class DataHandler */
  bool DataReceived(std::span<const std::byte> s) noexcept override;
};

#endif
