// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Config.hpp"
#include "Device/Port/Listener.hpp"

#include <memory>

class Args;
class Port;
class DataHandler;
struct DeviceConfig;
class EventLoop;
namespace Cares { class Channel; }

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

  std::unique_ptr<Port> Open(EventLoop &event_loop, Cares::Channel &cares,
                             DataHandler &handler);

  void SetListener(PortListener &_listener) {
    listener = &_listener;
  }

private:
  /* virtual methods from class PortListener */
  void PortStateChanged() noexcept override;
  void PortError(const char *msg) noexcept override;
};
