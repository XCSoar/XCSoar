// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  std::size_t Read(std::span<std::byte> dest) override;
  void WaitRead(std::chrono::steady_clock::duration timeout) override;
  bool StopRxThread() override;
  bool StartRxThread() override;

protected:
  /* virtual methods from class DataHandler */
  bool DataReceived(std::span<const std::byte> s) noexcept override;
};
