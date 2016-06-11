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

#ifndef XCSOAR_DEBUG_PORT_HPP
#define XCSOAR_DEBUG_PORT_HPP

#include "Device/Config.hpp"
#include "Device/Port/Listener.hpp"

#include <memory>

class Args;
class Port;
class DataHandler;
struct DeviceConfig;
namespace boost { namespace asio { class io_service; }}

DeviceConfig
ParsePortArgs(Args &args);

class DebugPort final : PortListener {
  DeviceConfig config;

  PortListener *listener = nullptr;

public:
  explicit DebugPort(Args &args)
    :config(ParsePortArgs(args)) {}

  const DeviceConfig &GetConfig() const {
    return config;
  }

  std::unique_ptr<Port> Open(boost::asio::io_service &io_service,
                             DataHandler &handler);

  void SetListener(PortListener &_listener) {
    listener = &_listener;
  }

private:
  /* virtual methods from class PortListener */
  void PortStateChanged() override;
  void PortError(const char *msg) override;
};

#endif
