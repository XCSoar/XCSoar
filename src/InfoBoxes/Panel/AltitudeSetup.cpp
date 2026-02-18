// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AltitudeSetup.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Device/MultipleDevices.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Integer.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/Edit.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Math/Util.hpp"
#include "thread/Mutex.hxx"

class AltitudeSetupPanel : public RowFormWidget,
                           private DataFieldListener {
  WndProperty *qnh_control = nullptr;
  WndProperty *elevation_control = nullptr;

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
  ComputerSettings &settings =
    CommonInterface::SetComputerSettings();

  DataField *qnh_df = qnh_control ? qnh_control->GetDataField() : nullptr;
  if (qnh_df && &_df == qnh_df) {
    DataFieldFloat &df = (DataFieldFloat &)_df;
    settings.pressure = Units::FromUserPressure(df.GetValue());
    settings.pressure_available.Update(CommonInterface::Basic().clock);

    if (backend_components && backend_components->devices) {
      MessageOperationEnvironment env;
      backend_components->devices->PutQNH(settings.pressure, env);
    }
    return;
  }

  DataField *elevation_df = elevation_control ? elevation_control->GetDataField() : nullptr;
  if (elevation_df && &_df == elevation_df) {
    DataFieldInteger &df = (DataFieldInteger &)_df;
    int elevation = df.GetValue();

    if (backend_components && backend_components->devices) {
      MessageOperationEnvironment env;
      backend_components->devices->PutElevation(elevation, env);
    }
  }
}

void
AltitudeSetupPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
{
  const ComputerSettings &settings =
    CommonInterface::GetComputerSettings();

  qnh_control = AddFloat(_("QNH"),
                         _("Area pressure for barometric altimeter calibration.  This is set automatically if Vega connected."),
                         GetUserPressureFormat(), GetUserPressureFormat(),
                         Units::ToUserPressure(Units::ToSysUnit(850, Unit::HECTOPASCAL)),
                         Units::ToUserPressure(Units::ToSysUnit(1300, Unit::HECTOPASCAL)),
                         GetUserPressureStep(), false,
                         Units::ToUserPressure(settings.pressure), this);
  {
    DataFieldFloat &df = *(DataFieldFloat *)qnh_control->GetDataField();
    df.SetUnits(Units::GetPressureName());
    qnh_control->RefreshDisplay();
  }

  int elevation = 0;
  bool elevation_available = false;

  /* Request elevation from devices and check if already available */
  if (backend_components && backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->RequestElevation(env);

    if (backend_components->device_blackboard) {
      auto &device_blackboard = *backend_components->device_blackboard;
      const std::lock_guard lock{device_blackboard.mutex};
      const NMEAInfo &device_basic = device_blackboard.Basic();

      if (device_basic.settings.elevation_available.IsValid()) {
        elevation = device_basic.settings.elevation;
        elevation_available = true;
      }
    }
  }

  /* If not received from device, read from CommonInterface (may have been set earlier) */
  if (!elevation_available) {
    const NMEAInfo &basic = CommonInterface::Basic();
    if (basic.settings.elevation_available.IsValid()) {
      elevation = basic.settings.elevation;
      elevation_available = true;
    }
  }

  /* Always show elevation control to display device state */
  if (elevation_available) {
    elevation_control = AddInteger(_("Elevation"), _("Ground elevation for device calibration (meters)."),
                                    "%d m", "%d", -500, 9000, 1, elevation, this);
  } else {
    AddReadOnly(_("Elevation"), _("Ground elevation for device calibration (meters)."),
                "N/A");
  }
}

std::unique_ptr<Widget>
LoadAltitudeSetupPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<AltitudeSetupPanel>();
}
