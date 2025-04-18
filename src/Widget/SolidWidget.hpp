// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <memory>

/**
 * A #Widget wrapper that inserts a panel into the "parent chain" that
 * draws a form background using #SolidContainerWindow.
 */
class SolidWidget : public WindowWidget {
  std::unique_ptr<Widget> widget;

public:
  explicit SolidWidget(std::unique_ptr<Widget> &&_widget) noexcept
    :widget(std::move(_widget)) {}

  ~SolidWidget() noexcept override;

  Widget &GetWidget() noexcept {
    return *widget;
  }

  /* virtual methods from class Widget */
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
