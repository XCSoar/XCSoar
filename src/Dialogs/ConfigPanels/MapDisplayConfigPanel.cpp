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

#include "MapDisplayConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static const StaticEnumChoice orientation_list[] = {
  { TRACKUP, N_("Track up"),
    N_("The moving map display will be rotated so the glider's track is oriented up.") },
  { NORTHUP, N_("North up"),
    N_("The moving map display will always be orientated north to south and the glider icon will be rotated to show its course.") },
  { TARGETUP, N_("Target up"),
    N_("The moving map display will be rotated so the navigation target is oriented up.") },
  { 0 }
};

static const StaticEnumChoice shift_bias_list[] = {
  { MAP_SHIFT_BIAS_NONE, N_("None"), N_("Disable adjustments.") },
  { MAP_SHIFT_BIAS_TRACK, N_("Track"),
    N_("Use a recent average of the ground track as basis.") },
  { MAP_SHIFT_BIAS_TARGET, N_("Target"),
    N_("Use the current target waypoint as basis.") },
  { 0 }
};

static WndForm* wf = NULL;

void
MapDisplayConfigPanel::UpdateVisibilities()
{
  bool northup = GetFormValueInteger(*wf, _T("prpOrientationCruise")) == NORTHUP;

  ShowFormControl(*wf, _T("prpMapShiftBias"), northup);
}

void
MapDisplayConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const SETTINGS_MAP &settings_map = CommonInterface::SettingsMap();

  LoadFormProperty(*wf, _T("prpOrientationCruise"), orientation_list,
                   settings_map.OrientationCruise);
  LoadFormProperty(*wf, _T("prpOrientationCircling"), orientation_list,
                   settings_map.OrientationCircling);
  LoadFormProperty(*wf, _T("prpMapShiftBias"), shift_bias_list,
                   settings_map.MapShiftBias);

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
  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();
  bool changed = false;

  changed |= SaveFormPropertyEnum(*wf, _T("prpOrientationCruise"),
                                  szProfileOrientationCruise,
                                  settings_map.OrientationCruise);

  changed |= SaveFormPropertyEnum(*wf, _T("prpOrientationCircling"),
                                  szProfileOrientationCircling,
                                  settings_map.OrientationCircling);

  changed |= SaveFormPropertyEnum(*wf, _T("prpMapShiftBias"),
                                  szProfileMapShiftBias,
                                  settings_map.MapShiftBias);

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
