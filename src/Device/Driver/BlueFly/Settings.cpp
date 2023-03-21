// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Device/Util/NMEAWriter.hpp"

#include <stdio.h>
#include <string.h>
#include <limits.h>

const char BlueFlyDevice::BlueFlySettings::VOLUME_NAME[] = "BVL";
const char BlueFlyDevice::BlueFlySettings::OUTPUT_MODE_NAME[] = "BOM";

/**
 * Parse the given BlueFly Vario setting identified by its name.
 */
void
BlueFlyDevice::BlueFlySettings::Parse(std::string_view name, unsigned long value)
{
  assert(value <= UINT_MAX);

  if (name == VOLUME_NAME)
    volume = double(value) / VOLUME_MULTIPLIER;
  else if (name == OUTPUT_MODE_NAME)
    output_mode = value;
}

void
BlueFlyDevice::WriteDeviceSetting(const char *name, int value,
                                  OperationEnvironment &env)
{
  char buffer[64];

  assert(strlen(name) == 3);

  sprintf(buffer, "%s %d", name, value);
  PortWriteNMEA(port, buffer, env);
}

void
BlueFlyDevice::RequestSettings(OperationEnvironment &env)
{
  {
    const std::lock_guard lock{mutex_settings};
    settings_ready = false;
  }

  PortWriteNMEA(port, "BST", env);
}

bool
BlueFlyDevice::WaitForSettings(unsigned int timeout)
{
  std::unique_lock lock{mutex_settings};
  if (!settings_ready)
    settings_cond.wait_for(lock, std::chrono::milliseconds(timeout));
  return settings_ready;
}

BlueFlyDevice::BlueFlySettings
BlueFlyDevice::GetSettings() noexcept
{
  const std::lock_guard lock{mutex_settings};
  return settings;
}

void
BlueFlyDevice::WriteDeviceSettings(const BlueFlySettings &new_settings,
                                   OperationEnvironment &env)
{
  // TODO: unprotected read access to settings
  if (new_settings.volume != settings.volume)
    WriteDeviceSetting(settings.VOLUME_NAME, new_settings.ExportVolume(), env);
  if (new_settings.output_mode != settings.output_mode)
    WriteDeviceSetting(settings.OUTPUT_MODE_NAME,
                       new_settings.ExportOutputMode(), env);

  /* update the old values from the new settings.
   * The BlueFly Vario does not send back any ACK. */
  const std::lock_guard lock{mutex_settings};
  settings = new_settings;
}
