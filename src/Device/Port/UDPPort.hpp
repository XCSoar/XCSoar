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

#ifndef XCSOAR_DEVICE_UDP_PORT_HPP
#define XCSOAR_DEVICE_UDP_PORT_HPP

#include "BufferedPort.hpp"

#include <boost/asio/ip/udp.hpp>

/**
 * A UDP listener port class.
 */
class UDPPort final : public BufferedPort
{
  boost::asio::ip::udp::socket socket;

  char input[4096];

public:
  /**
   * Creates a new UDPPort object, but does not open it yet.
   *
   * @param handler the callback object for input received on the
   * port
   */
  UDPPort(boost::asio::io_service &io_service,
          unsigned port,
          PortListener *_listener, DataHandler &_handler);

  /**
   * Closes the serial port (Destructor)
   */
  virtual ~UDPPort();

  /* virtual methods from class Port */
  PortState GetState() const override;

  bool Drain() override {
    /* writes are synchronous */
    return true;
  }

  bool SetBaudrate(unsigned baud_rate) override {
    return true;
  }

  unsigned GetBaudrate() const override {
    return 0;
  }

  size_t Write(const void *data, size_t length) override;

protected:
  void AsyncRead() {
    socket.async_receive(boost::asio::buffer(input, sizeof(input)),
                         std::bind(&UDPPort::OnRead, this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
  }

  void OnRead(const boost::system::error_code &ec, size_t nbytes);
};

#endif
