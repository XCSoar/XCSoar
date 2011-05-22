/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "FifoBuffer.hpp"
#include "Thread/StoppableThread.hpp"
#include "Device/Port.hpp"

/**
 * A serial port class for POSIX (/dev/ttyS*, /dev/ttyUSB*).
 */
class TTYPort : public Port, protected StoppableThread
{
  static const unsigned NMEA_BUF_SIZE = 100;

  /** Name of the serial port */
  TCHAR sPortName[64];

  unsigned rx_timeout;

  unsigned baud_rate;

  int fd;

  FifoBuffer<char> buffer;

public:
  /**
   * Creates a new TTYPort object, but does not open it yet.
   *
   * @param path the path of the virtual file to open, e.g. "/dev/ttyS0"
   * @param _baud_rate the speed of the port
   * @param _handler the callback object for input received on the
   * port
   */
  TTYPort(const TCHAR *path, unsigned _baud_rate, Handler &_handler);

  /**
   * Closes the serial port (Destructor)
   */
  virtual ~TTYPort();

  virtual size_t Write(const void *data, size_t length);
  virtual void Flush();

  /**
   * Opens the serial port
   * @return True on success, False on failure
   */
  bool Open();
  /**
   * Closes the serial port
   * @return True on success, False on failure
   */
  bool Close();

  virtual bool SetRxTimeout(unsigned Timeout);
  virtual unsigned long GetBaudrate() const;
  virtual unsigned long SetBaudrate(unsigned long BaudRate);
  virtual bool StopRxThread();
  virtual bool StartRxThread();
  void ProcessChar(char c);

  virtual int Read(void *Buffer, size_t Size);

protected:
  /**
   * Entry point for the receive thread
   */
  virtual void Run();
};

#endif
