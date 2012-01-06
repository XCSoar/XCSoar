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

#include "VarioConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Form/Form.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  AppGaugeVarioSpeedToFly,
  AppGaugeVarioAvgText,
  AppGaugeVarioMc,
  AppGaugeVarioBugs,
  AppGaugeVarioBallast,
  AppGaugeVarioGross,
  AppAveNeedle
};


class VarioConfigPanel : public RowFormWidget {
public:
  VarioConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

void
VarioConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const VarioSettings &settings = CommonInterface::GetUISettings().vario;

  RowFormWidget::Prepare(parent, rc);

  // Expert item (TODO)
  AddBoolean(_("Speed arrows"),
             _("Whether to show speed command arrows on the vario gauge.  When shown, in cruise mode, "
                 "arrows point up to command slow down; arrows point down to command speed up."),
             settings.ShowSpeedToFly);

  // Expert item
  AddBoolean(_("Show average"),
             _("Whether to show the average climb rate.  In cruise mode, this switches to showing the "
                 "average netto airmass rate."),
             settings.ShowAvgText);

  // Expert item
  AddBoolean(_("Show MacReady"), _("Whether to show the MacCready setting."), settings.ShowMc);

  // Expert item
  AddBoolean(_("Show bugs"), _("Whether to show the bugs percentage."), settings.ShowBugs);

  // Expert item
  AddBoolean(_("Show ballast"), _("Whether to show the ballast percentage."), settings.ShowBallast);

  // Expert item
  AddBoolean(_("Show gross"), _("Whether to show the gross climb rate."), settings.ShowGross);

  // Expert item
  AddBoolean(_("Averager needle"),
             _("If true, the vario gauge will display a hollow averager needle.  During cruise, this "
                 "needle displays the average netto value.  During circling, this needle displays the "
                 "average gross value."),
             settings.ShowAveNeedle);
}

bool
VarioConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  VarioSettings &settings = CommonInterface::SetUISettings().vario;

  changed |= SaveValue(AppGaugeVarioSpeedToFly, szProfileAppGaugeVarioSpeedToFly, settings.ShowSpeedToFly);

  changed |= SaveValue(AppGaugeVarioAvgText, szProfileAppGaugeVarioAvgText, settings.ShowAvgText);

  changed |= SaveValue(AppGaugeVarioMc, szProfileAppGaugeVarioMc, settings.ShowMc);

  changed |= SaveValue(AppGaugeVarioBugs, szProfileAppGaugeVarioBugs, settings.ShowBugs);

  changed |= SaveValue(AppGaugeVarioBallast, szProfileAppGaugeVarioBallast, settings.ShowBallast);

  changed |= SaveValue(AppGaugeVarioGross, szProfileAppGaugeVarioGross, settings.ShowGross);

  changed |= SaveValue(AppAveNeedle, szProfileAppAveNeedle, settings.ShowAveNeedle);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateVarioConfigPanel()
{
  return new VarioConfigPanel();
}
