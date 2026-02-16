// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"
#include "Form/Button.hpp"

#include <array>
#include <memory>

#include <array>

#include <tchar.h>

struct ButtonLook;
class Button;

/**
 * Show four buttons to increment/decrement a value.
 */
class OffsetButtonsWidget : public NullWidget {
  const ButtonLook &look;
  const char *const format;
  const double offsets[4];
  std::unique_ptr<std::array<Button, 4>> buttons;

public:
  OffsetButtonsWidget(const ButtonLook &_look, const char *_format,
                      double small_offset, double large_offset) noexcept
    :look(_look), format(_format),
     offsets{-large_offset, -small_offset, small_offset, large_offset} {}

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
  virtual void OnOffset(double offset) noexcept = 0;

private:
  Button MakeButton(ContainerWindow &parent, const PixelRect &r,
                    unsigned i) noexcept;
  std::array<Button, 4> MakeButtons(ContainerWindow &parent,
                                    const PixelRect &r) noexcept;
};
