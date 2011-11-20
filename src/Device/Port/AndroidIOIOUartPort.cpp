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

#include "AndroidIOIOUartPort.hpp"
#include "Android/IOIOHelper.hpp"
#include "Android/IOIOManager.hpp"
#include "Android/Main.hpp"
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
  helper->closeUart(Java::GetEnv(), UartID);
  ioio_manager->RemoveClient();
}

bool
AndroidIOIOUartPort::Open()
{
  assert(UartID >=0 && UartID < (int)getNumberUarts());

  helper = ioio_manager->AddClient();
  if (helper->openUart(Java::GetEnv(), UartID, BaudRate) == -1) {
    return false;
  }

  return true;
}

void
AndroidIOIOUartPort::Flush(void)
{
  helper->flush(Java::GetEnv(), UartID);
}

void
AndroidIOIOUartPort::Run()
{
  SetRxTimeout(500);

  while (!CheckStopped()) {
    int ch = helper->read(Java::GetEnv(), UartID);
    if (ch >= 0) {
      char ch2 = ch;
      handler.DataReceived(&ch2, sizeof(ch2));
    }
  }
}

bool
AndroidIOIOUartPort::Close()
{
  StopRxThread();
  return true;
}

size_t
AndroidIOIOUartPort::Write(const void *data, size_t length)
{
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

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

bool
AndroidIOIOUartPort::SetRxTimeout(unsigned Timeout)
{
  helper->setReadTimeout(Java::GetEnv(), UartID, Timeout);
  return true;
}

unsigned
AndroidIOIOUartPort::GetBaudrate() const
{
  return helper->getBaudRate(Java::GetEnv(), UartID);
}

unsigned
AndroidIOIOUartPort::SetBaudrate(unsigned BaudRate)
{
  return helper->setBaudRate(Java::GetEnv(), UartID, BaudRate);
}

int
AndroidIOIOUartPort::Read(void *Buffer, size_t Size)
{
  JNIEnv *env = Java::GetEnv();
  int ch = helper->read(env, UartID);
  if (ch < 0)
    return -1;

  *(uint8_t *)Buffer = ch;
  return 1;
}
