// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapProfile.hpp"
#include "Current.hpp"
#include "TerrainConfig.hpp"
#include "AirspaceConfig.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "MapSettings.hpp"

#include <algorithm> // for std::clamp()

static bool
IsValidMapOrientation(unsigned value)
{
  switch (value) {
  case (unsigned)MapOrientation::TRACK_UP:
  case (unsigned)MapOrientation::NORTH_UP:
  case (unsigned)MapOrientation::TARGET_UP:
  case (unsigned)MapOrientation::HEADING_UP:
  case (unsigned)MapOrientation::WIND_UP:
    return true;
  }

  return false;
}

static void
Load(const ProfileMap &map, FAITriangleSettings &settings)
{
  FAITriangleSettings::Threshold threshold;
  if (map.GetEnum(ProfileKeys::FAITriangleThreshold, threshold) &&
      unsigned(threshold) < unsigned(FAITriangleSettings::Threshold::MAX))
    settings.threshold = threshold;
}

void
Profile::Load(const ProfileMap &map, MapSettings &settings)
{
  map.Get(ProfileKeys::CircleZoom, settings.circle_zoom_enabled);
  map.Get(ProfileKeys::MaxAutoZoomDistance, settings.max_auto_zoom_distance);
  map.Get(ProfileKeys::DrawTopography, settings.topography_enabled);

  LoadTerrainRendererSettings(map, settings.terrain);

  map.GetEnum(ProfileKeys::AircraftSymbol, settings.aircraft_symbol);

  map.Get(ProfileKeys::DetourCostMarker, settings.detour_cost_markers_enabled);
  map.GetEnum(ProfileKeys::DisplayTrackBearing, settings.display_ground_track);
  map.Get(ProfileKeys::AutoZoom, settings.auto_zoom_enabled);

  map.GetEnum(ProfileKeys::WindArrowStyle, settings.wind_arrow_style);

  map.GetEnum(ProfileKeys::SkyLinesTrafficMapMode, settings.skylines_traffic_map_mode);

  settings.waypoint.LoadFromProfile();

  Load(map, settings.airspace);

  map.Get(ProfileKeys::GliderScreenPosition, settings.glider_screen_position);

  bool orientation_found = false;

  if (unsigned Temp = (unsigned)MapOrientation::NORTH_UP;
      map.Get(ProfileKeys::OrientationCircling, Temp)) {
    orientation_found = true;

    if (IsValidMapOrientation(Temp))
      settings.circling_orientation = (MapOrientation)Temp;
  }

  
  if (unsigned Temp = (unsigned)MapOrientation::NORTH_UP;
      map.Get(ProfileKeys::OrientationCruise, Temp)) {
    orientation_found = true;

    if (IsValidMapOrientation(Temp))
      settings.cruise_orientation = (MapOrientation)Temp;
  }

  if (!orientation_found) {
    unsigned Temp = 1;
    map.Get(ProfileKeys::DisplayUpValue, Temp);
    switch (Temp) {
    case 0:
      settings.cruise_orientation = MapOrientation::TRACK_UP;
      settings.circling_orientation = MapOrientation::TRACK_UP;
      break;
    case 1:
      settings.cruise_orientation = MapOrientation::NORTH_UP;
      settings.circling_orientation = MapOrientation::NORTH_UP;
      break;
    case 2:
      settings.cruise_orientation = MapOrientation::TRACK_UP;
      settings.circling_orientation = MapOrientation::NORTH_UP;
      break;
    case 3:
      settings.cruise_orientation = MapOrientation::TRACK_UP;
      settings.circling_orientation = MapOrientation::TARGET_UP;
      break;
    case 4:
      settings.cruise_orientation = MapOrientation::NORTH_UP;
      settings.circling_orientation = MapOrientation::TRACK_UP;
      break;
    }
  }

  if (double tmp; map.Get(ProfileKeys::ClimbMapScale, tmp))
    settings.circling_scale = std::clamp(tmp / 10000, 0.0003, 10.);

  if (double tmp; map.Get(ProfileKeys::CruiseMapScale, tmp))
    settings.cruise_scale = std::clamp(tmp / 10000, 0.0003, 10.);

  map.GetEnum(ProfileKeys::MapShiftBias, settings.map_shift_bias);
  map.Get(ProfileKeys::EnableFLARMMap, settings.show_flarm_on_map);
  map.Get(ProfileKeys::FadeTraffic, settings.fade_traffic);

  map.Get(ProfileKeys::EnableThermalProfile, settings.show_thermal_profile);
  map.Get(ProfileKeys::EnableFinalGlideBarMC0,
          settings.final_glide_bar_mc0_enabled);
  map.GetEnum(ProfileKeys::FinalGlideBarDisplayMode,
              settings.final_glide_bar_display_mode);
  map.Get(ProfileKeys::ShowFAITriangleAreas,
          settings.show_fai_triangle_areas);
  ::Load(map, settings.fai_triangle_settings);

  map.Get(ProfileKeys::EnableVarioBar,
          settings.vario_bar_enabled);

  Load(map, settings.trail);
  Load(map, settings.item_list);

  map.Get(ProfileKeys::Show95PercentRuleHelpers,
          settings.show_95_percent_rule_helpers);
}

void
Profile::Load(const ProfileMap &map, TrailSettings &settings)
{
  map.Get(ProfileKeys::TrailDrift, settings.wind_drift_enabled);
  map.Get(ProfileKeys::SnailWidthScale, settings.scaling_enabled);
  map.GetEnum(ProfileKeys::SnailType, settings.type);
  map.GetEnum(ProfileKeys::SnailTrail, settings.length);
}

void
Profile::Load(const ProfileMap &map, MapItemListSettings &settings)
{
  map.Get(ProfileKeys::EnableLocationMapItem, settings.add_location);
  map.Get(ProfileKeys::EnableArrivalAltitudeMapItem, settings.add_arrival_altitude);
}
