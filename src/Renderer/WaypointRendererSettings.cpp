// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointRendererSettings.hpp"
#include "Profile/Profile.hpp"

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
}
