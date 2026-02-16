// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadioEdit.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/OffsetButtonsWidget.hpp"
#include "Formatter/UserUnits.hpp"
#include "ActionInterface.hpp"
#include "UIGlobals.hpp"

class RadioOffsetButtons final : public OffsetButtonsWidget {
public:
  RadioOffsetButtons(bool active_freq) noexcept
    :OffsetButtonsWidget(UIGlobals::GetDialogLook().button, "%.0f kHz", 5, 1000),set_active_freq(active_freq) {}

protected:
  /* virtual methods from OffsetButtonsWidget */
  void OnOffset(double offset) noexcept override;

private:
  bool set_active_freq;
};

void
RadioOffsetButtons::OnOffset(double offset) noexcept
{
  if(set_active_freq) {
    ActionInterface::OffsetActiveFrequency(offset, true);
  } else {
    ActionInterface::OffsetStandbyFrequency(offset, true);
  }

}

std::unique_ptr<Widget>
LoadActiveRadioFrequencyEditPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<RadioOffsetButtons>(true);
}

std::unique_ptr<Widget>
LoadStandbyRadioFrequencyEditPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<RadioOffsetButtons>(false);
}
