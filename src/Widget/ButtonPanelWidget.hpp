// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"

#include <cassert>
#include <memory>

class ButtonPanel;

/**
 * Wrap a #ButtonPanel around another #Widget instance.
 */
class ButtonPanelWidget : public Widget {
public:
  enum class Alignment {
    AUTO, BOTTOM, LEFT
  };

private:
  const std::unique_ptr<Widget> widget;
  std::unique_ptr<ButtonPanel> buttons;
  Alignment alignment;

public:
  ButtonPanelWidget(std::unique_ptr<Widget> &&_widget,
                    Alignment _alignment=Alignment::AUTO) noexcept
    :widget(std::move(_widget)), alignment(_alignment) {}

  ~ButtonPanelWidget() noexcept override;

  Widget &GetWidget() noexcept {
    return *widget;
  }

  ButtonPanel &GetButtonPanel() noexcept {
    assert(buttons != nullptr);

    return *buttons;
  }

private:
  PixelRect UpdateLayout(const PixelRect &rc) noexcept;

public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool Save(bool &changed) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Leave() noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};
