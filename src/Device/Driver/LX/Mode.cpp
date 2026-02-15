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
#include "Interface.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"

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
LXDevice::OnCalculatedUpdate(const MoreData &basic,
                             const DerivedInfo &calculated)
{
  /* Only handle settings sync for LXNAV varios */
  if (!IsLXNAVVario())
    return;

  const bool do_receive =
    polar_sync == DeviceConfig::PolarSync::RECEIVE;
  const bool do_send =
    polar_sync == DeviceConfig::PolarSync::SEND;

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
      if (!mc_requested)
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
    }

    /* Ballast and bugs come via LXWP2, no need to request separately */
    /* Wait for settings to be received before proceeding with sync */
    return;
  }

  /* In RECEIVE mode, periodically request POLAR to detect
     changes made on the device */
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

  using namespace CommonInterface;

  /* RECEIVE mode: adopt device polar into XCSoar */
  if (do_receive)
    ReceivePolarFromDevice(basic);

  /* SEND mode: push XCSoar polar to device */
  if (do_send)
    SendPolarToDevice(calculated, env);

  /* Track polar changes to reset sent values on change */
  TrackPolarChanges(calculated);

  /* MC, ballast, bugs: always "last change wins" bidirectional,
     independent of polar_sync.  The RECEIVE direction is handled
     by ApplyExternalSettings via sync_from_device. */
  SyncMacCready(basic, calculated, env);
  SyncBallast(basic, calculated, env);
  SyncBugs(basic, calculated, env);

  /* Crew weight is pilot data - always push to device */
  SyncCrewWeight(basic, calculated, env);

  /* Empty weight only in SEND mode (airframe data) */
  SyncEmptyWeight(basic, calculated, env, do_send);
}

void
LXDevice::ReceivePolarFromDevice(const MoreData &basic) noexcept
{
  using namespace CommonInterface;
  const ExternalSettings &ext_settings = basic.settings;

  if (!ext_settings.polar_coefficients_available ||
      !ext_settings.polar_reference_mass_available)
    return;

  /* LXNAV devices use a format where v == 1 corresponds to
     100 km/h (27.78 m/s).  Convert to XCSoar format (m/s). */
  constexpr double LX_V =
    100.0 / 3.6; // 100 km/h in m/s
  const double a = ext_settings.polar_a / (LX_V * LX_V);
  const double b = ext_settings.polar_b / LX_V;
  const double c = ext_settings.polar_c;
  const double polar_weight = ext_settings.polar_reference_mass;
  const double polar_load =
    ext_settings.polar_load_available
    ? ext_settings.polar_load : 0;
  const double empty_weight =
    ext_settings.polar_empty_weight_available
    ? ext_settings.polar_empty_weight : 0;
  const double pilot_weight =
    ext_settings.polar_pilot_weight_available
    ? ext_settings.polar_pilot_weight : 0;

  /* Check if this is new data compared to what we already applied */
  bool should_apply = false;
  {
    const std::lock_guard lock{mutex};
    if (!device_polar.valid ||
        fabs(device_polar.a - a) > 0.001 ||
        fabs(device_polar.b - b) > 0.001 ||
        fabs(device_polar.c - c) > 0.001 ||
        fabs(device_polar.empty_weight - empty_weight) > 0.1 ||
        fabs(device_polar.pilot_weight - pilot_weight) > 0.1) {
      should_apply = true;
    } else {
      /* Also check if XCSoar's polar actually matches */
      const GlidePolar &gp =
        GetComputerSettings().polar.glide_polar_task;
      if (gp.IsValid()) {
        const PolarCoefficients &rc = gp.GetCoefficients();
        bool crew_differs = pilot_weight > 0 &&
          fabs(gp.GetCrewMass() - pilot_weight) > 0.1;
        if (fabs(rc.a - a) > 0.001 ||
            fabs(rc.b - b) > 0.001 ||
            fabs(rc.c - c) > 0.001 ||
            fabs(gp.GetReferenceMass() - polar_weight) > 0.1 ||
            fabs(gp.GetEmptyMass() - empty_weight) > 0.1 ||
            crew_differs)
          should_apply = true;
      } else {
        should_apply = true;
      }
    }

    if (should_apply) {
      device_polar.a = a;
      device_polar.b = b;
      device_polar.c = c;
      device_polar.polar_load = polar_load;
      device_polar.polar_weight = polar_weight;
      device_polar.max_weight =
        ext_settings.polar_maximum_mass_available
        ? ext_settings.polar_maximum_mass : 0;
      device_polar.empty_weight = empty_weight;
      device_polar.pilot_weight = pilot_weight;
      device_polar.valid = true;
    }
  }

  if (!should_apply)
    return;

  /* Apply the device polar to XCSoar */
  PolarCoefficients pc(a, b, c);
  if (!pc.IsValid())
    return;

  GlidePolar &polar = SetComputerSettings().polar.glide_polar_task;
  polar.SetCoefficients(pc, false);

  if (polar_weight > 0)
    polar.SetReferenceMass(polar_weight, false);
  if (empty_weight > 0)
    polar.SetEmptyMass(empty_weight, false);
  if (pilot_weight > 0)
    polar.SetCrewMass(pilot_weight, false);

  /* Estimate wing area from reference mass and wing loading */
  if (polar.GetWingArea() <= 0 &&
      polar_load > 0 && polar_weight > 0) {
    const double area = polar_weight / polar_load;
    if (area > 0)
      polar.SetWingArea(area);
  }

  polar.Update();

  if (backend_components && backend_components->calculation_thread)
    backend_components->SetTaskPolar(GetComputerSettings().polar);

  /* One-time notification */
  bool should_notify = false;
  {
    const std::lock_guard lock{mutex};
    if (!polar_sync_notified) {
      polar_sync_notified = true;
      should_notify = true;
    }
  }
  if (should_notify)
    Message::AddMessage(_("Polar received from device"));
}

void
LXDevice::SendPolarToDevice(const DerivedInfo &calculated,
                            OperationEnvironment &env)
{
  using namespace CommonInterface;
  const GlidePolar &gp = calculated.glide_polar_safety;
  if (!gp.IsValid())
    return;

  const PolarCoefficients &coeffs = gp.GetCoefficients();
  if (!coeffs.IsValid())
    return;

  /* Convert XCSoar m/s coefficients to LXNAV format
     (v == 1 corresponds to 100 km/h = 27.78 m/s) */
  constexpr double LX_V = 100.0 / 3.6;
  const double a_lx = coeffs.a * (LX_V * LX_V);
  const double b_lx = coeffs.b * LX_V;
  const double c_lx = coeffs.c;

  const double ref_mass = gp.GetReferenceMass();
  const double empty_mass = gp.GetEmptyMass();
  const double crew_mass = gp.GetCrewMass();
  const double wing_area = gp.GetWingArea();
  const double polar_load =
    (wing_area > 0 && ref_mass > 0) ? ref_mass / wing_area : 0;

  /* Check if we already sent these exact values */
  {
    const std::lock_guard lock{mutex};
    if (device_polar.valid &&
        fabs(device_polar.a - coeffs.a) < 0.0001 &&
        fabs(device_polar.b - coeffs.b) < 0.0001 &&
        fabs(device_polar.c - coeffs.c) < 0.0001 &&
        fabs(device_polar.polar_weight - ref_mass) < 0.1 &&
        fabs(device_polar.empty_weight - empty_mass) < 0.1 &&
        fabs(device_polar.pilot_weight - crew_mass) < 0.1)
      return;
  }

  if (!EnableNMEA(env))
    return;

  /* PLXV0,POLAR,W,a,b,c,polar_load,polar_weight,max_weight,
     empty_weight,pilot_weight,name,stall */
  const auto cmd = fmt::format(
    "PLXV0,POLAR,W,{:.6f},{:.6f},{:.6f},{:.2f},{:.1f},,{:.1f},{:.1f},,",
    a_lx, b_lx, c_lx, polar_load, ref_mass, empty_mass, crew_mass);
  PortWriteNMEA(port, cmd.c_str(), env);

  bool notify = false;
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

    if (!polar_sync_notified) {
      polar_sync_notified = true;
      notify = true;
    }
  }
  if (notify)
    Message::AddMessage(_("Polar sent to device"));
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

void
LXDevice::SyncMacCready(const MoreData &basic,
                        const DerivedInfo &calculated,
                        OperationEnvironment &env) noexcept
{
  const double mc = calculated.glide_polar_safety.GetMC();
  
  /* Check if device already has this value (echo) */
  {
    const std::lock_guard lock{mutex};
    if (IsMCEcho(basic.settings))
      return; // Device already has our value
  }
  
  /* Check if we already sent this exact value */
  {
    const std::lock_guard lock{mutex};
    if (last_sent_mc.has_value() && fabs(*last_sent_mc - mc) < 0.01)
      return; // Already sent this value
  }
  
  if (PutMacCready(mc, env)) {
    const std::lock_guard lock{mutex};
    last_sent_mc = mc;
  }
}

void
LXDevice::SyncBallast(const MoreData &basic,
                      const DerivedInfo &calculated,
                      OperationEnvironment &env) noexcept
{
  const GlidePolar &polar = calculated.glide_polar_safety;
  if (!polar.IsValid())
    return;

  const double ballast_litres = polar.GetBallastLitres();
  const double dry_mass = polar.GetDryMass();
  const double reference_mass = polar.GetReferenceMass();
  if (reference_mass <= 0)
    return;

  const double overload = (dry_mass + ballast_litres) / reference_mass;
  
  /* Check if device already has this value (echo) */
  {
    const std::lock_guard lock{mutex};
    if (IsBallastEcho(basic.settings))
      return; // Device already has our value
  }
  
  /* Check if we already sent this exact value */
  {
    const std::lock_guard lock{mutex};
    if (last_sent_ballast_overload.has_value() && 
        fabs(*last_sent_ballast_overload - overload) < 0.01)
      return; // Already sent this value
  }
  
  if (PutBallast(0.0, overload, env)) {
    const std::lock_guard lock{mutex};
    last_sent_ballast_overload = overload;
  }
}

void
LXDevice::SyncBugs(const MoreData &basic,
                   const DerivedInfo &calculated,
                   OperationEnvironment &env) noexcept
{
  const double bugs = calculated.glide_polar_safety.GetBugs();
  
  /* Check if device already has this value (echo) */
  {
    const std::lock_guard lock{mutex};
    if (IsBugsEcho(basic.settings))
      return; // Device already has our value
  }
  
  /* Check if we already sent this exact value */
  {
    const std::lock_guard lock{mutex};
    if (last_sent_bugs.has_value() && fabs(*last_sent_bugs - bugs) < 0.01)
      return; // Already sent this value
  }
  
  if (PutBugs(bugs, env)) {
    const std::lock_guard lock{mutex};
    last_sent_bugs = bugs;
  }
}

void
LXDevice::SyncCrewWeight([[maybe_unused]] const MoreData &basic,
                         const DerivedInfo &calculated,
                         OperationEnvironment &env)
{
  const GlidePolar &polar = calculated.glide_polar_safety;
  if (!polar.IsValid())
    return;

  const double xcsoar_crew = polar.GetCrewMass();
  const double device_crew =
    basic.settings.polar_pilot_weight_available
    ? basic.settings.polar_pilot_weight : 0;

  /* If either side is 0 the non-zero value wins;
     both non-zero → last change wins (bidirectional) */

  if (xcsoar_crew < 0.1 && device_crew < 0.1)
    return; // both unset

  if (xcsoar_crew < 0.1 && device_crew >= 0.1) {
    /* Device has a value, XCSoar doesn't → adopt device value */
    GlidePolar &gp =
      CommonInterface::SetComputerSettings().polar.glide_polar_task;
    gp.SetCrewMass(device_crew);
    if (backend_components && backend_components->calculation_thread)
      backend_components->SetTaskPolar(
        CommonInterface::GetComputerSettings().polar);
    const std::lock_guard lock{mutex};
    last_sent_crew_mass = device_crew;
    return;
  }

  /* XCSoar has a value → push to device (covers both
     "device is 0" and "both non-zero, last change wins") */

  {
    const std::lock_guard lock{mutex};
    if (IsCrewWeightEcho(basic.settings))
      return;

    if (last_sent_crew_mass.has_value() &&
        fabs(*last_sent_crew_mass - xcsoar_crew) < 0.1)
      return;
  }

  if (EnableNMEA(env)) {
    LXNAVVario::SetPilotWeight(port, env, xcsoar_crew);
    const std::lock_guard lock{mutex};
    last_sent_crew_mass = xcsoar_crew;
  }
}

void
LXDevice::SyncEmptyWeight([[maybe_unused]] const MoreData &basic,
                          const DerivedInfo &calculated,
                          OperationEnvironment &env,
                          bool send_to_device)
{
  if (!send_to_device)
    return;

  const GlidePolar &polar = calculated.glide_polar_safety;
  if (!polar.IsValid())
    return;

  const double empty_mass = polar.GetEmptyMass();
  
  {
    const std::lock_guard lock{mutex};
    if (IsEmptyWeightEcho(basic.settings))
      return;

    if (last_sent_empty_mass.has_value() &&
        fabs(*last_sent_empty_mass - empty_mass) < 0.1)
      return;
  }

  if (EnableNMEA(env)) {
    LXNAVVario::SetEmptyWeight(port, env, empty_mass);
    const std::lock_guard lock{mutex};
    last_sent_empty_mass = empty_mass;
  }
}
