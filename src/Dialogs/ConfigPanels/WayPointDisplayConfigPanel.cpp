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

#include "WayPointDisplayConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Appearance.hpp"
#include "Screen/Graphics.hpp"
#include "Language.hpp"

static WndForm* wf = NULL;


void
WayPointDisplayConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

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
    dfe->Set(XCSoarInterface::SettingsMap().DisplayTextType);
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
    dfe->Set(XCSoarInterface::SettingsMap().WaypointArrivalHeightDisplay);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWaypointLabelStyle"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Rounded rectangle"), RoundedBlack);
    dfe->addEnumText(_("Outlined"), OutlinedInverted);
    dfe->Set(XCSoarInterface::SettingsMap().LandableRenderMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpWayPointLabelSelection"));
  if (wp) {
    //Determines what waypoint labels are displayed for each waypoint (space permitting):&#10;
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);
    dfe->addEnumText(_("All"), wlsAllWayPoints,
                     _("All waypoint labels will be displayed."));
    dfe->addEnumText(_("Task waypoints & landables"),
                     wlsTaskAndLandableWayPoints,
                     _("All waypoints part of a task and all landables will be displayed."));
    dfe->addEnumText(_("Task waypoints"), wlsTaskWayPoints,
                     _("All waypoints part of a task will be displayed."));
    dfe->addEnumText(_("None"), wlsNoWayPoints,
                     _("No waypoint labels will be displayed."));
    dfe->Set(XCSoarInterface::SettingsMap().WayPointLabelSelection);
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
    dfe->Set(Appearance.IndLandable);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpAppUseSWLandablesRendering"),
                   Appearance.UseSWLandablesRendering);

  LoadFormProperty(*wf, _T("prpAppLandableRenderingScale"),
                   Appearance.LandableRenderingScale);

  LoadFormProperty(*wf, _T("prpAppScaleRunwayLength"),
                   Appearance.ScaleRunwayLength);
}


bool
WayPointDisplayConfigPanel::Save()
{
  bool changed = false;
  WndProperty *wp;

  changed |= SaveFormPropertyEnum(*wf, _T("prpWaypointLabels"),
                                  szProfileDisplayText,
                                  XCSoarInterface::SetSettingsMap().DisplayTextType);

  changed |= SaveFormPropertyEnum(*wf, _T("prpWaypointArrivalHeightDisplay"),
                                  szProfileWaypointArrivalHeightDisplay,
                                  XCSoarInterface::SetSettingsMap().WaypointArrivalHeightDisplay);

  changed |= SaveFormPropertyEnum(*wf, _T("prpWaypointLabelStyle"),
                                  szProfileWaypointLabelStyle,
                                  XCSoarInterface::SetSettingsMap().LandableRenderMode);

  changed |= SaveFormPropertyEnum(*wf, _T("prpWayPointLabelSelection"),
                                  szProfileWayPointLabelSelection,
                                  XCSoarInterface::SetSettingsMap().WayPointLabelSelection);

  wp = (WndProperty*)wf->FindByName(_T("prpAppIndLandable"));
  if (wp) {
    if (Appearance.IndLandable != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndLandable = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      Profile::Set(szProfileAppIndLandable, Appearance.IndLandable);
      changed = true;
      Graphics::InitLandableIcons();
    }
  }

  changed |= SaveFormProperty(*wf, _T("prpAppUseSWLandablesRendering"),
                              szProfileAppUseSWLandablesRendering,
                              Appearance.UseSWLandablesRendering);

  changed |= SaveFormProperty(*wf, _T("prpAppLandableRenderingScale"),
                              szProfileAppLandableRenderingScale,
                              Appearance.LandableRenderingScale);

  changed |= SaveFormProperty(*wf, _T("prpAppScaleRunwayLength"),
                              szProfileAppScaleRunwayLength,
                              Appearance.ScaleRunwayLength);


  return changed;
}
