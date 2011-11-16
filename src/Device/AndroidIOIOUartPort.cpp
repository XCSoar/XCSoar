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

#include "Device/AndroidIOIOUartPort.hpp"
#include "Android/IOIOHelper.hpp"
#include "OS/Sleep.h"

#include <assert.h>

IOIOHelper* helper = NULL;

AndroidIOIOUartPort::AndroidIOIOUartPort(unsigned UartID_, unsigned _BaudRate, Handler &_handler)
  :Port(_handler),
   BaudRate(_BaudRate),
   UartID(UartID_)
{
}

AndroidIOIOUartPort::~AndroidIOIOUartPort()
{
  Close();

  /**
   *  delete helper when ANY Uart port close
   *  1) this triggers devRestart which will restart all Uarts
   *  2) assumes that if bad connection caused Uart to fail, it
   *  needs to restart all
   *  3) this is independent of XCSoar's current model that always calls
   *  devRestart() on all devices when any device setting changes or
   *  when the GPS connection does not update for a while
   */

  if (helper) {
    delete helper; // closes all attached Uarts
    helper = NULL;
  }
}

bool
AndroidIOIOUartPort::Open()
{
  if (UartID < 0 || UartID >= (int)getNumberUarts())
    return false;

  if (helper == NULL) {
    helper = new IOIOHelper(Java::GetEnv());
  }

  if (helper == NULL) {
    return false;
  }

  if (helper->openUart(Java::GetEnv(), UartID, BaudRate) == -1) {
    return false;
  }

  return true;
}

void
AndroidIOIOUartPort::Flush(void)
{
  if (helper == NULL)
    return;

  helper->flush(Java::GetEnv(), UartID);
}

void
AndroidIOIOUartPort::Run()
{
  assert(helper != NULL);

  SetRxTimeout(500);

  while (!CheckStopped()) {
    if (helper == NULL) {
      break;
    }
    int ch = helper->read(Java::GetEnv(), UartID);
    if (ch >= 0)
      ProcessChar(ch);
  }
}

bool
AndroidIOIOUartPort::Close()
{
  if (helper == NULL) {
    return true;
  }
  StopRxThread();
  return true;
}

size_t
AndroidIOIOUartPort::Write(const void *data, size_t length)
{
  if (helper == NULL)
    return 0;

  JNIEnv *env = Java::GetEnv();

  size_t nbytes = 0;
  const uint8_t *bytes = (const uint8_t *)data;
  for (size_t i = 0; i < length; ++i) {
    if (!helper->write(env, UartID, bytes[i]))
      break;
    ++nbytes;
  }

  return nbytes;
}

bool
AndroidIOIOUartPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined()) {
    return true;
  }

  BeginStop();
  Thread::Join();

  return true;
}

bool
AndroidIOIOUartPort::StartRxThread(void)
{
  // Make sure the thread isn't starting itself
  assert(!Thread::IsInside());

  // Make sure the port was opened correctly
  if (helper == NULL)
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

bool
AndroidIOIOUartPort::SetRxTimeout(unsigned Timeout)
{
  if (helper == NULL)
    return false;
  helper->setReadTimeout(Java::GetEnv(), UartID, Timeout);
  return true;
}

unsigned
AndroidIOIOUartPort::GetBaudrate() const
{
  if (helper == NULL)
    return 0u;

  return helper->getBaudRate(Java::GetEnv(), UartID);
}

unsigned
AndroidIOIOUartPort::SetBaudrate(unsigned BaudRate)
{
  if (helper == NULL)
    return 0u;
  return helper->setBaudRate(Java::GetEnv(), UartID, BaudRate);
}

int
AndroidIOIOUartPort::Read(void *Buffer, size_t Size)
{
  if (helper == NULL)
    return -1;

  JNIEnv *env = Java::GetEnv();
  int ch = helper->read(env, UartID);
  if (ch < 0)
    return -1;

  *(uint8_t *)Buffer = ch;
  return 1;
}

void
AndroidIOIOUartPort::ProcessChar(char c)
{
  Buffer::Range range = buffer.write();
  if (range.second == 0) {
    // overflow, so reset buffer
    buffer.clear();
    return;
  }

  if (c == '\n') {
    range.first[0] = _T('\0');
    buffer.append(1);

    range = buffer.read();
    handler.LineReceived(range.first);
    buffer.clear();
  } else if (c != '\r') {
    range.first[0] = c;
    buffer.append(1);
  }
}
