// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "LXNAVVario.hpp"
#include "LX1600.hpp"
#include "NanoProtocol.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "util/NumberParser.hpp"

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

  mc_sent = false;
  mc_requested = false;
  last_sent_mc.reset();
  ballast_sent = false;
  ballast_requested = false;
  last_sent_ballast_overload.reset();
  bugs_sent = false;
  bugs_requested = false;
  last_sent_bugs.reset();
  vario_just_detected = false;

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

void
LXDevice::OnCalculatedUpdate(const MoreData &basic,
                             const DerivedInfo &calculated)
{
  /* Only handle settings sync for LXNAV varios */
  if (!IsLXNAVVario())
    return;

  NullOperationEnvironment env;

  /* Check if vario was just detected and request settings */
  bool should_request_settings = false;
  {
    const std::lock_guard lock{mutex};
    if (vario_just_detected) {
      should_request_settings = true;
      vario_just_detected = false;
    }
  }

  if (should_request_settings) {
    /* Request all settings on detection */
    {
      const std::lock_guard lock{mutex};
      if (!mc_requested) {
        mc_requested = true;
      }
      ballast_requested = true;
      bugs_requested = true;
    }
    RequestLXNAVVarioSetting("MC", env);
    
    /* Ballast and bugs come via LXWP2, no need to request separately */
    /* Wait for settings to be received before proceeding with sync */
    return;
  }

  /* Sync all settings */
  SyncMacCready(basic, calculated, env);
  SyncBallast(basic, calculated, env);
  SyncBugs(basic, calculated, env);
}

void
LXDevice::SyncMacCready(const MoreData &basic,
                        const DerivedInfo &calculated,
                        OperationEnvironment &env) noexcept
{
  {
    const std::lock_guard lock{mutex};
    if (mc_sent)
      return;
  }

  if (!basic.settings.mac_cready_available.IsValid())
    return;

  const double device_mc = basic.settings.mac_cready;
  const bool is_echo = IsMCEcho(basic.settings);
  
  if (!is_echo && device_mc > 0) {
    /* Device has non-zero MC that we didn't send, don't override it */
    const std::lock_guard lock{mutex};
    mc_sent = true;
    return;
  }

  /* Device MC is 0, not set, or is echoing our value - send XCSoar's MC */
  const double mc = calculated.glide_polar_safety.GetMC();
  if (PutMacCready(mc, env)) {
    const std::lock_guard lock{mutex};
    mc_sent = true;
    last_sent_mc = mc;
  }
}

void
LXDevice::SyncBallast(const MoreData &basic,
                      const DerivedInfo &calculated,
                      OperationEnvironment &env) noexcept
{
  {
    const std::lock_guard lock{mutex};
    if (ballast_sent)
      return;
  }

  if (!basic.settings.ballast_overload_available.IsValid())
    return;

  const double device_ballast_overload = basic.settings.ballast_overload;
  const bool is_echo = IsBallastEcho(basic.settings);
  
  if (!is_echo && device_ballast_overload > 1.0) {
    /* Device has ballast that we didn't send, don't override it */
    const std::lock_guard lock{mutex};
    ballast_sent = true;
    return;
  }

  /* Device has no ballast or is echoing our value - send XCSoar's ballast */
  const GlidePolar &polar = calculated.glide_polar_safety;
  if (!polar.IsValid())
    return;

  const double ballast_litres = polar.GetBallastLitres();
  const double dry_mass = polar.GetDryMass();
  const double reference_mass = polar.GetReferenceMass();
  const double overload = (dry_mass + ballast_litres) / reference_mass;
  const double ballast_fraction = polar.GetBallast();
  
  if (PutBallast(ballast_fraction, overload, env)) {
    const std::lock_guard lock{mutex};
    ballast_sent = true;
    last_sent_ballast_overload = overload;
  }
}

void
LXDevice::SyncBugs(const MoreData &basic,
                   const DerivedInfo &calculated,
                   OperationEnvironment &env) noexcept
{
  {
    const std::lock_guard lock{mutex};
    if (bugs_sent)
      return;
  }

  if (!basic.settings.bugs_available.IsValid())
    return;

  const double device_bugs = basic.settings.bugs;
  const bool is_echo = IsBugsEcho(basic.settings);
  
  if (!is_echo && device_bugs < 1.0) {
    /* Device has bugs that we didn't send, don't override it */
    const std::lock_guard lock{mutex};
    bugs_sent = true;
    return;
  }

  /* Device has no bugs or is echoing our value - send XCSoar's bugs */
  const double bugs = calculated.glide_polar_safety.GetBugs();
  if (PutBugs(bugs, env)) {
    const std::lock_guard lock{mutex};
    bugs_sent = true;
    last_sent_bugs = bugs;
  }
}
