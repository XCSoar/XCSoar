// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BufferedPort.hpp"
#include "event/PipeEvent.hxx"

#include <atomic>
#include <tchar.h>

/**
 * A serial port class for POSIX (/dev/ttyS*, /dev/ttyUSB*).
 */
class TTYPort : public BufferedPort
{
  PipeEvent socket;

  std::atomic<bool> valid;

public:
  /**
   * Creates a new TTYPort object, but does not open it yet.
   *
   * @param _handler the callback object for input received on the
   * port
   */
  TTYPort(EventLoop &event_loop,
          PortListener *_listener, DataHandler &_handler);
  ~TTYPort() noexcept override;

  auto &GetEventLoop() const noexcept {
    return socket.GetEventLoop();
  }

  /**
   * Opens the serial port
   *
   * Throws on error.
   */
  void Open(const char *path, unsigned baud_rate);

  /**
   * Opens this object with a new pseudo-terminal.  This is only used
   * for debugging.
   *
   * @return the path of the slave pseudo-terminal, nullptr on error
   */
  const char *OpenPseudo();

  /* virtual methods from class Port */
  virtual PortState GetState() const noexcept override;
  virtual bool Drain() override;
  virtual void Flush() override;
  virtual void SetBaudrate(unsigned baud_rate) override;
  virtual unsigned GetBaudrate() const noexcept override;
  virtual std::size_t Write(std::span<const std::byte> src) override;

private:
  void WaitWrite(unsigned timeout_ms);

  void OnSocketReady(unsigned events) noexcept;
};
