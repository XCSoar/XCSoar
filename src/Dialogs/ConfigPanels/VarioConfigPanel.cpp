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
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Interface.hpp"
#include "Form/Form.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"

class VarioConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
VarioConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
VarioConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
VarioConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_VARIOCONFIGPANEL") :
                               _T("IDR_XML_VARIOCONFIGPANEL_L"));

  const VarioSettings &settings = CommonInterface::GetUISettings().vario;

  LoadFormProperty(form, _T("prpAppGaugeVarioSpeedToFly"),
                   settings.ShowSpeedToFly);

  LoadFormProperty(form, _T("prpAppGaugeVarioAvgText"),
                   settings.ShowAvgText);

  LoadFormProperty(form, _T("prpAppGaugeVarioMc"),
                   settings.ShowMc);

  LoadFormProperty(form, _T("prpAppGaugeVarioBugs"),
                   settings.ShowBugs);

  LoadFormProperty(form, _T("prpAppGaugeVarioBallast"),
                   settings.ShowBallast);

  LoadFormProperty(form, _T("prpAppGaugeVarioGross"),
                   settings.ShowGross);

  LoadFormProperty(form, _T("prpAppAveNeedle"),
                   settings.ShowAveNeedle);
}

bool
VarioConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  VarioSettings &settings = CommonInterface::SetUISettings().vario;

  changed |= SaveFormProperty(form, _T("prpAppGaugeVarioSpeedToFly"),
                              szProfileAppGaugeVarioSpeedToFly,
                              settings.ShowSpeedToFly);

  changed |= SaveFormProperty(form, _T("prpAppGaugeVarioAvgText"),
                              szProfileAppGaugeVarioAvgText,
                              settings.ShowAvgText);

  changed |= SaveFormProperty(form, _T("prpAppGaugeVarioMc"),
                              szProfileAppGaugeVarioMc,
                              settings.ShowMc);

  changed |= SaveFormProperty(form, _T("prpAppGaugeVarioBugs"),
                              szProfileAppGaugeVarioBugs,
                              settings.ShowBugs);

  changed |= SaveFormProperty(form, _T("prpAppGaugeVarioBallast"),
                              szProfileAppGaugeVarioBallast,
                              settings.ShowBallast);

  changed |= SaveFormProperty(form, _T("prpAppGaugeVarioGross"),
                              szProfileAppGaugeVarioGross,
                              settings.ShowGross);

  changed |= SaveFormProperty(form, _T("prpAppAveNeedle"), szProfileAppAveNeedle,
                              settings.ShowAveNeedle);

  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateVarioConfigPanel()
{
  return new VarioConfigPanel();
}
