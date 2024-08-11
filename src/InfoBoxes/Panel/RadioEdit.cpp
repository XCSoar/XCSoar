// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadioEdit.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/RadioEditWidget.hpp"
#include "Formatter/UserUnits.hpp"
#include "ActionInterface.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/Frequency/dlgUserFrequencyList.hpp"

class RadioEdit final : public RadioEditWidget
{
public:
  explicit RadioEdit(bool active_freq) noexcept
      : RadioEditWidget(UIGlobals::GetDialogLook()), set_active_freq(active_freq) {}

protected:
  /* virtual methods from OffsetButtonsWidget */
  void OnFrequencyChanged(RadioFrequency new_freq) noexcept override;
  void OnOpenList() noexcept override;
  RadioFrequency GetCurrentFrequency() const noexcept override;

private:
  const bool set_active_freq;
};

std::unique_ptr<Widget>
LoadActiveRadioFrequencyEditPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<RadioEdit>(true);
}

std::unique_ptr<Widget>
LoadStandbyRadioFrequencyEditPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<RadioEdit>(false);
}

void RadioEdit::OnFrequencyChanged(RadioFrequency new_freq) noexcept
{
  if (set_active_freq)
    ActionInterface::SetActiveFrequency(new_freq, _T(""));
  else
    ActionInterface::SetStandbyFrequency(new_freq, _T(""));
}

RadioFrequency RadioEdit::GetCurrentFrequency() const noexcept
{
  ComputerSettings &settings =
      CommonInterface::SetComputerSettings();
  if (set_active_freq)
    return settings.radio.active_frequency;
  else
    return settings.radio.standby_frequency;
}

void RadioEdit::OnOpenList() noexcept
{
  auto mode = UserFrequencyListWidget::DialogMode::SELECT_ACTIVE;
  if (!set_active_freq)
    mode = UserFrequencyListWidget::DialogMode::SELECT_STANDBY;
  dlgUserFrequencyListWidgetShowModal(mode);

  UpdateFrequencyField(GetCurrentFrequency());
}
