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
#include "Form/Util.hpp"
#include "Interface.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/XML.hpp"

class GaugesConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
GaugesConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
GaugesConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
GaugesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_GAUGESCONFIGPANEL") :
                               _T("IDR_XML_GAUGESCONFIGPANEL_L"));

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  LoadFormProperty(form, _T("prpEnableFLARMGauge"),
                   ui_settings.enable_flarm_gauge);

  LoadFormProperty(form, _T("prpAutoCloseFlarmDialog"),
                   ui_settings.auto_close_flarm_dialog);

  LoadFormProperty(form, _T("prpEnableTAGauge"),
                   ui_settings.enable_thermal_assistant_gauge);

  LoadFormProperty(form, _T("prpEnableThermalProfile"),
                   XCSoarInterface::SettingsMap().show_thermal_profile);
}

bool
GaugesConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  UISettings &ui_settings = CommonInterface::SetUISettings();

  changed |= SaveFormProperty(form, _T("prpEnableFLARMGauge"),
                              szProfileEnableFLARMGauge,
                              ui_settings.enable_flarm_gauge);

  changed |= SaveFormProperty(form, _T("prpAutoCloseFlarmDialog"),
                              szProfileAutoCloseFlarmDialog,
                              ui_settings.auto_close_flarm_dialog);

  changed |= SaveFormProperty(form, _T("prpEnableTAGauge"),
                              szProfileEnableTAGauge,
                              ui_settings.enable_thermal_assistant_gauge);

  changed |= SaveFormProperty(form, _T("prpEnableThermalProfile"),
                              szProfileEnableThermalProfile,
                              XCSoarInterface::SetSettingsMap().show_thermal_profile);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateGaugesConfigPanel()
{
  return new GaugesConfigPanel();
}
