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

#include "AndroidBluetoothPort.hpp"
#include "AndroidPort.hpp"
#include "Android/BluetoothHelper.hpp"
#include "java/Global.hxx"

#include <cassert>

std::unique_ptr<Port>
OpenAndroidBluetoothPort(const TCHAR *address, PortListener *listener,
                         DataHandler &handler)
{
  assert(address != nullptr);

  PortBridge *bridge = BluetoothHelper::connect(Java::GetEnv(), address);
  if (bridge == nullptr)
    return nullptr;

  return std::make_unique<AndroidPort>(listener, handler, bridge);
}

std::unique_ptr<Port>
OpenAndroidBluetoothServerPort(PortListener *listener, DataHandler &handler)
{
  PortBridge *bridge = BluetoothHelper::createServer(Java::GetEnv());
  if (bridge == nullptr)
    return nullptr;

  return std::make_unique<AndroidPort>(listener, handler, bridge);
}
