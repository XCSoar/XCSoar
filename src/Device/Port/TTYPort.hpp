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

#ifndef XCSOAR_DEVICE_TTY_PORT_HPP
#define XCSOAR_DEVICE_TTY_PORT_HPP

#include "BufferedPort.hpp"

#include <boost/asio/serial_port.hpp>

#include <atomic>

#include <tchar.h>

/**
 * A serial port class for POSIX (/dev/ttyS*, /dev/ttyUSB*).
 */
class TTYPort : public BufferedPort
{
  boost::asio::serial_port serial_port;

  std::atomic<bool> valid;

public:
  /**
   * Creates a new TTYPort object, but does not open it yet.
   *
   * @param _handler the callback object for input received on the
   * port
   */
  TTYPort(boost::asio::io_service &io_service,
          PortListener *_listener, DataHandler &_handler);
  virtual ~TTYPort();

  /**
   * Opens the serial port
   * @return True on success, False on failure
   */
  bool Open(const TCHAR *path, unsigned baud_rate);

  /**
   * Opens this object with a new pseudo-terminal.  This is only used
   * for debugging.
   *
   * @return the path of the slave pseudo-terminal, nullptr on error
   */
  const char *OpenPseudo();

  WaitResult WaitWrite(unsigned timeout_ms);

  /* virtual methods from class Port */
  virtual PortState GetState() const override;
  virtual bool Drain() override;
  virtual void Flush() override;
  virtual bool SetBaudrate(unsigned baud_rate) override;
  virtual unsigned GetBaudrate() const override;
  virtual size_t Write(const void *data, size_t length) override;

private:
  void OnReadReady(const boost::system::error_code &ec);

  void AsyncRead() {
    serial_port.async_read_some(boost::asio::null_buffers(),
                                std::bind(&TTYPort::OnReadReady, this,
                                          std::placeholders::_1));
  }
};

#endif
