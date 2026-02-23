// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WarningComputer.hpp"
#include "Settings.hpp"
#include "NMEA/Aircraft.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "FLARM/AlertZone.hpp"
#include "FLARM/Data.hpp"
#include "TransponderCode.hpp"

#include <fmt/format.h>

using namespace std::chrono;

WarningComputer::WarningComputer(const AirspaceWarningConfig &_config,
                                 Airspaces &_airspaces)
  :airspaces(_airspaces),
   manager(_config, airspaces),
   protected_manager(manager)
{
}

void
WarningComputer::Update(const ComputerSettings &settings_computer,
                        const MoreData &basic,
                        const DerivedInfo &calculated,
                        AirspaceWarningsInfo &result)
{
  if (!basic.time_available)
    return;

  const auto dt = delta_time.Update(basic.time, seconds{1}, seconds{20});
  if (dt.count() < 0)
    /* time warp */
    Reset();

  if (dt.count() <= 0)
    return;

  airspaces.SetFlightLevels(settings_computer.pressure);

  AirspaceActivity day(calculated.date_time_local.day_of_week);
  airspaces.SetActivity(day);

  if (!settings_computer.airspace.enable_warnings ||
      !basic.location_available || !basic.NavAltitudeAvailable()) {
    if (initialised) {
      initialised = false;
      protected_manager.Clear();
    }

    return;
  }

  const AircraftState as = ToAircraftState(basic, calculated);
  ProtectedAirspaceWarningManager::ExclusiveLease lease(protected_manager);

  lease->SetConfig(settings_computer.airspace.warnings);

  if (!initialised) {
    initialised = true;
    lease->Reset(as);
  }

  {
    AirspaceWarningManager &mgr = lease;
    UpdateFlarmZones(basic.flarm, mgr);
  }

  if (lease->Update(as, settings_computer.polar.glide_polar_task,
                    calculated.task_stats,
                    calculated.circling,
                    round<duration<unsigned>>(dt)))
    result.latest.Update(basic.clock);
}

void
WarningComputer::UpdateFlarmZones(const FlarmData &flarm,
                                  AirspaceWarningManager &mgr)
{
  const auto &zones = flarm.alert_zones;

  // Remove cached airspaces for zones no longer present
  std::erase_if(flarm_zone_airspaces, [&](const auto &pair) {
    return zones.FindZone(pair.first) == nullptr;
  });

  std::vector<ConstAirspacePtr> external;

  for (const auto &zone : zones.list) {
    if (!zone.valid.IsValid())
      continue;

    auto &cache = flarm_zone_airspaces[zone.id];
    const bool changed = cache.airspace == nullptr ||
      cache.center != zone.center || cache.radius != zone.radius ||
      cache.bottom != zone.bottom || cache.top != zone.top ||
      cache.zone_type != zone.zone_type;

    if (changed) {
      cache.airspace = std::make_shared<AirspaceCircle>(zone.center, zone.radius);
      cache.center = zone.center;
      cache.radius = zone.radius;
      cache.bottom = zone.bottom;
      cache.top = zone.top;
      cache.zone_type = zone.zone_type;

      AirspaceAltitude base{}, top{};
      base.reference = AltitudeReference::MSL;
      base.altitude = zone.bottom;
      base.flight_level = 0;
      base.altitude_above_terrain = 0;
      top.reference = AltitudeReference::MSL;
      top.altitude = zone.top;
      top.flight_level = 0;
      top.altitude_above_terrain = 0;

      cache.airspace->SetProperties(
        fmt::format("FLARM: {}",
                    FlarmAlertZone::GetZoneTypeString(zone.zone_type)),
        std::string{}, TransponderCode::Null(),
        AirspaceClass::ALERT, AirspaceClass::ALERT,
        base, top);
    }

    external.push_back(cache.airspace);
  }

  mgr.SetExternalAirspaces(external);
}
