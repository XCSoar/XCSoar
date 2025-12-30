// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/Widget.hpp"
#include "Form/CheckBox.hpp"
#include "Widget/LargeTextWidget.hpp"

class DontShowAgainWidget : public NullWidget {
  CheckBoxControl checkbox;
  LargeTextWidget info_text;
  const DialogLook &look;

public:
  explicit DontShowAgainWidget(const DialogLook &_look);

  PixelRect MakeCheckboxRect(const PixelRect &rc) const noexcept;
  PixelRect MakeTextRect(const PixelRect &rc, const PixelRect &cb_rect) const noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
};
