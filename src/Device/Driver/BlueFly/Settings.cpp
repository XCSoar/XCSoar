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

#include "Internal.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Util/StringView.hxx"

#include <stdio.h>
#include <limits.h>

const char BlueFlyDevice::BlueFlySettings::VOLUME_NAME[] = "BVL";
const char BlueFlyDevice::BlueFlySettings::OUTPUT_MODE_NAME[] = "BOM";

/**
 * Parse the given BlueFly Vario setting identified by its name.
 */
void
BlueFlyDevice::BlueFlySettings::Parse(StringView name, unsigned long value)
{
  assert(value <= UINT_MAX);

  if (name.Equals(VOLUME_NAME))
    volume = double(value) / VOLUME_MULTIPLIER;
  else if (name.Equals(OUTPUT_MODE_NAME))
    output_mode = value;
}

bool
BlueFlyDevice::WriteDeviceSetting(const char *name, int value,
                                  OperationEnvironment &env)
{
  char buffer[64];

  assert(strlen(name) == 3);

  sprintf(buffer, "%s %d", name, value);
  return PortWriteNMEA(port, buffer, env);
}

bool
BlueFlyDevice::RequestSettings(OperationEnvironment &env)
{
  {
    const std::lock_guard<Mutex> lock(mutex_settings);
    settings_ready = false;
  }

  return PortWriteNMEA(port, "BST", env);
}

bool
BlueFlyDevice::WaitForSettings(unsigned int timeout)
{
  std::unique_lock<Mutex> lock(mutex_settings);
  if (!settings_ready)
    settings_cond.wait_for(lock, std::chrono::milliseconds(timeout));
  return settings_ready;
}

BlueFlyDevice::BlueFlySettings
BlueFlyDevice::GetSettings() noexcept
{
  const std::lock_guard<Mutex> lock(mutex_settings);
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
  const std::lock_guard<Mutex> lock(mutex_settings);
  settings = new_settings;
}
