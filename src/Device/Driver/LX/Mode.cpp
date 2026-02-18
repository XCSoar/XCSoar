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
#include "Engine/Task/TaskType.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Profile/Profile.hpp"
#include "system/Path.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"

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

      /* Request declaration H-records to get registration and
         competition ID for plane profile matching */
      {
        const std::lock_guard lock{mutex};
        if (!declaration_requested) {
          declaration_requested = true;
          device_declaration = {};
        }
      }
      PortWriteNMEA(port, "PLXVC,DECL,R,1,7", env);
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

  /* RECEIVE mode: match declaration to a plane profile */
  if (do_receive)
    MatchPlaneProfile();

  /* SEND mode: push XCSoar polar to device */
  if (do_send)
    SendPolarToDevice(calculated, env);

  /* Track polar changes to reset sent values on change */
  TrackPolarChanges(calculated);

  /* MC, ballast, bugs: bidirectional sync is handled by the generic
     XCSoar path (ApplyExternalSettings for device→XCSoar, and
     ActionInterface/DeviceDescriptor for XCSoar→device).
     DeviceDescriptor::settings_sent provides echo suppression. */

  /* Crew weight is pilot data - always push to device */
  SyncCrewWeight(basic, calculated, env);

  /* Empty weight only in SEND mode (airframe data) */
  SyncEmptyWeight(basic, calculated, env, do_send);

  /* Send navigation target to the vario via PLXVTARG.
     During ordered tasks the vario handles its own waypoint
     advancement from its declaration, so we only send targets
     for GoTo/Abort.  On transition back to Ordered we send
     one re-sync target so the vario switches away from the
     GoTo target. */
  {
    const auto task_type = calculated.common_stats.task_type;
    const auto prev_type = static_cast<TaskType>(last_task_type);
    const bool was_goto = (prev_type == TaskType::GOTO ||
                           prev_type == TaskType::ABORT);
    const bool is_goto = (task_type == TaskType::GOTO ||
                          task_type == TaskType::ABORT);
    const bool transitioning_back = was_goto && !is_goto &&
      task_type == TaskType::ORDERED;

    if (is_goto || transitioning_back) {
      if (backend_components &&
          backend_components->protected_task_manager) {
        const auto wp =
          backend_components->protected_task_manager->GetActiveWaypoint();

        if (wp && wp->location.IsValid()) {
          if (transitioning_back ||
              wp->name != last_sent_target_name ||
              wp->location != last_sent_target_location) {
            LXNAVVario::SetTarget(port, env,
                                  wp->name.c_str(),
                                  wp->location,
                                  wp->GetElevationOrZero());
            last_sent_target_name = wp->name;
            last_sent_target_location = wp->location;
          }
        }
      }
    }

    last_task_type = static_cast<uint8_t>(task_type);
  }
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

      /* Parse glider name and stall from stored POLAR value.
         The settings map stores the full value after
         "PLXV0,POL,W," as fields:
         a,b,c,load,ref,max,empty,pilot,name,stall */
      auto polar_val = GetLXNAVVarioSetting("POL");
      if (polar_val.empty())
        polar_val = GetLXNAVVarioSetting("POLAR");
      if (!polar_val.empty()) {
        NMEAInputLine pline{polar_val.c_str()};
        /* Skip a,b,c,load,ref,max,empty,pilot (8 fields) */
        pline.Skip(8);
        device_polar.name = std::string{pline.ReadView()};
        double stall_val = 0;
        if (pline.ReadChecked(stall_val))
          device_polar.stall = stall_val;
      }
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

  /* Derive wing area from reference mass and wing loading.
     Always update so XCSoar stays in sync with the device. */
  if (polar_load > 0 && polar_weight > 0) {
    const double area = polar_weight / polar_load;
    if (area > 0)
      polar.SetWingArea(area);
  }

  /* Reset VMax so the plane profile's max cruise speed doesn't
     cap the speed-to-fly solver when the polar comes from the
     device.  75 m/s (270 km/h) is a safe upper bound. */
  polar.SetVMax(75, false);

  polar.Update();

  if (backend_components && backend_components->calculation_thread)
    backend_components->SetTaskPolar(GetComputerSettings().polar);

  /* Update the active plane profile with received polar values */
  {
    Plane &plane = SetComputerSettings().plane;
    if (plane.plane_profile_active) {
      /* Regenerate polar shape points from coefficients.
         Use the existing 3 speeds if valid, otherwise pick
         standard speeds (90, 130, 180 km/h). */
      auto &ps = plane.polar_shape;
      if (ps.points[0].v <= 0 || ps.points[1].v <= 0 ||
          ps.points[2].v <= 0) {
        ps.points[0].v = 90.0 / 3.6;
        ps.points[1].v = 130.0 / 3.6;
        ps.points[2].v = 180.0 / 3.6;
      }
      for (auto &pt : ps.points)
        pt.w = a * pt.v * pt.v + b * pt.v + c;
      ps.reference_mass = polar_weight;

      if (empty_weight > 0)
        plane.empty_mass = empty_weight;
      if (polar_load > 0 && polar_weight > 0)
        plane.wing_area = polar_weight / polar_load;

      /* Reset max_speed so it doesn't cap speed-to-fly */
      plane.max_speed = 75.0;

      /* Save to disk */
      try {
        auto plane_path = Profile::GetPath("PlanePath");
        if (plane_path != nullptr)
          PlaneGlue::WriteFile(plane, plane_path);
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to save plane profile");
      }
    }
  }

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
LXDevice::MatchPlaneProfile()
{
  using namespace CommonInterface;

  /* Check if declaration is complete and not yet processed */
  std::string registration;
  std::string competition_id;
  std::string glider_type;
  {
    const std::lock_guard lock{mutex};
    if (!device_declaration.complete || device_declaration.processed)
      return;
    registration = device_declaration.registration;
    competition_id = device_declaration.competition_id;
    glider_type = device_declaration.glider_type;
  }

  if (registration.empty()) {
    const std::lock_guard lock{mutex};
    device_declaration.processed = true;
    return;
  }

  /* Check if current profile already matches */
  {
    const Plane &current = GetComputerSettings().plane;
    if (current.plane_profile_active &&
        current.registration.equals(registration.c_str())) {
      /* Already using the correct profile, update comp ID if needed */
      if (!competition_id.empty())
        SetComputerSettings().plane.competition_id.SetUTF8(
          competition_id.c_str());
      const std::lock_guard lock{mutex};
      device_declaration.processed = true;
      return;
    }
  }

  /* Scan all .xcp files for a matching registration */
  struct PlaneMatch {
    const std::string &target_reg;
    AllocatedPath found_path{nullptr};
  } match{registration};

  class MatchVisitor : public File::Visitor {
    PlaneMatch &match;
  public:
    explicit MatchVisitor(PlaneMatch &m) : match(m) {}
    void Visit(Path path, [[maybe_unused]] Path filename) override {
      if (match.found_path != nullptr)
        return;
      Plane p{};
      if (PlaneGlue::ReadFile(p, path) &&
          p.registration.equals(match.target_reg.c_str()))
        match.found_path = AllocatedPath{path};
    }
  } visitor{match};

  VisitDataFiles("*.xcp", visitor);

  if (match.found_path != nullptr) {
    /* Found a matching profile — activate it */
    Plane &plane = SetComputerSettings().plane;
    if (PlaneGlue::ReadFile(plane, match.found_path)) {
      /* Update competition ID from vario if available */
      if (!competition_id.empty())
        plane.competition_id.SetUTF8(competition_id.c_str());

      Profile::SetPath("PlanePath", match.found_path);
      PlaneGlue::Synchronize(plane, SetComputerSettings(),
                             SetComputerSettings().polar.glide_polar_task);
      if (backend_components && backend_components->calculation_thread)
        backend_components->SetTaskPolar(GetComputerSettings().polar);
      Profile::Save();

      const auto msg = fmt::format("Plane profile matched: {}",
                                   registration);
      Message::AddMessage(msg.c_str());
    }
  } else {
    /* No matching profile found — create a new one */
    /* Start from the current plane settings (which may already have
       polar data from ReceivePolarFromDevice) and overlay the
       declaration fields */
    Plane plane = GetComputerSettings().plane;
    plane.registration.SetUTF8(registration.c_str());
    plane.competition_id.SetUTF8(competition_id.c_str());
    plane.type.SetUTF8(glider_type.c_str());
    plane.polar_name.clear();

    /* If no polar shape data yet, populate from current GlidePolar */
    if (plane.polar_shape.reference_mass <= 0) {
      const auto &gp =
        GetComputerSettings().polar.glide_polar_task;
      if (gp.IsValid()) {
        plane.polar_shape.reference_mass = gp.GetReferenceMass();
        plane.empty_mass = gp.GetEmptyMass();
        plane.wing_area = gp.GetWingArea();
        plane.max_speed = 75.0;
        plane.max_ballast = gp.GetMaxBallast();

        /* Regenerate polar shape points from coefficients */
        const auto &coeffs = gp.GetCoefficients();
        if (coeffs.IsValid()) {
          constexpr double speeds[] = {25.0, 36.1, 50.0};
          for (unsigned i = 0; i < 3; ++i) {
            const double v = speeds[i];
            plane.polar_shape.points[i].v = v;
            plane.polar_shape.points[i].w =
              coeffs.a * v * v + coeffs.b * v + coeffs.c;
          }
        }
      }
    }

    const auto filename = fmt::format("{}.xcp", registration);
    const auto path = LocalPath(filename.c_str());

    try {
      PlaneGlue::WriteFile(plane, path);
      plane.plane_profile_active = true;
      SetComputerSettings().plane = plane;
      Profile::SetPath("PlanePath", path);
      Profile::Save();

      const auto msg = fmt::format("Plane profile created: {}",
                                   registration);
      Message::AddMessage(msg.c_str());
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to create plane profile");
    }
  }

  {
    const std::lock_guard lock{mutex};
    device_declaration.processed = true;
  }
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

  /* Read device-specific fields and check for duplicate sends
     in a single lock acquisition */
  double polar_load = 0;
  double max_weight = 0;
  std::string glider_name;
  double stall = 0;
  {
    const std::lock_guard lock{mutex};
    if (device_polar.valid) {
      /* Check if we already sent these exact values */
      if (fabs(device_polar.a - coeffs.a) < 0.0001 &&
          fabs(device_polar.b - coeffs.b) < 0.0001 &&
          fabs(device_polar.c - coeffs.c) < 0.0001 &&
          fabs(device_polar.polar_weight - ref_mass) < 0.1 &&
          fabs(device_polar.empty_weight - empty_mass) < 0.1 &&
          fabs(device_polar.pilot_weight - crew_mass) < 0.1)
        return;

      /* Preserve device metadata: polar_load is a metadata field on
         the vario that doesn't affect calculations.  Also preserve
         max_weight, glider name, and stall speed. */
      polar_load = device_polar.polar_load;
      max_weight = device_polar.max_weight;
      glider_name = device_polar.name;
      stall = device_polar.stall;
    }
  }

  /* Fall back: compute polar_load from wing area if no device value */
  if (polar_load <= 0) {
    const double wing_area = gp.GetWingArea();
    polar_load =
      (wing_area > 0 && ref_mass > 0) ? ref_mass / wing_area : 0;
  }

  if (!EnableNMEA(env))
    return;

  const auto cmd = fmt::format(
    "PLXV0,POLAR,W,{:.6f},{:.6f},{:.6f},{:.2f},{:.1f},{:.0f},{:.1f},{:.1f},{},{:.0f}",
    a_lx, b_lx, c_lx, polar_load, ref_mass,
    max_weight, empty_mass, crew_mass, glider_name, stall);
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
