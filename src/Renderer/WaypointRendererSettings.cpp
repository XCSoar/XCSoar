// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointRendererSettings.hpp"
#include "Profile/Profile.hpp"
#include "util/StringFormat.hpp"

#include <cassert>

static const char *
MakeWaypointTypeDisplayName(char *buffer, size_t size, unsigned i) noexcept
{
  const int written = StringFormat(buffer, size, "WaypointTypeDisplay%u", i);
  assert(written > 0 && size_t(written) < size);
  if (written <= 0 || size_t(written) >= size)
    buffer[0] = '\0';
  return buffer;
}

void
WaypointRendererSettings::LoadFromProfile() noexcept
{
  using namespace Profile;

  // NOTE: WaypointLabelSelection must be loaded after this code
  GetEnum(ProfileKeys::DisplayText, display_text_type);
  if (display_text_type == DisplayTextType::OBSOLETE_DONT_USE_NAMEIFINTASK) {
    // pref migration. The migrated value of DisplayTextType and
    // WaypointLabelSelection will not be written to the config file
    // unless the user explicitly changes the corresponding setting manually.
    // This requires ordering because a manually changed WaypointLabelSelection
    // may be overwritten by the following migration code.
    display_text_type = DisplayTextType::NAME;
    label_selection = LabelSelection::TASK;
  } else if (display_text_type == DisplayTextType::OBSOLETE_DONT_USE_NUMBER)
    display_text_type = DisplayTextType::NAME;

  // NOTE: DisplayTextType must be loaded before this code
  //       due to pref migration dependencies!
  GetEnum(ProfileKeys::WaypointLabelSelection, label_selection);
  GetEnum(ProfileKeys::WaypointArrivalHeightDisplay, arrival_height_display);
  GetEnum(ProfileKeys::WaypointLabelStyle, landable_render_mode);

  GetEnum(ProfileKeys::AppIndLandable, landable_style);
  Get(ProfileKeys::AppUseSWLandablesRendering, vector_landable_rendering);
  Get(ProfileKeys::AppScaleRunwayLength, scale_runway_length);
  Get(ProfileKeys::AppLandableRenderingScale, landable_rendering_scale);
  Get(ProfileKeys::MapWaypointIconScale, map_waypoint_icon_scale);

  for (unsigned i = 0; i < unsigned(Waypoint::Type::COUNT); ++i) {
    char name[64];
    MakeWaypointTypeDisplayName(name, sizeof(name), i);
    Get(name, display_types[i]);
  }

  Get(ProfileKeys::WaypointDisplayNonIcaoAirports,
      display_non_icao_airports);
}

bool
WaypointRendererSettings::IsWaypointDisplayed(const Waypoint &waypoint) const noexcept
{
  if (!IsTypeDisplayed(waypoint.type))
    return false;

  if (!display_non_icao_airports && waypoint.IsAirport() &&
      waypoint.shortname.length() != 4)
    return false;

  return true;
}

void
WaypointRendererSettings::SaveTypeDisplay(Waypoint::Type type,
                                          bool display) noexcept
{
  const unsigned type_index = unsigned(type);
  if (type_index >= unsigned(Waypoint::Type::COUNT))
    return;

  display_types[type_index] = display;

  char name[64];
  MakeWaypointTypeDisplayName(name, sizeof(name), type_index);
  Profile::Set(name, display);
}

void
WaypointRendererSettings::SaveNonIcaoAirportsDisplay(bool display) noexcept
{
  display_non_icao_airports = display;
  Profile::Set(ProfileKeys::WaypointDisplayNonIcaoAirports, display);
}
