// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AltitudeSetup.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Device/MultipleDevices.hpp"
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

  settings.pressure = Units::FromUserPressure(df.GetValue());
  settings.pressure_available.Update(CommonInterface::Basic().clock);

  if (backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->PutQNH(settings.pressure, env);
  }
}

void
AltitudeSetupPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
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
LoadAltitudeSetupPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<AltitudeSetupPanel>();
}
