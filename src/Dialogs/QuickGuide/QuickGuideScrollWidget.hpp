// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Form/VScrollPanel.hpp"

struct DialogLook;

/**
 * A Quick Guide specific widget wrapper that provides a VScrollPanel
 * and reserves room for the vertical scrollbar.
 */
class QuickGuideScrollWidget final : public WindowWidget, VScrollPanelListener {
  const DialogLook &look;
  std::unique_ptr<Widget> widget;
  bool visible = false;

public:
  explicit QuickGuideScrollWidget(std::unique_ptr<Widget> &&_widget,
                                  const DialogLook &_look) noexcept;

  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;

private:
  VScrollPanel &GetWindow() noexcept {
    return static_cast<VScrollPanel &>(WindowWidget::GetWindow());
  }

  [[gnu::pure]]
  static unsigned GetScrollbarWidth() noexcept;

  [[gnu::pure]]
  PixelRect ReserveScrollbar(PixelRect rc) const noexcept;

  [[gnu::pure]]
  unsigned CalcVirtualHeight(const PixelRect &rc) const noexcept;

  void UpdateVirtualHeight(const PixelRect &rc) noexcept;

  void OnVScrollPanelChange() noexcept override;
};
