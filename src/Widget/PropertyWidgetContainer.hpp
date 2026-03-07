// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"
#include "Form/Edit.hpp"
#include "ui/dim/Rect.hpp"

#include <memory>

/**
 * A Widget that composes a read-only WndProperty label at the top and
 * a content Widget below it, forwarding all lifecycle methods to the
 * content Widget.  Subclasses provide the content Widget via
 * GetContentWidget() and may override CalculateLayout() to reserve
 * extra space (e.g. for a bottom checkbox).
 */
class PropertyWidgetContainer : public NullWidget {
  const char *property_label;

protected:
  std::unique_ptr<WndProperty> property;
  PixelRect property_rect{};
  PixelRect content_rect{};

  explicit PropertyWidgetContainer(const char *_label) noexcept
    : property_label(_label) {}

public:
  ~PropertyWidgetContainer() noexcept override;

  virtual Widget &GetContentWidget() noexcept = 0;
  virtual const Widget &GetContentWidget() const noexcept = 0;

  virtual void CalculateLayout(const PixelRect &rc) noexcept;

  void UpdatePropertyText(const char *text) noexcept;

public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
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
