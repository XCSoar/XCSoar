// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/StoppableThread.hpp"
#include "BufferedPort.hpp"

#include <windef.h>

class OverlappedEvent;

/**
 * Generic SerialPort thread handler class
 */
class SerialPort : public BufferedPort, protected StoppableThread
{
  unsigned baud_rate;

  HANDLE hPort;

public:
  /**
   * Creates a new serial port (RS-232) object, but does not open it yet.
   *
   * @param _handler the callback object for input received on the
   * port
   */
  SerialPort(PortListener *_listener, DataHandler &_handler);

  /**
   * Closes the serial port (Destructor)
   */
  ~SerialPort() noexcept override;

  /**
   * Opens the serial port
   *
   * Throws on error.
   */
  void Open(const char *path, unsigned baud_rate);

protected:
  bool SetRxTimeout(unsigned Timeout);

  bool IsDataPending() const noexcept {
    COMSTAT com_stat;
    DWORD errors;

    return ::ClearCommError(hPort, &errors, &com_stat) &&
      com_stat.cbInQue > 0;
  }

  /**
   * Determine the number of bytes in the driver's send buffer.
   *
   * @return the number of bytes, or -1 on error
   */
  [[gnu::pure]]
  int GetDataQueued() const noexcept;

  /**
   * Determine the number of bytes in the driver's receive buffer.
   *
   * @return the number of bytes, or -1 on error
   */
  int GetDataPending() const noexcept;

  /**
   * Wait until there is data in the driver's receive buffer.
   *
   * Throws on error.
   */
  void WaitDataPending(OverlappedEvent &overlapped,
                       unsigned timeout_ms) const;

public:
  /* virtual methods from class Port */
  PortState GetState() const noexcept override;
  bool Drain() override;
  void Flush() override;
  void SetBaudrate(unsigned baud_rate) override;
  unsigned GetBaudrate() const noexcept override;
  std::size_t Write(std::span<const std::byte> src) override;

protected:
  /* virtual methods from class Thread */
  void Run() noexcept override;
};
