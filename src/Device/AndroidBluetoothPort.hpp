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

#ifndef XCSOAR_DEVICE_ANDROID_BLUETOOTH_PORT_HPP
#define XCSOAR_DEVICE_ANDROID_BLUETOOTH_PORT_HPP

#include "Util/StaticString.hpp"
#include "Util/FifoBuffer.hpp"
#include "Thread/StoppableThread.hpp"
#include "Thread/Trigger.hpp"
#include "Device/Port.hpp"
#include "Java/Object.hpp"

#include <windows.h>

class BluetoothHelper;

/**
 * A #Port implementation which transmits data over a Bluetooth RFCOMM
 * socket.
 */
class AndroidBluetoothPort : public Port, protected StoppableThread
{
  /** the peer's Bluetooth address */
  StaticString<32> address;

  BluetoothHelper *helper;

  FifoBuffer<char> buffer;

public:
  AndroidBluetoothPort(const TCHAR *address, Handler &_handler);
  virtual ~AndroidBluetoothPort();

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
