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

#ifndef XCSOAR_DEVICE_TTY_PORT_HPP
#define XCSOAR_DEVICE_TTY_PORT_HPP

#include "BufferedPort.hpp"
#include "event/SocketEvent.hxx"

#include <atomic>

#include <tchar.h>

/**
 * A serial port class for POSIX (/dev/ttyS*, /dev/ttyUSB*).
 */
class TTYPort : public BufferedPort
{
  SocketEvent socket;

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
  void Open(const TCHAR *path, unsigned baud_rate);

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
  virtual std::size_t Write(const void *data, std::size_t length) override;

private:
  void WaitWrite(unsigned timeout_ms);

  void OnSocketReady(unsigned events) noexcept;
};

#endif
