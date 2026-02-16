// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AltitudeSimulator.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/OffsetButtonsWidget.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

class AltitudeSimulatorOffsetButtons final : public OffsetButtonsWidget {
public:
  using OffsetButtonsWidget::OffsetButtonsWidget;

protected:
  /* virtual methods from OffsetButtonsWidget */
  void OnOffset(double offset) noexcept override;
};

void
AltitudeSimulatorOffsetButtons::OnOffset(const double step) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.gps.simulator)
    return;

  backend_components->device_blackboard->SetAltitude(basic.gps_altitude +
                                                     Units::ToSysAltitude(step));
}

std::unique_ptr<Widget>
LoadAltitudeSimulatorPanel([[maybe_unused]] unsigned id)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.gps.simulator)
    return nullptr;

  return std::make_unique<AltitudeSimulatorOffsetButtons>(UIGlobals::GetDialogLook().button,
                                                          "%+.0f",
                                                          10, 100);
}
