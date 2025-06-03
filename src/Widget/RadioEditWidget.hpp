// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"
#include "Form/Button.hpp"
#include "Form/DigitEntry.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "Form/DigitEntry.hpp"
#include "Form/DataField/Frequency.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Device/MultipleDevices.hpp"
#include "Units/Units.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/Edit.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

#include <array>
#include <memory>
#include <tchar.h>

class Button;

class RadioEditWidget : public NullWidget,
                               private DataFieldListener
{
  const DialogLook &look;
  std::unique_ptr<Button> list_button;
  std::unique_ptr<WndProperty> freq_edit;

public:
  RadioEditWidget(const DialogLook &_look) noexcept
      : look(_look){};

private:
  void OnModified(DataField &df) noexcept override;

public:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;

protected:
  virtual void OnFrequencyChanged(RadioFrequency new_freq) noexcept = 0;
  virtual void OnOpenList() noexcept = 0;
  virtual RadioFrequency GetCurrentFrequency() const noexcept = 0;
  void UpdateFrequencyField(RadioFrequency freq);
};
