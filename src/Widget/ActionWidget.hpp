// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"

#include <functional>

/**
 * A #Widget implementation that calls a function when clicked.
 *
 * A #NullWidget avoids creating a panel window. The parent already
 * paints the dialog background.
 */
class ActionWidget : public NullWidget
{
  const std::function<void()> callback;

public:
  explicit ActionWidget(std::function<void()> _callback) noexcept
    :callback(std::move(_callback)) {}

public:
  bool Click() noexcept override;
  void ReClick() noexcept override;

  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
};
