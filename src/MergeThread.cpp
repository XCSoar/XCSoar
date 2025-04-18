// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MergeThread.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "NMEA/MoreData.hpp"
#include "Audio/VarioGlue.hpp"
#include "Device/MultipleDevices.hpp"

MergeThread::MergeThread(DeviceBlackboard &_device_blackboard,
                         MultipleDevices *_devices) noexcept
  :WorkerThread("MergeThread",
#ifdef KOBO
                /* throttle more on the Kobo, because the EPaper
                   screen cannot be updated that often */
                std::chrono::milliseconds{450},
                std::chrono::milliseconds{100},
#else
                std::chrono::milliseconds{50},
                std::chrono::milliseconds{20},
#endif
                std::chrono::milliseconds{10}),
   device_blackboard(_device_blackboard),
   devices(_devices)
{
  last_fix.Reset();
  last_any.Reset();
}

void
MergeThread::Process() noexcept
{
  assert(!IsDefined() || IsInside());

  device_blackboard.Merge();

  const MoreData &basic = device_blackboard.Basic();
  const ComputerSettings &settings_computer =
    device_blackboard.GetComputerSettings();

  computer.Fill(device_blackboard.SetMoreData(), settings_computer);
  computer.Compute(device_blackboard.SetMoreData(), last_any, last_fix,
                   device_blackboard.Calculated());

  flarm_computer.Process(device_blackboard.SetBasic().flarm,
                         last_fix.flarm, basic);
}

void
MergeThread::Tick() noexcept
{
  bool gps_updated, calculated_updated;

#ifdef HAVE_PCM_PLAYER
  bool vario_available;
  double vario;
#endif

  {
    const std::lock_guard lock{device_blackboard.mutex};

    Process();

    const MoreData &basic = device_blackboard.Basic();

    /* call Driver::OnSensorUpdate() on all devices */
    if (devices != nullptr)
      devices->NotifySensorUpdate(basic);

    /* trigger update if gps has become available or dropped out */
    gps_updated = last_any.location_available != basic.location_available;

    /* trigger a redraw when the connection was just lost, to show the
       new state; when no GPS is connected, no other entity triggers
       the redraw, so we have to do it */
    calculated_updated = (bool)last_any.alive != (bool)basic.alive ||
      (bool)last_any.location_available != (bool)basic.location_available;

#ifdef HAVE_PCM_PLAYER
    vario_available = basic.brutto_vario_available;
    vario = vario_available ? basic.brutto_vario : 0;
#endif

    /* update last_any in every iteration */
    last_any = basic;

    /* update last_fix only when a new GPS fix was received */
    if ((basic.time_available &&
         (!last_fix.time_available || basic.time != last_fix.time)) ||
        basic.location_available != last_fix.location_available)
      last_fix = basic;
  }

#ifdef HAVE_PCM_PLAYER
  if (vario_available)
    AudioVarioGlue::SetValue(vario);
  else
    AudioVarioGlue::NoValue();
#endif

  if (gps_updated)
    TriggerGPSUpdate();

  if (calculated_updated)
    TriggerCalculatedUpdate();

  TriggerVarioUpdate();
}
