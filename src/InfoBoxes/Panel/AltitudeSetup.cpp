/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "AltitudeSetup.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/Edit.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

class AltitudeSetupPanel : public RowFormWidget,
                           private DataFieldListener {
public:
  AltitudeSetupPanel():RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

private:
  void OnModified(DataField &df) noexcept override;
};

void
AltitudeSetupPanel::OnModified(DataField &_df) noexcept
{
  DataFieldFloat &df = (DataFieldFloat &)_df;
  ComputerSettings &settings =
    CommonInterface::SetComputerSettings();

  settings.pressure = Units::FromUserPressure(df.GetAsFixed());
  settings.pressure_available.Update(CommonInterface::Basic().clock);

  {
    MessageOperationEnvironment env;
    device_blackboard->SetQNH(settings.pressure, env);
  }
}

void
AltitudeSetupPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const ComputerSettings &settings =
    CommonInterface::GetComputerSettings();

  WndProperty *wp;
  wp = AddFloat(_("QNH"),
                _("Area pressure for barometric altimeter calibration.  This is set automatically if Vega connected."),
                GetUserPressureFormat(), GetUserPressureFormat(),
                Units::ToUserPressure(Units::ToSysUnit(850, Unit::HECTOPASCAL)),
                Units::ToUserPressure(Units::ToSysUnit(1300, Unit::HECTOPASCAL)),
                GetUserPressureStep(), false,
                Units::ToUserPressure(settings.pressure), this);
  {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetUnits(Units::GetPressureName());
    wp->RefreshDisplay();
  }
}

std::unique_ptr<Widget>
LoadAltitudeSetupPanel(unsigned id)
{
  return std::make_unique<AltitudeSetupPanel>();
}
