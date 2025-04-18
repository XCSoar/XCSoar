// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Util/LineHandler.hpp"

class MultipleDevices;

/**
 * A #DataHandler that dispatches incoming data to all NMEA outputs.
 */
class DeviceDispatcher final : public PortLineHandler {
  MultipleDevices &devices;

  /**
   * The device index that should be excluded.  It is this
   * dispatcher's own index.
   */
  unsigned exclude;

public:
  DeviceDispatcher(MultipleDevices &_devices, unsigned _exclude) noexcept
    :devices(_devices), exclude(_exclude) {}

  /* virtual methods from DataHandler */
  bool LineReceived(const char *line) noexcept override;
};
