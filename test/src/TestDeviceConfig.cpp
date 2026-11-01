// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Config.hpp"
#include "TestUtil.hpp"

static bool
ShouldReopenOnTimeout(DeviceConfig::PortType port_type) noexcept
{
  DeviceConfig config{};
  config.Clear();
  config.port_type = port_type;
  return config.ShouldReopenOnTimeout();
}

int main()
{
  plan_tests(7);

  /* an Android USB serial port can go silent after a glitch on the
     USB link without being reported as failed (#3346) */
  ok1(ShouldReopenOnTimeout(DeviceConfig::PortType::ANDROID_USB_SERIAL));

  ok1(!ShouldReopenOnTimeout(DeviceConfig::PortType::DISABLED));
  ok1(!ShouldReopenOnTimeout(DeviceConfig::PortType::SERIAL));
  ok1(!ShouldReopenOnTimeout(DeviceConfig::PortType::RFCOMM));
  ok1(!ShouldReopenOnTimeout(DeviceConfig::PortType::IOIOUART));
  ok1(!ShouldReopenOnTimeout(DeviceConfig::PortType::TCP_CLIENT));
  ok1(!ShouldReopenOnTimeout(DeviceConfig::PortType::INTERNAL));

  return exit_status();
}
