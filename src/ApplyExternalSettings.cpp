// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ApplyExternalSettings.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "ActionInterface.hpp"
#include "Device/MultipleDevices.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Plane/Plane.hpp"
#include "Plane/PlaneFileGlue.hpp"
#include "Plane/PlaneGlue.hpp"
#include "Profile/Profile.hpp"
#include "system/Path.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Util.hpp"
#include "LogFile.hpp"
#include "time/PeriodClock.hpp"

#include <fmt/format.h>

static bool
BallastProcessTimer() noexcept
{
  bool modified = false;

  static Validity last_fraction, last_overload;
  const ExternalSettings &settings = CommonInterface::Basic().settings;
  const Plane &plane = CommonInterface::GetComputerSettings().plane;
  const GlidePolar &polar = CommonInterface::GetComputerSettings().polar.glide_polar_task;

  if (settings.ballast_fraction_available.Modified(last_fraction)) {
    ActionInterface::SetBallastFraction(settings.ballast_fraction, false);
    modified = true;
  }

  last_fraction = settings.ballast_fraction_available;

  /* Ignore overload values below 0.8 -- these indicate
     uninitialized or implausible readings from the device */
  if (settings.ballast_overload_available.Modified(last_overload) &&
      settings.ballast_overload >= 0.8 &&
      plane.max_ballast > 0 &&
      plane.polar_shape.reference_mass > 0) {
    auto water_mass =
        (settings.ballast_overload * plane.polar_shape.reference_mass) -
         polar.GetCrewMass() - plane.empty_mass;
    if (water_mass < 0)
      water_mass = 0;
    ActionInterface::SetBallastLitres(water_mass, false);
    modified = true;
  }

  last_overload = settings.ballast_overload_available;

  return modified;
}

static bool
BallastLitresProcessTimer()
{
  bool modified = false;
  static Validity last_ballast_litres_validity;
  const NMEAInfo &basic = CommonInterface::Basic();
  if(basic.settings.ballast_litres_available.Modified(last_ballast_litres_validity)){
    PolarSettings &polar(CommonInterface::SetComputerSettings().polar);
    polar.glide_polar_task.SetBallastLitres(basic.settings.ballast_litres);
    modified = true;
  }
  last_ballast_litres_validity = basic.settings.ballast_litres_available;
  return modified;
}

static bool
BugsProcessTimer() noexcept
{
  bool modified = false;

  static Validity last;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (settings.bugs_available.Modified(last)) {
    ActionInterface::SetBugs(settings.bugs, false);
    modified = true;
  }

  last = settings.bugs_available;

  return modified;
}

static bool
QNHProcessTimer(OperationEnvironment &env) noexcept
{
  bool modified = false;

  ComputerSettings &settings_computer =
    CommonInterface::SetComputerSettings();
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (basic.settings.qnh_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = basic.settings.qnh;
    settings_computer.pressure_available = basic.settings.qnh_available;
    modified = true;
  }

  if (calculated.pressure_available.Modified(settings_computer.pressure_available)) {
    settings_computer.pressure = calculated.pressure;
    settings_computer.pressure_available = calculated.pressure_available;

    if (backend_components && backend_components->devices) {
      backend_components->devices->PutQNH(settings_computer.pressure, env);

      if (calculated.pressure_elevation_available.IsValid()) {
        int elevation = iround(calculated.pressure_elevation);
        backend_components->devices->PutElevation(elevation, env);
      }
    }

    modified = true;
  }

  return modified;
}

static bool
MacCreadyProcessTimer() noexcept
{
  bool modified = false;

  static ExternalSettings last_external_settings;
  static Validity last_auto_mac_cready;

  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (last_auto_mac_cready.Modified(calculated.auto_mac_cready_available)) {
    /* time warp, reset */
    last_auto_mac_cready.Clear();
    modified = true;
  }

  if (basic.settings.mac_cready_available.Modified(last_external_settings.mac_cready_available)) {
    ActionInterface::SetMacCready(basic.settings.mac_cready, false);
    modified = true;
  } else if (calculated.auto_mac_cready_available.Modified(last_auto_mac_cready)) {
    last_auto_mac_cready = calculated.auto_mac_cready_available;
    ActionInterface::SetMacCready(calculated.auto_mac_cready);
    modified = true;
  }

  last_external_settings = basic.settings;

  return modified;
}

static bool
RadioProcess() noexcept
{
  bool modified = false;

  const NMEAInfo &basic = CommonInterface::Basic();

  static Validity last_active_frequency;
  static Validity last_standby_frequency;
  static Validity last_swap_frequencies;

  if (basic.settings.has_active_frequency.Modified(last_active_frequency)) {
    ActionInterface::SetActiveFrequency(basic.settings.active_frequency, basic.settings.active_freq_name, false);
    last_active_frequency = basic.settings.has_active_frequency;
    modified = true;
  }

  if (basic.settings.has_standby_frequency.Modified(last_standby_frequency)) {
    ActionInterface::SetStandbyFrequency(basic.settings.standby_frequency, basic.settings.standby_freq_name, false);
    last_standby_frequency = basic.settings.has_standby_frequency;
    modified = true;
  }

  if (basic.settings.swap_frequencies.Modified(last_swap_frequencies)) {
    ActionInterface::ExchangeRadioFrequencies(false);
    last_swap_frequencies = basic.settings.swap_frequencies;
  }

  return modified;
}

static bool
TransponderProcess() noexcept
{
  bool modified = false;

  const NMEAInfo &basic = CommonInterface::Basic();

  static Validity last_transponder_code;
  static Validity last_transponder_mode;

  if (basic.settings.has_transponder_code.Modified(last_transponder_code)) {
    ActionInterface::SetTransponderCode(basic.settings.transponder_code,
                                        false);
    last_transponder_code = basic.settings.has_transponder_code;
    modified = true;
  }

  if (basic.settings.has_transponder_mode.Modified(last_transponder_mode)) {
    ActionInterface::SetTransponderMode(basic.settings.transponder_mode);
    last_transponder_mode = basic.settings.has_transponder_mode;
    modified = true;
  }

  return modified;
}

static bool
PolarProcessTimer() noexcept
{
  static Validity last_coefficients;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (!settings.polar_coefficients_available.Modified(last_coefficients)) {
    last_coefficients = settings.polar_coefficients_available;
    return false;
  }

  last_coefficients = settings.polar_coefficients_available;

  PolarCoefficients pc(settings.polar_a, settings.polar_b, settings.polar_c);
  if (!pc.IsValid())
    return false;

  using namespace CommonInterface;
  GlidePolar &polar = SetComputerSettings().polar.glide_polar_task;
  polar.SetCoefficients(pc, false);

  if (settings.polar_reference_mass_available &&
      settings.polar_reference_mass > 0)
    polar.SetReferenceMass(settings.polar_reference_mass, false);

  if (settings.polar_empty_weight_available &&
      settings.polar_empty_weight > 0)
    polar.SetEmptyMass(settings.polar_empty_weight, false);

  if (settings.polar_pilot_weight_available &&
      settings.polar_pilot_weight > 0)
    polar.SetCrewMass(settings.polar_pilot_weight, false);

  if (settings.polar_load_available && settings.polar_load > 0 &&
      settings.polar_reference_mass_available &&
      settings.polar_reference_mass > 0) {
    const double area = settings.polar_reference_mass / settings.polar_load;
    if (area > 0)
      polar.SetWingArea(area);
  }

  polar.SetVMax(DEFAULT_MAX_SPEED, false);
  polar.Update();

  if (backend_components)
    backend_components->SetTaskPolar(GetComputerSettings().polar);

  /* Update active plane profile with received polar values */
  Plane &plane = SetComputerSettings().plane;
  if (plane.plane_profile_active) {
    auto &ps = plane.polar_shape;
    if (ps.points[0].v <= 0 || ps.points[1].v <= 0 ||
        ps.points[2].v <= 0) {
      ps.points[0].v = 90.0 / 3.6;
      ps.points[1].v = 130.0 / 3.6;
      ps.points[2].v = 180.0 / 3.6;
    }
    for (auto &pt : ps.points)
      pt.w = pc.a * pt.v * pt.v + pc.b * pt.v + pc.c;

    if (settings.polar_reference_mass_available)
      ps.reference_mass = settings.polar_reference_mass;
    if (settings.polar_empty_weight_available &&
        settings.polar_empty_weight > 0)
      plane.empty_mass = settings.polar_empty_weight;
    if (settings.polar_load_available && settings.polar_load > 0 &&
        settings.polar_reference_mass_available &&
        settings.polar_reference_mass > 0)
      plane.wing_area = settings.polar_reference_mass / settings.polar_load;
    plane.max_speed = DEFAULT_MAX_SPEED;

    try {
      auto plane_path = Profile::GetPath("PlanePath");
      if (plane_path != nullptr)
        PlaneGlue::WriteFile(plane, plane_path);
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to save plane profile");
    }
  }

  Message::AddMessage(_("Polar received from device"));

  return true;
}

static bool
PolarSendProcessTimer(OperationEnvironment &env) noexcept
{
  static PeriodClock clock;
  if (!clock.CheckUpdate(std::chrono::seconds(10)))
    return false;

  if (!backend_components || !backend_components->devices)
    return false;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const GlidePolar &gp = calculated.glide_polar_safety;
  if (!gp.IsValid())
    return false;

  backend_components->devices->PutPolar(gp, env);
  return false;
}

static bool
PlaneProfileProcessTimer() noexcept
{
  static Validity last_registration;
  const ExternalSettings &settings = CommonInterface::Basic().settings;

  if (!settings.glider_registration_available.Modified(last_registration)) {
    last_registration = settings.glider_registration_available;
    return false;
  }

  last_registration = settings.glider_registration_available;

  if (settings.glider_registration.empty())
    return false;

  using namespace CommonInterface;

  /* Check if current profile already matches */
  const Plane &current = GetComputerSettings().plane;
  if (current.plane_profile_active &&
      current.registration.equals(settings.glider_registration.c_str())) {
    if (settings.glider_competition_id_available &&
        !settings.glider_competition_id.empty())
      SetComputerSettings().plane.competition_id.SetUTF8(
        settings.glider_competition_id.c_str());
    return true;
  }

  try {
    /* Search for matching .xcp profile */
    auto found_path = PlaneGlue::FindByRegistration(
      settings.glider_registration.c_str());

    if (found_path != nullptr) {
      Plane plane_copy = GetComputerSettings().plane;
      if (PlaneGlue::ReadFile(plane_copy, found_path)) {
        if (settings.glider_competition_id_available &&
            !settings.glider_competition_id.empty())
          plane_copy.competition_id.SetUTF8(
            settings.glider_competition_id.c_str());

        SetComputerSettings().plane = plane_copy;
        Profile::SetPath("PlanePath", found_path);
        PlaneGlue::Synchronize(plane_copy, SetComputerSettings(),
                               SetComputerSettings().polar.glide_polar_task);
        if (backend_components)
          backend_components->SetTaskPolar(GetComputerSettings().polar);
        Profile::Save();

        const char *reg = settings.glider_registration.c_str();
        const auto msg = fmt::vformat(
          _("Plane profile matched: {}"),
          fmt::make_format_args(reg));
        Message::AddMessage(msg.c_str());
      }
    } else {
      /* No matching profile — create a new one */
      const GlidePolar &gp =
        GetComputerSettings().polar.glide_polar_task;

      auto path = PlaneGlue::CreateFromPolar(
        settings.glider_registration.c_str(),
        settings.glider_competition_id_available
          ? settings.glider_competition_id.c_str() : "",
        settings.glider_type_available
          ? settings.glider_type.c_str() : "",
        gp);

      if (path != nullptr) {
        Plane plane{};
        if (PlaneGlue::ReadFile(plane, path)) {
          plane.plane_profile_active = true;
          SetComputerSettings().plane = plane;
          PlaneGlue::Synchronize(plane, SetComputerSettings(),
                                 SetComputerSettings().polar.glide_polar_task);
          if (backend_components)
            backend_components->SetTaskPolar(GetComputerSettings().polar);
          Profile::SetPath("PlanePath", path);
          Profile::Save();

          const char *reg = settings.glider_registration.c_str();
          const auto msg = fmt::vformat(
            _("Plane profile created: {}"),
            fmt::make_format_args(reg));
          Message::AddMessage(msg.c_str());
        }
      }
    }
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to match/create plane profile");
  }

  return true;
}

static bool
TargetProcessTimer(OperationEnvironment &env) noexcept
{
  if (!backend_components || !backend_components->devices ||
      !backend_components->protected_task_manager)
    return false;

  const DerivedInfo &calculated = CommonInterface::Calculated();
  const auto task_type = calculated.common_stats.task_type;

  static TaskType last_task_type = TaskType::NONE;
  static std::string last_sent_name;
  static GeoPoint last_sent_location = GeoPoint::Invalid();

  const bool was_goto = (last_task_type == TaskType::GOTO ||
                         last_task_type == TaskType::ABORT);
  const bool is_goto = (task_type == TaskType::GOTO ||
                        task_type == TaskType::ABORT);
  const bool transitioning_back = was_goto && !is_goto &&
    task_type == TaskType::ORDERED;

  if (is_goto || transitioning_back) {
    const auto wp =
      backend_components->protected_task_manager->GetActiveWaypoint();

    if (wp && wp->location.IsValid()) {
      if (transitioning_back ||
          wp->name != last_sent_name ||
          wp->location != last_sent_location) {
        backend_components->devices->PutTarget(
          wp->location, wp->name.c_str(),
          wp->has_elevation
            ? std::optional<double>(wp->elevation)
            : std::nullopt,
          env);
        last_sent_name = wp->name;
        last_sent_location = wp->location;
      }
    }
  }

  last_task_type = task_type;
  return false;
}

bool
ApplyExternalSettings(OperationEnvironment &env) noexcept
{
  bool modified = false;
  modified |= BallastLitresProcessTimer();
  modified |= BallastProcessTimer();
  modified |= BugsProcessTimer();
  modified |= QNHProcessTimer(env);
  modified |= MacCreadyProcessTimer();
  modified |= RadioProcess();
  modified |= TransponderProcess();
  modified |= PolarProcessTimer();
  PolarSendProcessTimer(env);
  PlaneProfileProcessTimer();
  TargetProcessTimer(env);
  return modified;
}
