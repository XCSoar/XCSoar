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
#include "Plane/Plane.hpp"

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
      polar_requested = true;
    }
    RequestLXNAVVarioSetting("MC", env);
    RequestLXNAVVarioSetting("POLAR", env);
    {
      const std::lock_guard lock{mutex};
      last_polar_request.Update();
    }
    
    /* Ballast and bugs come via LXWP2, no need to request separately */
    /* Wait for settings to be received before proceeding with sync */
    return;
  }

  /* Periodically request POLAR from device to detect changes made on the device */
  bool should_request_polar = false;
  {
    const std::lock_guard lock{mutex};
    if (last_polar_request.CheckUpdate(std::chrono::seconds(30))) {
      /* Request POLAR every 30 seconds to detect device-side changes */
      should_request_polar = true;
    }
  }
  
  if (should_request_polar) {
    RequestLXNAVVarioSetting("POLAR", env);
  }

  /* Check if plane profile is active - if default plane (LS-815) is active, read from vario;
     if plane profile is active, push XCSoar settings to vario */
  using namespace CommonInterface;
  const Plane &plane = GetComputerSettings().plane;
  const bool plane_profile_active = plane.plane_profile_active;
  
  if (!plane_profile_active) {
    /* Default plane (LS-815) is active - read polar from vario and apply to XCSoar */
    /* Read polar from ExternalSettings (provided by Parser) */
    const ExternalSettings &ext_settings = basic.settings;
    
    if (ext_settings.polar_coefficients_available &&
        ext_settings.polar_reference_mass_available) {
      /* LXNAV devices use a format where v == 1 corresponds to 100 km/h (27.78 m/s).
       * Convert to XCSoar format (m/s) when reading from ExternalSettings. */
      constexpr double LX_POLAR_V_SCALE = 100.0 / 3.6; // 100 km/h in m/s = 27.78 m/s
      const double a_device = ext_settings.polar_a;
      const double b_device = ext_settings.polar_b;
      const double c_device = ext_settings.polar_c;
      const double a = a_device / (LX_POLAR_V_SCALE * LX_POLAR_V_SCALE);
      const double b = b_device / LX_POLAR_V_SCALE;
      const double c = c_device; // c is already in m/s
      const double polar_weight = ext_settings.polar_reference_mass;
      const double polar_load = ext_settings.polar_load_available ? 
                                ext_settings.polar_load : 0;
      const double empty_weight = ext_settings.polar_empty_weight_available ? 
                                  ext_settings.polar_empty_weight : 0;
      const double pilot_weight = ext_settings.polar_pilot_weight_available ? 
                                  ext_settings.polar_pilot_weight : 0;
      
      /* Check if this is new data */
      bool should_apply = false;
      {
        const std::lock_guard lock{mutex};
        if (!device_polar.valid) {
          should_apply = true;
        } else if (fabs(device_polar.a - a) > 0.001 ||
                   fabs(device_polar.b - b) > 0.001 ||
                   fabs(device_polar.c - c) > 0.001 ||
                   fabs(device_polar.empty_weight - empty_weight) > 0.1 ||
                   fabs(device_polar.pilot_weight - pilot_weight) > 0.1) {
          should_apply = true;
        } else {
          /* Check if XCSoar's polar actually matches - if not, we should apply */
          using namespace CommonInterface;
          const GlidePolar &polar_current = GetComputerSettings().polar.glide_polar_task;
          if (polar_current.IsValid()) {
            const PolarCoefficients &ref_coeffs = polar_current.GetCoefficients();
            const double ref_mass = polar_current.GetReferenceMass();
            const double empty_mass = polar_current.GetEmptyMass();
            const double current_crew_mass = polar_current.GetCrewMass();
            
            /* Check if XCSoar polar matches device polar (compare reference coefficients, not scaled) */
            /* Also compare crew mass if device provides a non-zero pilot weight */
            bool crew_differs = false;
            if (pilot_weight > 0) {
              crew_differs = fabs(current_crew_mass - pilot_weight) > 0.1;
            }
            /* If device sends 0 for pilot_weight, don't compare - keep XCSoar's current value */
            
            if (fabs(ref_coeffs.a - a) > 0.001 ||
                fabs(ref_coeffs.b - b) > 0.001 ||
                fabs(ref_coeffs.c - c) > 0.001 ||
                fabs(ref_mass - polar_weight) > 0.1 ||
                fabs(empty_mass - empty_weight) > 0.1 ||
                crew_differs) {
              should_apply = true;
            }
          } else {
            should_apply = true;
          }
        }
        
        if (should_apply) {
          device_polar.a = a;
          device_polar.b = b;
          device_polar.c = c;
          device_polar.polar_load = ext_settings.polar_load_available ? ext_settings.polar_load : 0;
          device_polar.polar_weight = polar_weight;
          device_polar.max_weight = ext_settings.polar_maximum_mass_available ? ext_settings.polar_maximum_mass : 0;
          device_polar.empty_weight = empty_weight;
          device_polar.pilot_weight = pilot_weight;
          device_polar.valid = true;
        }
      }
      
      if (should_apply) {
        /* Apply full polar from device to XCSoar */
        GlidePolar &polar = SetComputerSettings().polar.glide_polar_task;
        
        /* Apply polar coefficients (already converted to XCSoar format above) */
        PolarCoefficients pc(a, b, c);
        if (pc.IsValid()) {
          polar.SetCoefficients(pc, false);
          
          /* Apply reference mass (polar_weight) to polar, but don't set plane.polar_shape.reference_mass
           * when no plane profile is active - it should remain 0 */
          if (polar_weight > 0) {
            polar.SetReferenceMass(polar_weight, false);
          }
          
          /* Apply empty mass */
          if (empty_weight > 0) {
            polar.SetEmptyMass(empty_weight, false);
          }
          
          /* Apply crew mass (pilot weight) only if device provides a non-zero value.
           * If device sends 0, keep XCSoar's current/default crew mass */
          if (pilot_weight > 0) {
            polar.SetCrewMass(pilot_weight, false);
          }
          /* If pilot_weight is 0, don't change crew_mass - keep XCSoar's default or current value */
          
          /* Set wing area if not already set, estimated from reference mass and reference wing loading */
          if (polar.GetWingArea() <= 0 && polar_load > 0 && polar_weight > 0) {
            const double estimated_wing_area = polar_weight / polar_load;
            if (estimated_wing_area > 0) {
              polar.SetWingArea(estimated_wing_area);
            }
          }
          
          /* Do not set or calculate max_ballast in LXNAV driver */
          
          polar.Update();

          /* Trigger recalculation */
          if (backend_components && backend_components->calculation_thread) {
            backend_components->SetTaskPolar(GetComputerSettings().polar);
          }
        }
      }
    }
  } else {
    /* Plane profile is active - push XCSoar settings to vario */
    /* This is handled by the sync functions called at the end of OnCalculatedUpdate */
  }

  /* Check if polar has changed (e.g., due to plane profile activation) */
  const GlidePolar &polar = calculated.glide_polar_safety;
  if (polar.IsValid()) {
    const PolarCoefficients &current_coeffs = polar.GetCoefficients();
    const double current_reference_mass = polar.GetReferenceMass();
    const double current_empty_mass = polar.GetEmptyMass();
    const double current_crew_mass = polar.GetCrewMass();
    
    {
      const std::lock_guard lock{mutex};
      if (!tracked_polar.valid ||
          fabs(tracked_polar.a - current_coeffs.a) > 0.0001 ||
          fabs(tracked_polar.b - current_coeffs.b) > 0.0001 ||
          fabs(tracked_polar.c - current_coeffs.c) > 0.0001 ||
          fabs(tracked_polar.reference_mass - current_reference_mass) > 0.1 ||
          fabs(tracked_polar.empty_mass - current_empty_mass) > 0.1 ||
          fabs(tracked_polar.crew_mass - current_crew_mass) > 0.1) {
        /* Polar has changed (coefficients or masses) - reset last sent values for polar-related settings to trigger re-sync */
        last_sent_ballast_overload.reset();
        last_sent_crew_mass.reset();
        last_sent_empty_mass.reset();
        tracked_polar.a = current_coeffs.a;
        tracked_polar.b = current_coeffs.b;
        tracked_polar.c = current_coeffs.c;
        tracked_polar.reference_mass = current_reference_mass;
        tracked_polar.empty_mass = current_empty_mass;
        tracked_polar.crew_mass = current_crew_mass;
        tracked_polar.valid = true;
      }
    }
  }

  /* Sync all settings */
  /* If plane profile is active, push XCSoar settings to vario;
     if default plane is active, sync functions will handle reading from vario */
  SyncMacCready(basic, calculated, env, plane_profile_active);
  SyncBallast(basic, calculated, env, plane_profile_active);
  SyncBugs(basic, calculated, env, plane_profile_active);
  SyncCrewWeight(basic, calculated, env, plane_profile_active);
  SyncEmptyWeight(basic, calculated, env, plane_profile_active);
}

void
LXDevice::SyncMacCready(const MoreData &basic,
                        const DerivedInfo &calculated,
                        OperationEnvironment &env,
                        bool plane_profile_active) noexcept
{
  if (!plane_profile_active)
    return;

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
                      OperationEnvironment &env,
                      bool plane_profile_active) noexcept
{
  if (!plane_profile_active)
    return;

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
                   OperationEnvironment &env,
                   bool plane_profile_active) noexcept
{
  if (!plane_profile_active)
    return;

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
                         OperationEnvironment &env,
                         bool plane_profile_active)
{
  if (!plane_profile_active)
    return;

  const GlidePolar &polar = calculated.glide_polar_safety;
  if (!polar.IsValid())
    return;

  const double crew_mass = polar.GetCrewMass();
  constexpr double DEFAULT_CREW_MASS = 90.0; // Default crew weight in kg

  /* Check if device already has this value (echo) */
  {
    const std::lock_guard lock{mutex};
    if (IsCrewWeightEcho(basic.settings))
      return; // Device already has our value
  }

  /* Check if we already sent this exact value */
  {
    const std::lock_guard lock{mutex};
    if (last_sent_crew_mass.has_value() && 
        fabs(*last_sent_crew_mass - crew_mass) < 0.1)
      return; // Already sent this value
  }

  /* If crew weight is at default, check if device has a pilot weight - if so, don't override it */
  if (fabs(crew_mass - DEFAULT_CREW_MASS) < 0.1) {
    if (basic.settings.polar_pilot_weight_available && 
        basic.settings.polar_pilot_weight > 0) {
      /* Device has pilot weight, don't override it */
      return;
    }
  }

  /* Crew weight is not default or device has no pilot weight - send XCSoar's crew weight */
  if (EnableNMEA(env)) {
    LXNAVVario::SetPilotWeight(port, env, crew_mass);
    const std::lock_guard lock{mutex};
    last_sent_crew_mass = crew_mass;
  }
}

void
LXDevice::SyncEmptyWeight([[maybe_unused]] const MoreData &basic,
                          const DerivedInfo &calculated,
                          OperationEnvironment &env,
                          bool plane_profile_active)
{
  if (!plane_profile_active)
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
