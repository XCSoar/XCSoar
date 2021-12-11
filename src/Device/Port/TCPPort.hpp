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

#ifndef XCSOAR_DEVICE_TCP_PORT_HPP
#define XCSOAR_DEVICE_TCP_PORT_HPP

#include "BufferedPort.hpp"
#include "event/SocketEvent.hxx"

/**
 * A TCP listener port class.
 */
class TCPPort final : public BufferedPort
{
  SocketEvent listener, connection;

public:
  /**
   * Creates a new TCPPort object, but does not open it yet.
   *
   * @param handler the callback object for input received on the
   * port
   */
  TCPPort(EventLoop &event_loop,
          unsigned port,
          PortListener *_listener, DataHandler &_handler);

  /**
   * Closes the serial port (Destructor)
   */
  ~TCPPort() noexcept override;

  auto &GetEventLoop() const noexcept {
    return listener.GetEventLoop();
  }

  /* virtual methods from class Port */
  PortState GetState() const noexcept override;

  bool Drain() override {
    /* writes are synchronous */
    return true;
  }

  void SetBaudrate(unsigned) override {
  }

  unsigned GetBaudrate() const noexcept override {
    return 0;
  }

  std::size_t Write(const void *data, std::size_t length) override;

protected:
  void AsyncAccept() noexcept {
    listener.ScheduleRead();
  }

  void OnListenerReady(unsigned events) noexcept;

  void AsyncRead() noexcept {
    connection.ScheduleRead();
  }

  void OnConnectionReady(unsigned events) noexcept;
};

#endif
