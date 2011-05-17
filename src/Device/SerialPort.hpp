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

#ifndef XCSOAR_DEVICE_SERIAL_PORT_HPP
#define XCSOAR_DEVICE_SERIAL_PORT_HPP

#include "FifoBuffer.hpp"
#include "Thread/StoppableThread.hpp"
#include "Device/Port.hpp"

#include <windef.h>

#ifndef _WIN32_WCE
class OverlappedEvent;
#endif

/**
 * Generic SerialPort thread handler class
 */
class SerialPort : public Port, protected StoppableThread
{
  static const unsigned NMEA_BUF_SIZE = 100;

  /** Name of the serial port */
  TCHAR sPortName[64];

  unsigned baud_rate;

  HANDLE hPort;

  FifoBuffer<char> buffer;

#ifdef _WIN32_WCE
  /**
   * @see IsWidcommDevice()
   */
  bool is_widcomm;
#else
  static const bool is_widcomm = false;

  unsigned rx_timeout;
#endif

public:
  /**
   * Creates a new serial port (RS-232) object, but does not open it yet.
   *
   * @param path the path of the virtual file to open, e.g. "COM1:"
   * @param _baud_rate the speed of the port
   * @param _handler the callback object for input received on the
   * port
   */
  SerialPort(const TCHAR *path, unsigned _baud_rate, Handler &_handler);

  /**
   * Closes the serial port (Destructor)
   */
  virtual ~SerialPort();

protected:
  bool IsDataPending() const {
    COMSTAT com_stat;
    DWORD errors;

    return ::ClearCommError(hPort, &errors, &com_stat) &&
      com_stat.cbInQue > 0;
  }

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
  int WaitDataPending(OverlappedEvent &overlapped, unsigned timeout_ms) const;
#endif

public:
  virtual void Write(const void *data, unsigned length);
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

  unsigned GetRxTimeout();
  virtual bool SetRxTimeout(int Timeout);
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
