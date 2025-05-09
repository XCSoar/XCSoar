// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"

bool
IsAirspaceTypeOrClassVisible(const AbstractAirspace &airspace,
                      const AirspaceRendererSettings &renderer_settings)
{
  return renderer_settings.classes[airspace.GetClassOrType()].display || renderer_settings.classes[airspace.GetTypeOrClass()].display;
}

bool
IsAirspaceAltitudeVisible(const AbstractAirspace &airspace,
                          const AltitudeState &state,
                          const AirspaceComputerSettings &computer_settings,
                          const AirspaceRendererSettings &renderer_settings)
{
  /// @todo airspace visibility did use ToMSL(..., map.Calculated().TerrainAlt); 

  switch (renderer_settings.altitude_mode) {
  case AirspaceDisplayMode::ALLON:
    return true;

  case AirspaceDisplayMode::CLIP:
    return airspace.GetBase().GetAltitude(state) <= renderer_settings.clip_altitude;

  case AirspaceDisplayMode::AUTO:
    return airspace.GetBase().IsBelow(state, computer_settings.warnings.altitude_warning_margin)
      && airspace.GetTop().IsAbove(state, computer_settings.warnings.altitude_warning_margin);

  case AirspaceDisplayMode::ALLBELOW:
    return airspace.GetBase().IsBelow(state, computer_settings.warnings.altitude_warning_margin);

  case AirspaceDisplayMode::INSIDE:
    return (airspace.GetBase().IsBelow(state) && airspace.GetTop().IsAbove(state));

  case AirspaceDisplayMode::ALLOFF:
    return false;
  }
  return true;
}

bool
AirspaceVisibility::operator()(const AbstractAirspace &airspace) const
{
  return IsAirspaceTypeOrClassVisible(airspace, renderer_settings) &&
    IsAirspaceAltitudeVisible(airspace, state,
                              computer_settings, renderer_settings);
}
