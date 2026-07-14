// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MergeThread.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Computer/TraceComputer.hpp"
#include "Protection.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Audio/VarioGlue.hpp"
#include "Device/MultipleDevices.hpp"

#ifdef HAVE_TRACKING
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Tracking/TrackingGlue.hpp"
#endif

MergeThread::MergeThread(DeviceBlackboard &_device_blackboard,
                         MultipleDevices *_devices,
                         TraceComputer *_trail_vario_sink) noexcept
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
   devices(_devices),
   trail_vario_sink(_trail_vario_sink)
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
                   device_blackboard.Calculated(), settings_computer);

#ifdef HAVE_TRACKING
  if (net_components != nullptr && net_components->tracking != nullptr)
    net_components->tracking->MergeOnlineTraffic(
      device_blackboard.SetBasic().flarm, basic);
#endif

  flarm_computer.Process(device_blackboard.SetBasic().flarm,
                         last_fix.flarm, basic);
}

void
MergeThread::Tick() noexcept
{
  bool gps_updated, calculated_updated;

#ifdef HAVE_PCM_PLAYER
  AudioVarioGlue::VarioAudioInput vario_audio_input;
#endif

  TracePoint::Time trail_push_time{};
  float trail_push_vario = 0;
  bool do_trail_vario_push = false;
  bool vario_output_updated = false;

  {
    const std::lock_guard lock{device_blackboard.mutex};

    Process();

    const MoreData &basic = device_blackboard.Basic();
    const DerivedInfo &calculated = device_blackboard.Calculated();

    if (trail_vario_sink != nullptr &&
        basic.time_available &&
        basic.location_available &&
        basic.NavAltitudeAvailable() &&
        calculated.flight.flying) {
      if (!computer.FilteredVarioActive()) {
        if (basic.netto_vario_available) {
          do_trail_vario_push = true;
          trail_push_time = basic.time.Cast<TracePoint::Time>();
          trail_push_vario = (float)basic.netto_vario;
        }
      } else if (basic.brutto_vario_available &&
                 computer.FilteredVarioSampleUpdated()) {
        do_trail_vario_push = true;
        trail_push_time = basic.time.Cast<TracePoint::Time>();
        trail_push_vario = (float)basic.FilteredNettoVario();
      }
    }

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
    if (!computer.FilteredVarioActive()) {
      if (basic.brutto_vario_available)
        vario_audio_input.vario = basic.brutto_vario;
    } else {
      if (basic.filtered_brutto_vario_available)
        vario_audio_input.vario = basic.FilteredBruttoVario();
    }

    if (calculated.V_stf_available && basic.airspeed_available &&
        basic.total_energy_vario_available && calculated.flight.flying)
      vario_audio_input.stf_speed_error =
        calculated.V_stf - basic.true_airspeed;

    vario_audio_input.circling = calculated.circling;
#endif

    /* Throttle map vario-bar redraws to ~1 Hz when the LX filter is active. */
    vario_output_updated = !computer.FilteredVarioActive() ||
      computer.FilteredVarioSampleUpdated();

    /* update last_any in every iteration */
    last_any = basic;

    /* update last_fix only when a new GPS fix was received */
    if ((basic.time_available &&
         (!last_fix.time_available || basic.time != last_fix.time)) ||
        basic.location_available != last_fix.location_available)
      last_fix = basic;
  }

  if (do_trail_vario_push && trail_vario_sink != nullptr)
    trail_vario_sink->PushMergeVarioSample(trail_push_time, trail_push_vario);

#ifdef HAVE_PCM_PLAYER
  AudioVarioGlue::SetValue(vario_audio_input);
#endif

  if (gps_updated)
    TriggerGPSUpdate();

  if (calculated_updated)
    TriggerCalculatedUpdate();

  TriggerVarioUpdate(vario_output_updated);
}
