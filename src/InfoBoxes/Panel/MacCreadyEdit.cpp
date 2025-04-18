// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MacCreadyEdit.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/OffsetButtonsWidget.hpp"
#include "Formatter/UserUnits.hpp"
#include "Units/Units.hpp"
#include "ActionInterface.hpp"
#include "UIGlobals.hpp"

class MacCreadyOffsetButtons final : public OffsetButtonsWidget {
public:
  using OffsetButtonsWidget::OffsetButtonsWidget;

protected:
  /* virtual methods from OffsetButtonsWidget */
  void OnOffset(double offset) noexcept override;
};

void
MacCreadyOffsetButtons::OnOffset(double offset) noexcept
{
  ActionInterface::OffsetManualMacCready(Units::ToSysVSpeed(offset));
}

std::unique_ptr<Widget>
LoadMacCreadyEditPanel([[maybe_unused]] unsigned id)
{
  const auto step = GetUserVerticalSpeedStep();
  return std::make_unique<MacCreadyOffsetButtons>(UIGlobals::GetDialogLook().button,
                                                  GetUserVerticalSpeedFormat(false, true),
                                                  step, 5 * step);
}
