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
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"

static WndForm* wf = NULL;

void
WaypointDisplayConfigPanel::UpdateVisibilities()
{
  WndProperty* wp = (WndProperty*)wf->FindByName(_T("prpAppUseSWLandablesRendering"));
  assert(wp != NULL);
  bool visible = (wp->GetDataField()->GetAsInteger() != 0);

  ShowFormControl(*wf, _T("prpAppLandableRenderingScale"), visible);
  ShowFormControl(*wf, _T("prpAppScaleRunwayLength"), visible);
}

void
WaypointDisplayConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  const WaypointRendererSettings &settings =
    CommonInterface::SettingsMap().waypoint;

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointLabels"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);
    dfe->addEnumText(_("Full name"), DISPLAYNAME, _("The full name of each waypoint is displayed."));
    dfe->addEnumText(_("First word of name"), DISPLAYUNTILSPACE, _("The first word of the waypoint name is displayed."));
    dfe->addEnumText(_("First 3 letters"), DISPLAYFIRSTTHREE, _("The first 3 letters of the waypoint name are displayed."));
    dfe->addEnumText(_("First 5 letters"), DISPLAYFIRSTFIVE, _("The first 5 letters of the waypoint name are displayed."));
    dfe->addEnumText(_("None"), DISPLAYNONE, _("No waypoint name is displayed."));
    dfe->Set(settings.display_text_type);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointArrivalHeightDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);
    dfe->addEnumText(_("None"), WP_ARRIVAL_HEIGHT_NONE, _("No arrival height is displayed."));
    dfe->addEnumText(_("Straight glide"), WP_ARRIVAL_HEIGHT_GLIDE, _("Straight glide arrival height (no terrain is considered)."));
    dfe->addEnumText(_("Terrain avoidance glide"), WP_ARRIVAL_HEIGHT_TERRAIN, _("Arrival height considering terrain avoidance"));
    dfe->addEnumText(_("Straight & terrain glide"), WP_ARRIVAL_HEIGHT_GLIDE_AND_TERRAIN, _("Both arrival heights are displayed."));
    dfe->Set(settings.arrival_height_display);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointLabelStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Rounded rectangle"), RoundedBlack);
    dfe->addEnumText(_("Outlined"), OutlinedInverted);
    dfe->Set(settings.landable_render_mode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointLabelSelection"));
  if (wp) {
    //Determines what waypoint labels are displayed for each waypoint (space permitting):&#10;
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);
    dfe->addEnumText(_("All"), wlsAllWaypoints,
                     _("All waypoint labels will be displayed."));
    dfe->addEnumText(_("Task waypoints & landables"),
                     wlsTaskAndLandableWaypoints,
                     _("All waypoints part of a task and all landables will be displayed."));
    dfe->addEnumText(_("Task waypoints"), wlsTaskWaypoints,
                     _("All waypoints part of a task will be displayed."));
    dfe->addEnumText(_("None"), wlsNoWaypoints,
                     _("No waypoint labels will be displayed."));
    dfe->Set(settings.label_selection);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndLandable"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);
    dfe->addEnumText(_("Purple circle"), wpLandableWinPilot,
                     _("Airports and outlanding fields are displayed as purple circles. If the waypoint is reachable a bigger green circle is added behind the purple one. If the waypoint is blocked by a mountain the green circle will be red instead."));
    dfe->addEnumText(_("B/W"), wpLandableAltA,
                     _("Airports and outlanding fields are displayed in white/grey. If the waypoint is reachable the color is changed to green. If the waypoint is blocked by a mountain the color is changed to red instead."));
    dfe->addEnumText(_("Traffic lights"), wpLandableAltB,
                     _("Airports and outlanding fields are displayed in the colors of a traffic light. Green if reachable, Orange if blocked by mountain and red if not reachable at all."));
    dfe->Set(settings.landable_style);
    wp->RefreshDisplay();
  }

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
  case DataField::daInc:
  case DataField::daDec:
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
  WndProperty *wp;

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

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndLandable"));
  if (wp) {
    if (settings.landable_style != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      settings.landable_style = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppIndLandable, settings.landable_style);
      changed = true;

      CommonInterface::main_window.look->waypoint.Initialise(settings);
    }
  }

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
