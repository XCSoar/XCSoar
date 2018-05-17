/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "WaypointRendererSettings.hpp"
#include "Profile/Profile.hpp"

void
WaypointRendererSettings::LoadFromProfile()
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
