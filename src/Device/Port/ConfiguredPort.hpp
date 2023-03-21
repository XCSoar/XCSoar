// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

namespace Cares { class Channel; }
class EventLoop;
class Port;
class PortListener;
class DataHandler;
struct DeviceConfig;

/**
 * Open the port described by #DeviceConfig.  On error, throws a
 * #std::runtime_error or returns nullptr.
 */
std::unique_ptr<Port>
OpenPort(EventLoop &event_loop, Cares::Channel &cares,
         const DeviceConfig &config, PortListener *listener,
         DataHandler &handler);
