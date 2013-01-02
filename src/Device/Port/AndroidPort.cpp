/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "AndroidPort.hpp"
#include "Android/PortBridge.hpp"

AndroidPort::AndroidPort(DataHandler &_handler, PortBridge *_bridge)
  :BufferedPort(_handler), bridge(_bridge)
{
  bridge->setListener(Java::GetEnv(), this);
}

AndroidPort::~AndroidPort()
{
  BeginClose();

  delete bridge;

  EndClose();
}

bool
AndroidPort::IsValid() const
{
  return bridge != NULL && bridge->isValid(Java::GetEnv());
}

bool
AndroidPort::Drain()
{
  return bridge != NULL && bridge->drain(Java::GetEnv());
}

unsigned
AndroidPort::GetBaudrate() const
{
  return bridge != NULL
    ? bridge->getBaudRate(Java::GetEnv())
    : 0;
}

bool
AndroidPort::SetBaudrate(unsigned baud_rate)
{
  return bridge != NULL &&
    bridge->setBaudRate(Java::GetEnv(), baud_rate);
}

size_t
AndroidPort::Write(const void *data, size_t length)
{
  if (bridge == NULL)
    return 0;

  JNIEnv *env = Java::GetEnv();
  int nbytes = bridge->write(env, data, length);
  return nbytes > 0
    ? (size_t)nbytes
    : 0;
}
