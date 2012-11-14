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

#ifndef XCSOAR_DEVICE_SERIAL_PORT_HPP
#define XCSOAR_DEVICE_SERIAL_PORT_HPP

#include "Thread/StoppableThread.hpp"
#include "BufferedPort.hpp"

#include <windef.h>

#ifndef _WIN32_WCE
class OverlappedEvent;
#endif

/**
 * Generic SerialPort thread handler class
 */
class SerialPort : public BufferedPort, protected StoppableThread
{
  unsigned baud_rate;

  HANDLE hPort;

#ifdef _WIN32_WCE
  /**
   * @see IsWidcommDevice()
   */
  bool is_widcomm;
#else
  static constexpr bool is_widcomm = false;
#endif

public:
  /**
   * Creates a new serial port (RS-232) object, but does not open it yet.
   *
   * @param _handler the callback object for input received on the
   * port
   */
  SerialPort(DataHandler &_handler);

  /**
   * Closes the serial port (Destructor)
   */
  virtual ~SerialPort();

  /**
   * Opens the serial port
   * @return True on success, False on failure
   */
  bool Open(const TCHAR *path, unsigned baud_rate);

protected:
  bool SetRxTimeout(unsigned Timeout);

  bool IsDataPending() const {
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
  gcc_pure
  int GetDataQueued() const;

  /**
   * Determine the number of bytes in the driver's receive buffer.
   *
   * @return the number of bytes, or -1 on error
   */
  int GetDataPending() const;

#ifndef _WIN32_WCE
  /**
   * Wait until there is data in the driver's receive buffer.
   *
   * @return the number of bytes, or -1 on error
   */
  WaitResult WaitDataPending(OverlappedEvent &overlapped,
                             unsigned timeout_ms) const;
#endif

public:
  /* virtual methods from class Port */
  virtual PortState GetState() const gcc_override;
  virtual bool Drain() gcc_override;
  virtual void Flush() gcc_override;
  virtual bool SetBaudrate(unsigned baud_rate) gcc_override;
  virtual unsigned GetBaudrate() const gcc_override;
  virtual size_t Write(const void *data, size_t length) gcc_override;

protected:
  /* virtual methods from class Thread */
  virtual void Run() gcc_override;
};

#endif
