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
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/XML.hpp"


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

class MapDisplayConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void UpdateVisibilities();
};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static MapDisplayConfigPanel *instance;

void
MapDisplayConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
MapDisplayConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
MapDisplayConfigPanel::UpdateVisibilities()
{
  bool northup = GetFormValueInteger(form, _T("prpOrientationCruise")) == NORTHUP;

  ShowFormControl(form, _T("prpMapShiftBias"), northup);
}

static void
OnShiftTypeData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange: {
    instance->UpdateVisibilities();
    break;
  }
  case DataField::daSpecial:
    return;
  }
}

gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnShiftTypeData),
  DeclareCallBackEntry(NULL)
};

void
MapDisplayConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;
  LoadWindow(CallBackTable, parent,
             Layout::landscape ? _T("IDR_XML_MAPDISPLAYCONFIGPANEL") :
                               _T("IDR_XML_MAPDISPLAYCONFIGPANEL_L"));


  const SETTINGS_MAP &settings_map = CommonInterface::SettingsMap();

  LoadFormProperty(form, _T("prpOrientationCruise"), orientation_list,
                   settings_map.cruise_orientation);
  LoadFormProperty(form, _T("prpOrientationCircling"), orientation_list,
                   settings_map.circling_orientation);
  LoadFormProperty(form, _T("prpMapShiftBias"), shift_bias_list,
                   settings_map.map_shift_bias);

  LoadFormProperty(form, _T("prpGliderScreenPosition"),
                   XCSoarInterface::SettingsMap().glider_screen_position);

  LoadFormProperty(form, _T("prpCirclingZoom"),
                   XCSoarInterface::SettingsMap().circle_zoom_enabled);

  LoadFormProperty(form, _T("prpMaxAutoZoomDistance"), ugDistance,
                   XCSoarInterface::SettingsMap().max_auto_zoom_distance);

  UpdateVisibilities();
}

bool
MapDisplayConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  SETTINGS_MAP &settings_map = CommonInterface::SetSettingsMap();

  changed |= SaveFormPropertyEnum(form, _T("prpOrientationCruise"),
                                  szProfileOrientationCruise,
                                  settings_map.cruise_orientation);

  changed |= SaveFormPropertyEnum(form, _T("prpOrientationCircling"),
                                  szProfileOrientationCircling,
                                  settings_map.circling_orientation);

  changed |= SaveFormPropertyEnum(form, _T("prpMapShiftBias"),
                                  szProfileMapShiftBias,
                                  settings_map.map_shift_bias);

  changed |= SaveFormProperty(form, _T("prpGliderScreenPosition"),
                              szProfileGliderScreenPosition,
                              XCSoarInterface::SetSettingsMap().glider_screen_position);

  changed |= SaveFormProperty(form, _T("prpCirclingZoom"),
                              szProfileCircleZoom,
                              XCSoarInterface::SetSettingsMap().circle_zoom_enabled);

  changed |= SaveFormProperty(form, _T("prpMaxAutoZoomDistance"),
                              ugDistance,
                              XCSoarInterface::SetSettingsMap().max_auto_zoom_distance,
                              szProfileMaxAutoZoomDistance);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateMapDisplayConfigPanel()
{
  return new MapDisplayConfigPanel();
}
