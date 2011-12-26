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

#include "GaugesConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Interface.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  EnableFLARMGauge,
  AutoCloseFlarmDialog,
  EnableTAGauge,
  EnableThermalProfile
};

class GaugesConfigPanel : public RowFormWidget {

public:
  GaugesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

void
GaugesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("FLARM radar"),
             _("This enables the display of the FLARM radar gauge. The track bearing of the target relative to the track bearing of the aircraft is displayed as an arrow head, and a triangle pointing up or down shows the relative altitude of the target relative to you. In all modes, the color of the target indicates the threat level."),
             ui_settings.enable_flarm_gauge);

  AddBoolean(_("Auto close FLARM"),
             _("Setting this to \"On\" will automatically close the FLARM dialog if there is no traffic. \"Off\" will keep the dialog open even without current traffic."),
             ui_settings.auto_close_flarm_dialog);

  AddBoolean(_("Thermal assistant"),
             _("This enables the display of the thermal assistant gauge."),
             ui_settings.enable_thermal_assistant_gauge);

  AddBoolean(_("Thermal band"),
             _("This enables the display of the thermal profile (climb band) display on the map."),
             XCSoarInterface::GetMapSettings().show_thermal_profile);
}

bool
GaugesConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  changed |= SaveValue(EnableFLARMGauge, szProfileEnableFLARMGauge,
                       ui_settings.enable_flarm_gauge);

  changed |= SaveValue(AutoCloseFlarmDialog, szProfileAutoCloseFlarmDialog,
                       ui_settings.auto_close_flarm_dialog);

  changed |= SaveValue(EnableTAGauge, szProfileEnableTAGauge,
                       ui_settings.enable_thermal_assistant_gauge);

  changed |= SaveValue(EnableThermalProfile, szProfileEnableThermalProfile,
                       XCSoarInterface::SetMapSettings().show_thermal_profile);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateGaugesConfigPanel()
{
  return new GaugesConfigPanel();
}
