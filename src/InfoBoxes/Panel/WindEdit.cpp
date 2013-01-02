/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "WindEdit.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Float.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Units/Group.hpp"
#include "Form/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"

enum ControlIndex {
  WindSpeed,
  WindDirection,
  LastItemInList,
};

class WindEditPanel: public RowFormWidget, DataFieldListener {
public:
  WindEditPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);

protected:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df);

private:
  void OnWindSpeed(DataFieldFloat &Sender);
  void OnWindDirection(DataFieldFloat &Sender);
};

void
WindEditPanel::OnModified(DataField &df)
{
  if (IsDataField(WindSpeed, df))
    OnWindSpeed((DataFieldFloat&)df);

  else if (IsDataField(WindDirection, df))
    OnWindDirection((DataFieldFloat&)df);
}

void
WindEditPanel::OnWindSpeed(DataFieldFloat &Sender)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  const bool external_wind = basic.external_wind_available &&
    settings.use_external_wind;

  if (!external_wind) {
    settings.manual_wind.norm = Units::ToSysWindSpeed(Sender.GetAsFixed());
    settings.manual_wind_available.Update(basic.clock);
  }
}

void
WindEditPanel::OnWindDirection(DataFieldFloat &Sender)
{
  const NMEAInfo &basic = XCSoarInterface::Basic();
  WindSettings &settings = CommonInterface::SetComputerSettings().wind;
  const bool external_wind = basic.external_wind_available &&
    settings.use_external_wind;

  if (!external_wind) {
    settings.manual_wind.bearing = Angle::Degrees(Sender.GetAsFixed());
    settings.manual_wind_available.Update(basic.clock);
  }
}

void
WindEditPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  AddFloat(_("Speed"), _("Manual adjustment of wind speed."), _T("%.0f"),
           _T("%.0f"), fixed_zero, fixed(130), fixed(1), false,
           UnitGroup::WIND_SPEED,
           CommonInterface::Calculated().GetWindOrZero().norm, this);

  AddFloat(_("Direction"), _("Manual adjustment of wind direction."),
           _T("%.0f°"), _T("%.0f"), fixed_zero, fixed(355), fixed(5), false, // TO DO Add degrees to first display format
           CommonInterface::Calculated().GetWindOrZero().bearing.Degrees(),
           this);
}

Widget *
LoadWindEditPanel(unsigned id)
{
  return new WindEditPanel();
}
