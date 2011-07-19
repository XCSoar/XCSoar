/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
  GetEnum(szProfileDisplayText, display_text_type);
  if (display_text_type == OBSOLETE_DONT_USE_DISPLAYNAMEIFINTASK) {
    // pref migration. The migrated value of DisplayTextType and
    // WaypointLabelSelection will not be written to the config file
    // unless the user explicitly changes the corresponding setting manually.
    // This requires ordering because a manually changed WaypointLabelSelection
    // may be overwritten by the following migration code.
    display_text_type = DISPLAYNAME;
    label_selection = wlsTaskWaypoints;
  } else if (display_text_type == OBSOLETE_DONT_USE_DISPLAYNUMBER)
    display_text_type = DISPLAYNAME;

  // NOTE: DisplayTextType must be loaded before this code
  //       due to pref migration dependencies!
  GetEnum(szProfileWaypointLabelSelection, label_selection);
  GetEnum(szProfileWaypointArrivalHeightDisplay, arrival_height_display);
  GetEnum(szProfileWaypointLabelStyle, landable_render_mode);
}
