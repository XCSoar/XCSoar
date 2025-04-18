// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "Form/VScrollPanel.hpp"

struct DialogLook;

/**
 * A #Widget implementation which shows a vertical scroll bar if the
 * hosted #Widget is larger than the available space.
 */
class VScrollWidget final : public WindowWidget, VScrollPanelListener {
  const DialogLook &look;

  std::unique_ptr<Widget> widget;

  bool visible = false;

public:
  explicit VScrollWidget(std::unique_ptr<Widget> &&_widget,
                         const DialogLook &_look) noexcept
    :look(_look),
     widget(std::move(_widget)) {}

  Widget &GetWidget() noexcept {
    return *widget;
  }

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Leave() noexcept override;
  void Hide() noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

private:
  VScrollPanel &GetWindow() noexcept {
    return (VScrollPanel &)WindowWidget::GetWindow();
  }

  [[gnu::pure]]
  unsigned CalcVirtualHeight(const PixelRect &rc) const noexcept;

  void UpdateVirtualHeight(const PixelRect &rc) noexcept;

  /* virtual methods from class VScrollPanelListener */
  void OnVScrollPanelChange() noexcept override;
};
