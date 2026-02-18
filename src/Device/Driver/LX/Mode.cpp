// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "LXNAVVario.hpp"
#include "LX1600.hpp"
#include "NanoProtocol.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Operation/Operation.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "util/NumberParser.hpp"

#include <fmt/format.h>

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

  mc_requested = false;
  last_sent_mc.reset();
  ballast_requested = false;
  last_sent_ballast_overload.reset();
  last_sent_crew_mass.reset();
  bugs_requested = false;
  last_sent_bugs.reset();
  polar_requested = false;
  last_sent_empty_mass.reset();
  tracked_polar.valid = false;
  device_polar.valid = false;
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
  bool should_enable_direct = false;
  {
    const std::lock_guard lock{mutex};
    should_enable_direct = IsLXNAVVario() || use_pass_through;
    
    /* If already in pass-through mode and we don't need to enable direct,
       we can return early. Otherwise, we need to ensure the device is in
       the correct mode. */
    if (mode == Mode::PASS_THROUGH && !should_enable_direct)
      return true;
  }

  if (should_enable_direct) {
    LXNAVVario::ModeDirect(port, env);
    
    /* make sure the direct mode command has been sent to the device
       before we continue. The vario needs time to switch modes before
       it can forward commands to the FLARM. */
    port.Drain();
    
    /* Additional delay to allow the vario to complete the mode switch.
       Similar to the delay in EnableCommandMode() for baud rate changes. */
    env.Sleep(std::chrono::milliseconds(100));
  }

  {
    const std::lock_guard lock{mutex};
    mode = Mode::PASS_THROUGH;
  }
  return true;
}

bool
LXDevice::EnableLoggerNMEA(OperationEnvironment &env)
{
  /* S-series varios have a built-in logger that understands PLXVC
     commands in normal NMEA mode.  If a FLARM is behind the vario,
     its flight downloads are handled by the FLARM driver on a
     separate device slot. */
  if (IsSVario())
    return EnableNMEA(env);

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
LXDevice::OnCalculatedUpdate([[maybe_unused]] const MoreData &basic,
                             const DerivedInfo &calculated)
{
  if (!IsLXNAVVario())
    return;

  const bool do_receive =
    polar_sync == DeviceConfig::PolarSync::RECEIVE;

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
    {
      const std::lock_guard lock{mutex};
      mc_requested = true;
      ballast_requested = true;
      bugs_requested = true;
    }
    RequestLXNAVVarioSetting("MC", env);

    if (do_receive) {
      {
        const std::lock_guard lock{mutex};
        polar_requested = true;
        last_polar_request.Update();
      }
      RequestLXNAVVarioSetting("POLAR", env);

      {
        const std::lock_guard lock{mutex};
        if (!declaration_requested) {
          declaration_requested = true;
          device_declaration = {};
        }
      }
      PortWriteNMEA(port, "PLXVC,DECL,R,1,7", env);
    }

    return;
  }

  /* Periodically request POLAR in RECEIVE mode */
  if (do_receive) {
    bool should_request_polar = false;
    {
      const std::lock_guard lock{mutex};
      if (last_polar_request.CheckUpdate(std::chrono::seconds(30)))
        should_request_polar = true;
    }

    if (should_request_polar)
      RequestLXNAVVarioSetting("POLAR", env);
  }

  TrackPolarChanges(calculated);
}

bool
LXDevice::PutPolar(const GlidePolar &polar,
                   OperationEnvironment &env)
{
  if (!IsLXNAVVario())
    return true;

  if (!polar.IsValid())
    return true;

  const PolarCoefficients &coeffs = polar.GetCoefficients();
  if (!coeffs.IsValid())
    return true;

  /* Convert XCSoar m/s coefficients to LXNAV format
     (v == 1 corresponds to 100 km/h) */
  const double a_lx = coeffs.a * (LX_POLAR_V * LX_POLAR_V);
  const double b_lx = coeffs.b * LX_POLAR_V;
  const double c_lx = coeffs.c;

  const double ref_mass = polar.GetReferenceMass();
  const double empty_mass = polar.GetEmptyMass();
  const double crew_mass = polar.GetCrewMass();

  double polar_load = 0;
  double max_weight = 0;
  std::string glider_name;
  double stall = 0;
  {
    const std::lock_guard lock{mutex};
    if (device_polar.valid) {
      if (fabs(device_polar.a - coeffs.a) < 0.0001 &&
          fabs(device_polar.b - coeffs.b) < 0.0001 &&
          fabs(device_polar.c - coeffs.c) < 0.0001 &&
          fabs(device_polar.polar_weight - ref_mass) < 0.1 &&
          fabs(device_polar.empty_weight - empty_mass) < 0.1 &&
          fabs(device_polar.pilot_weight - crew_mass) < 0.1)
        return true;

      polar_load = device_polar.polar_load;
      max_weight = device_polar.max_weight;
      glider_name = device_polar.name;
      stall = device_polar.stall;
    }
  }

  if (polar_load <= 0) {
    const double wing_area = polar.GetWingArea();
    polar_load =
      (wing_area > 0 && ref_mass > 0) ? ref_mass / wing_area : 0;
  }

  if (!EnableNMEA(env))
    return false;

  std::erase_if(glider_name, [](char c) {
    return c == ',' || c == '*' || c == '\r' || c == '\n';
  });

  const auto cmd = fmt::format(
    "PLXV0,POLAR,W,{:.6f},{:.6f},{:.6f},{:.2f},{:.1f},{:.0f},"
    "{:.1f},{:.1f},{},{:.0f}",
    a_lx, b_lx, c_lx, polar_load, ref_mass,
    max_weight, empty_mass, crew_mass, glider_name, stall);
  PortWriteNMEA(port, cmd.c_str(), env);

  {
    const std::lock_guard lock{mutex};
    device_polar.a = coeffs.a;
    device_polar.b = coeffs.b;
    device_polar.c = coeffs.c;
    device_polar.polar_load = polar_load;
    device_polar.polar_weight = ref_mass;
    device_polar.empty_weight = empty_mass;
    device_polar.pilot_weight = crew_mass;
    device_polar.valid = true;
  }

  return true;
}

bool
LXDevice::PutTarget(const GeoPoint &location, const char *name,
                    std::optional<double> elevation,
                    OperationEnvironment &env)
{
  if (!IsLXNAVVario())
    return true;

  LXNAVVario::SetTarget(port, env, name, location,
                         elevation.value_or(0));
  return true;
}

void
LXDevice::TrackPolarChanges(const DerivedInfo &calculated) noexcept
{
  const GlidePolar &polar = calculated.glide_polar_safety;
  if (!polar.IsValid())
    return;

  const PolarCoefficients &cc = polar.GetCoefficients();
  const double rm = polar.GetReferenceMass();
  const double em = polar.GetEmptyMass();
  const double cm = polar.GetCrewMass();

  const std::lock_guard lock{mutex};
  if (!tracked_polar.valid ||
      fabs(tracked_polar.a - cc.a) > 0.0001 ||
      fabs(tracked_polar.b - cc.b) > 0.0001 ||
      fabs(tracked_polar.c - cc.c) > 0.0001 ||
      fabs(tracked_polar.reference_mass - rm) > 0.1 ||
      fabs(tracked_polar.empty_mass - em) > 0.1 ||
      fabs(tracked_polar.crew_mass - cm) > 0.1) {
    last_sent_ballast_overload.reset();
    last_sent_crew_mass.reset();
    last_sent_empty_mass.reset();
    tracked_polar.a = cc.a;
    tracked_polar.b = cc.b;
    tracked_polar.c = cc.c;
    tracked_polar.reference_mass = rm;
    tracked_polar.empty_mass = em;
    tracked_polar.crew_mass = cm;
    tracked_polar.valid = true;
  }
}

