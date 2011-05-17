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

#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "MapDisplayConfigPanel.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;

void
MapDisplayConfigPanel::UpdateVisibilities()
{
  WndProperty* wp_shift = (WndProperty*)wf->FindByName(_T("prpMapShiftBias"));
  assert(wp_shift != NULL);
  WndProperty* wp_pos = (WndProperty*)wf->FindByName(_T("prpGliderScreenPosition"));
  assert(wp_pos != NULL);
  wp_pos->set_visible(wp_shift->GetDataField()->GetAsInteger() != 0);
}

void
MapDisplayConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpOrientationCruise"));
  assert(wp != NULL);
  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->EnableItemHelp(true);
  dfe->addEnumText(_("Track up"), TRACKUP,
                   _("The moving map display will be rotated so the glider's track is oriented up."));
  dfe->addEnumText(_("North up"), NORTHUP,
                   _("The moving map display will always be orientated north to south and the glider icon will be rotated to show its course."));
  dfe->addEnumText(_("Target up"), TARGETUP,
                   _("The moving map display will be rotated so the navigation target is oriented up."));
  dfe->Set(XCSoarInterface::SettingsMap().OrientationCruise);
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpOrientationCircling"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->EnableItemHelp(true);
  dfe->addEnumText(_("Track up"), TRACKUP,
                   _("The moving map display will be rotated so the glider's track is oriented up."));
  dfe->addEnumText(_("North up"), NORTHUP,
                   _("The moving map display will always be orientated north to south and the glider icon will be rotated to show its course."));
  dfe->addEnumText(_("Target up"), TARGETUP,
                   _("The moving map display will be rotated so the navigation target is oriented up."));
  dfe->Set(XCSoarInterface::SettingsMap().OrientationCircling);
  wp->RefreshDisplay();

  wp = (WndProperty*)wf->FindByName(_T("prpMapShiftBias"));
  assert(wp != NULL);
  dfe = (DataFieldEnum*)wp->GetDataField();
  dfe->EnableItemHelp(true);
  dfe->addEnumText(_("None"), MAP_SHIFT_BIAS_NONE, _("Disable adjustments."));
  dfe->addEnumText(_("Track"), MAP_SHIFT_BIAS_TRACK, _("Use a recent average of the ground track as basis."));
  dfe->addEnumText(_("Target"), MAP_SHIFT_BIAS_TARGET, _("Use the current target waypoint as basis."));
  dfe->Set(XCSoarInterface::SettingsMap().MapShiftBias);
  wp->RefreshDisplay();

  LoadFormProperty(*wf, _T("prpGliderScreenPosition"),
                   XCSoarInterface::SettingsMap().GliderScreenPosition);

  LoadFormProperty(*wf, _T("prpCirclingZoom"),
                   XCSoarInterface::SettingsMap().CircleZoom);

  LoadFormProperty(*wf, _T("prpMaxAutoZoomDistance"), ugDistance,
                   XCSoarInterface::SettingsMap().MaxAutoZoomDistance);

  UpdateVisibilities();
}

void
MapDisplayConfigPanel::OnShiftTypeData(DataField *Sender, DataField::DataAccessKind_t Mode)
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
MapDisplayConfigPanel::Save()
{
  bool changed = false;
  WndProperty *wp;

  wp = (WndProperty*)wf->FindByName(_T("prpOrientationCruise"));
  assert(wp != NULL);
  if (XCSoarInterface::SettingsMap().OrientationCruise != wp->GetDataField()->GetAsInteger()) {
    XCSoarInterface::SetSettingsMap().OrientationCruise = (DisplayOrientation_t)wp->GetDataField()->GetAsInteger();
    Profile::Set(szProfileOrientationCruise,
                 XCSoarInterface::SettingsMap().OrientationCruise);
    changed = true;
  }

  wp = (WndProperty*)wf->FindByName(_T("prpOrientationCircling"));
  assert(wp != NULL);
  if (XCSoarInterface::SettingsMap().OrientationCircling != wp->GetDataField()->GetAsInteger()) {
    XCSoarInterface::SetSettingsMap().OrientationCircling = (DisplayOrientation_t)wp->GetDataField()->GetAsInteger();
    Profile::Set(szProfileOrientationCircling,
                 XCSoarInterface::SettingsMap().OrientationCircling);
    changed = true;
  }

  wp = (WndProperty*)wf->FindByName(_T("prpMapShiftBias"));
  assert(wp != NULL);
  if (XCSoarInterface::SettingsMap().MapShiftBias != wp->GetDataField()->GetAsInteger()) {
    XCSoarInterface::SetSettingsMap().MapShiftBias = (MapShiftBias_t)wp->GetDataField()->GetAsInteger();
    Profile::Set(szProfileMapShiftBias,
                 XCSoarInterface::SettingsMap().MapShiftBias);
    changed = true;
  }

  changed |= SaveFormProperty(*wf, _T("prpGliderScreenPosition"),
                              szProfileGliderScreenPosition,
                              XCSoarInterface::SetSettingsMap().GliderScreenPosition);

  changed |= SaveFormProperty(*wf, _T("prpCirclingZoom"),
                              szProfileCircleZoom,
                              XCSoarInterface::SetSettingsMap().CircleZoom);

  changed |= SaveFormProperty(*wf, _T("prpMaxAutoZoomDistance"),
                              ugDistance,
                              XCSoarInterface::SetSettingsMap().MaxAutoZoomDistance,
                              szProfileMaxAutoZoomDistance);

  return changed;
}
