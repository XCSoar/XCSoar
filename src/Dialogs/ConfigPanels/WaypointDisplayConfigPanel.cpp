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

#include "WaypointDisplayConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;

void
WaypointDisplayConfigPanel::UpdateVisibilities()
{
  bool visible = GetFormValueBoolean(*wf, _T("prpAppUseSWLandablesRendering"));

  ShowFormControl(*wf, _T("prpAppLandableRenderingScale"), visible);
  ShowFormControl(*wf, _T("prpAppScaleRunwayLength"), visible);
}

void
WaypointDisplayConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const WaypointRendererSettings &settings =
    CommonInterface::SettingsMap().waypoint;

  static gcc_constexpr_data StaticEnumChoice wp_labels_list[] = {
    { DISPLAYNAME, N_("Full name"),
      N_("The full name of each waypoint is displayed.") },
    { DISPLAYUNTILSPACE, N_("First word of name"),
      N_("The first word of the waypoint name is displayed.") },
    { DISPLAYFIRSTTHREE, N_("First 3 letters"),
      N_("The first 3 letters of the waypoint name are displayed.") },
    { DISPLAYFIRSTFIVE, N_("First 5 letters"),
      N_("The first 5 letters of the waypoint name are displayed.") },
    { DISPLAYNONE, N_("None"), N_("No waypoint name is displayed.") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpWaypointLabels"), wp_labels_list,
                   settings.display_text_type);

  static gcc_constexpr_data StaticEnumChoice wp_arrival_list[] = {
    { WP_ARRIVAL_HEIGHT_NONE, N_("None"),
      N_("No arrival height is displayed.") },
    { WP_ARRIVAL_HEIGHT_GLIDE, N_("Straight glide"),
      N_("Straight glide arrival height (no terrain is considered).") },
    { WP_ARRIVAL_HEIGHT_TERRAIN, N_("Terrain avoidance glide"),
      N_("Arrival height considering terrain avoidance") },
    { WP_ARRIVAL_HEIGHT_GLIDE_AND_TERRAIN, N_("Straight & terrain glide"),
      N_("Both arrival heights are displayed.") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpWaypointArrivalHeightDisplay"), wp_arrival_list,
                   settings.arrival_height_display);

  static gcc_constexpr_data StaticEnumChoice wp_label_list[] = {
    { RM_ROUNDED_BLACK, N_("Rounded rectangle") },
    { RM_OUTLINED_INVERTED, N_("Outlined") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpWaypointLabelStyle"), wp_label_list,
                   settings.landable_render_mode);

  static gcc_constexpr_data StaticEnumChoice wp_selection_list[] = {
    { wlsAllWaypoints, N_("All"), N_("All waypoint labels will be displayed.") },
    { wlsTaskAndLandableWaypoints, N_("Task waypoints & landables"),
      N_("All waypoints part of a task and all landables will be displayed.") },
    { wlsTaskWaypoints, N_("Task waypoints"),
      N_("All waypoints part of a task will be displayed.") },
    { wlsNoWaypoints, N_("None"), N_("No waypoint labels will be displayed.") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpWaypointLabelSelection"), wp_selection_list,
                   settings.label_selection);

  static gcc_constexpr_data StaticEnumChoice wp_style_list[] = {
    { wpLandableWinPilot, N_("Purple circle"),
      N_("Airports and outlanding fields are displayed as purple circles. If the waypoint is reachable a bigger green circle is added behind the purple one. If the waypoint is blocked by a mountain the green circle will be red instead.") },
    { wpLandableAltA, N_("B/W"),
      N_("Airports and outlanding fields are displayed in white/grey. If the waypoint is reachable the color is changed to green. If the waypoint is blocked by a mountain the color is changed to red instead.") },
    { wpLandableAltB, N_("Traffic lights"),
      N_("Airports and outlanding fields are displayed in the colors of a traffic light. Green if reachable, Orange if blocked by mountain and red if not reachable at all.") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpAppIndLandable"), wp_style_list,
                   settings.landable_style);

  LoadFormProperty(*wf, _T("prpAppUseSWLandablesRendering"),
                   settings.vector_landable_rendering);

  LoadFormProperty(*wf, _T("prpAppLandableRenderingScale"),
                   settings.landable_rendering_scale);

  LoadFormProperty(*wf, _T("prpAppScaleRunwayLength"),
                   settings.scale_runway_length);

  UpdateVisibilities();
}

void
WaypointDisplayConfigPanel::OnRenderingTypeData(DataField *Sender,
                                                DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange: {
    UpdateVisibilities();
    break;
  }
  case DataField::daSpecial:
    return;
  }
}

bool
WaypointDisplayConfigPanel::Save()
{
  WaypointRendererSettings &settings =
    CommonInterface::SetSettingsMap().waypoint;

  bool changed = false;

  changed |= SaveFormPropertyEnum(*wf, _T("prpWaypointLabels"),
                                  szProfileDisplayText,
                                  settings.display_text_type);

  changed |= SaveFormPropertyEnum(*wf, _T("prpWaypointArrivalHeightDisplay"),
                                  szProfileWaypointArrivalHeightDisplay,
                                  settings.arrival_height_display);

  changed |= SaveFormPropertyEnum(*wf, _T("prpWaypointLabelStyle"),
                                  szProfileWaypointLabelStyle,
                                  settings.landable_render_mode);

  changed |= SaveFormPropertyEnum(*wf, _T("prpWaypointLabelSelection"),
                                  szProfileWaypointLabelSelection,
                                  settings.label_selection);

  changed |= SaveFormPropertyEnum(*wf, _T("prpAppIndLandable"),
                                  szProfileAppIndLandable,
                                  settings.landable_style);

  changed |= SaveFormProperty(*wf, _T("prpAppUseSWLandablesRendering"),
                              szProfileAppUseSWLandablesRendering,
                              settings.vector_landable_rendering);

  changed |= SaveFormProperty(*wf, _T("prpAppLandableRenderingScale"),
                              szProfileAppLandableRenderingScale,
                              settings.landable_rendering_scale);

  changed |= SaveFormProperty(*wf, _T("prpAppScaleRunwayLength"),
                              szProfileAppScaleRunwayLength,
                              settings.scale_runway_length);


  return changed;
}
