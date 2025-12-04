// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "LXNAVVario.hpp"
#include "LX1600.hpp"
#include "NanoProtocol.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "time/TimeoutClock.hpp"
// Forward declarations - avoid pulling in UI dependencies
class Device;
struct DeviceInfo;
void ManageLXNAVVarioDialog(Device &device, const DeviceInfo &info,
                            const DeviceInfo &secondary_info);
void ManageNanoDialog(Device &device, const DeviceInfo &info);
void ManageLX16xxDialog(Device &device, const DeviceInfo &info);
#include "NMEA/Info.hpp"

void
LXDevice::LinkTimeout()
{
  busy = false;

  const std::lock_guard lock{mutex};

  ResetDeviceDetection();

  {
    const std::lock_guard<Mutex> lock(lxnav_vario_settings);
    lxnav_vario_settings.clear();
  }

  {
    const std::lock_guard<Mutex> lock(nano_settings);
    nano_settings.clear();
  }

  mode = Mode::UNKNOWN;
  old_baud_rate = 0;
}

bool
LXDevice::EnableNMEA(OperationEnvironment &env)
{
  unsigned old_baud_rate;

  {
    const std::lock_guard lock{mutex};
    if (mode == Mode::NMEA)
      return true;

    old_baud_rate = this->old_baud_rate;
    this->old_baud_rate = 0;
    mode = Mode::NMEA;
    busy = false;
  }

  if (is_colibri) {
    /* avoid confusing a Colibri with new protocol commands */
    if (old_baud_rate != 0)
      port.SetBaudrate(old_baud_rate);
    return true;
  }

  /* just in case the V7 is still in pass-through mode: */
  LXNAVVario::ModeNormal(port, env);

  LXNAVVario::SetupNMEA(port, env);
  if (!IsLXNAVVario())
    LX1600::SetupNMEA(port, env);

  if (old_baud_rate != 0)
    port.SetBaudrate(old_baud_rate);

  port.Flush();

  Nano::RequestForwardedInfo(port, env);
  if (!IsLXNAVVario())
    Nano::RequestInfo(port, env);

  return true;
}

void
LXDevice::OnSysTicker()
{
  const std::lock_guard lock{mutex};
  if (mode == Mode::COMMAND && !busy) {
    /* keep the command mode alive while the user chooses a flight in
       the download dialog */
    port.Flush();
    LX::SendSYN(port);
  }
}

bool
LXDevice::EnablePassThrough(OperationEnvironment &env)
{
  if (mode == Mode::PASS_THROUGH)
    return true;

  if (is_v7 || use_pass_through)
    LXNAVVario::ModeDirect(port, env);

  mode = Mode::PASS_THROUGH;
  return true;
}

bool
LXDevice::EnableLoggerNMEA(OperationEnvironment &env)
{
  return IsV7() || UsePassThrough()
    ? EnablePassThrough(env)
    : (LXNAVVario::ModeNormal(port, env), true);
}

bool
LXDevice::EnableCommandMode(OperationEnvironment &env)
{
  {
    const std::lock_guard lock{mutex};
    if (mode == Mode::COMMAND)
      return true;
  }

  port.StopRxThread();

  if (!EnablePassThrough(env)) {
    mode = Mode::UNKNOWN;
    return false;
  }

  /* make sure the pass-through command has been sent to the device
     before we continue sending commands */
  port.Drain();

  if (bulk_baud_rate != 0) {
    old_baud_rate = port.GetBaudrate();
    if (old_baud_rate == bulk_baud_rate)
      old_baud_rate = 0;
    else if (old_baud_rate != 0) {
      /* before changing the baud rate, we need an additional delay,
         because Port::Drain() does not seem to work reliably on Linux
         with a USB-RS232 converter; with a V7+Nano, 100ms is more
         than enough */
      env.Sleep(std::chrono::milliseconds(100));

      try {
        port.SetBaudrate(bulk_baud_rate);
      } catch (...) {
        mode = Mode::UNKNOWN;
        throw;
      }
    }
  } else
    old_baud_rate = 0;

  try {
    LX::CommandMode(port, env);
  } catch (...) {
    if (old_baud_rate != 0) {
      port.SetBaudrate(old_baud_rate);
      old_baud_rate = 0;
    }

    const std::lock_guard lock{mutex};
    mode = Mode::UNKNOWN;
    throw;
  }

  busy = false;

  const std::lock_guard lock{mutex};
  mode = Mode::COMMAND;
  return true;
}

bool
LXDevice::Manage(unsigned device_index,
                 DeviceBlackboard &device_blackboard)
{
  DeviceInfo info, secondary_info;

  {
    const std::lock_guard lock{device_blackboard.mutex};
    const NMEAInfo &basic = device_blackboard.RealState(device_index);
    info = basic.device;
    secondary_info = basic.secondary_device;
  }

  if (IsLXNAVVario())
    ManageLXNAVVarioDialog(*this, info, secondary_info);
  else if (IsNano())
    ManageNanoDialog(*this, info);
  else if (IsLX16xx())
    ManageLX16xxDialog(*this, info);
  else
    return false;

  return true;
}

bool
LXDevice::ManagePassthroughDevice(Device *passthrough_device,
                                   unsigned device_index,
                                   DeviceBlackboard &device_blackboard,
                                   OperationEnvironment &env)
{
  if (passthrough_device == nullptr)
    return false;

  /* Capture the current alive timestamp before enabling passthrough */
  Validity alive_before;
  {
    const std::lock_guard lock{device_blackboard.mutex};
    const NMEAInfo &basic = device_blackboard.RealState(device_index);
    alive_before = basic.alive;
  }

  /* Enable passthrough mode */
  if (!EnablePassThrough(env))
    return false;

  /* Wait for the secondary device to become alive before calling Manage().
     This ensures that passthrough mode is fully established and the
     secondary device is responding. We wait for the alive flag to be
     updated (modified) after passthrough was enabled, indicating the
     secondary device has sent data. */
  const TimeoutClock timeout(std::chrono::seconds(10));

  while (!env.IsCancelled() && !timeout.HasExpired()) {
    {
      const std::lock_guard lock{device_blackboard.mutex};
      const NMEAInfo &basic = device_blackboard.RealState(device_index);
      if (basic.alive.IsValid() && basic.alive.Modified(alive_before))
        break;
    }

    env.Sleep(std::chrono::milliseconds(50));
  }

  /* Check if operation was cancelled or timed out */
  if (env.IsCancelled() || timeout.HasExpired()) {
    /* Restore NMEA mode before returning */
    EnableNMEA(env);
    return false;
  }

  /* Manage the passthrough device */
  const bool result = passthrough_device->Manage(device_index,
                                                 device_blackboard);

  /* Restore NMEA mode */
  EnableNMEA(env);

  return result;
}
