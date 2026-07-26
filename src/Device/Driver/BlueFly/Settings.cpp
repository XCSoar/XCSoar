// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Device/Util/NMEAWriter.hpp"

#include <stdio.h>
#include <string.h>
#include <limits.h>

const char BlueFlyDevice::BlueFlySettings::VOLUME_NAME[] =
  "BVL";
const char BlueFlyDevice::BlueFlySettings::AUDIO_WHEN_CONNECTED_NAME[] =
  "BAC";
const char BlueFlyDevice::BlueFlySettings::LIFT_THRESHOLD_NAME[] =
  "BFL";
const char BlueFlyDevice::BlueFlySettings::LIFT_OFF_THRESHOLD_NAME[] =
  "BOL";
const char BlueFlyDevice::BlueFlySettings::SINK_THRESHOLD_NAME[] =
  "BFS";
const char BlueFlyDevice::BlueFlySettings::SINK_OFF_THRESHOLD_NAME[] =
  "BOS";
const char BlueFlyDevice::BlueFlySettings::OUTPUT_MODE_NAME[] =
  "BOM";
const char BlueFlyDevice::BlueFlySettings::OUTPUT_FREQUENCY_NAME[] =
  "BOF";

/**
 * Parse the given BlueFly Vario setting identified by its name.
 */
void
BlueFlyDevice::BlueFlySettings::Parse(std::string_view name,
                                      unsigned long value)
{
  assert(value <= UINT_MAX);

  if (name == VOLUME_NAME)
    volume = double(value) / VOLUME_MULTIPLIER;
  else if (name == AUDIO_WHEN_CONNECTED_NAME)
    audio_when_connected = value != 0;
  else if (name == LIFT_THRESHOLD_NAME)
    lift_threshold = double(value) / THRESHOLD_MULTIPLIER;
  else if (name == LIFT_OFF_THRESHOLD_NAME)
    lift_off_threshold = double(value) / THRESHOLD_MULTIPLIER;
  else if (name == SINK_THRESHOLD_NAME)
    sink_threshold = double(value) / THRESHOLD_MULTIPLIER;
  else if (name == SINK_OFF_THRESHOLD_NAME)
    sink_off_threshold = double(value) / THRESHOLD_MULTIPLIER;
  else if (name == OUTPUT_MODE_NAME)
    output_mode = value;
  else if (name == OUTPUT_FREQUENCY_NAME)
    output_frequency = value;
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
    WriteDeviceSetting(settings.VOLUME_NAME,
                       new_settings.ExportVolume(), env);
  if (new_settings.audio_when_connected != settings.audio_when_connected)
    WriteDeviceSetting(settings.AUDIO_WHEN_CONNECTED_NAME,
                       new_settings.ExportAudioWhenConnected(), env);
  if (new_settings.lift_threshold != settings.lift_threshold)
    WriteDeviceSetting(settings.LIFT_THRESHOLD_NAME,
                       new_settings.ExportLiftThreshold(), env);
  if (new_settings.lift_off_threshold != settings.lift_off_threshold)
    WriteDeviceSetting(settings.LIFT_OFF_THRESHOLD_NAME,
                       new_settings.ExportLiftOffThreshold(), env);
  if (new_settings.sink_threshold != settings.sink_threshold)
    WriteDeviceSetting(settings.SINK_THRESHOLD_NAME,
                       new_settings.ExportSinkThreshold(), env);
  if (new_settings.sink_off_threshold != settings.sink_off_threshold)
    WriteDeviceSetting(settings.SINK_OFF_THRESHOLD_NAME,
                       new_settings.ExportSinkOffThreshold(), env);
  if (new_settings.output_mode != settings.output_mode)
    WriteDeviceSetting(settings.OUTPUT_MODE_NAME,
                       new_settings.ExportOutputMode(), env);
  /* Compare exported values so a missing BST (frequency 0) does not
     spuriously write BOF 1 after the dialog clamps the display value. */
  if (new_settings.ExportOutputFrequency() !=
      BlueFlySettings::ExportOutputFrequency(settings.output_frequency))
    WriteDeviceSetting(settings.OUTPUT_FREQUENCY_NAME,
                       new_settings.ExportOutputFrequency(), env);

  /* update the old values from the new settings.
   * The BlueFly Vario does not send back any ACK. */
  const std::lock_guard lock{mutex_settings};
  settings = new_settings;
}
