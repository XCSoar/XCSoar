// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"

/**
 * A #Widget implementation that invokes a callback when clicked.  It
 * has no visual representation.
 *
 * A #NullWidget avoids creating a panel window. The parent already
 * paints the dialog background.
 */
class CallbackWidget : public NullWidget
{
  void (*const callback)();

public:
  CallbackWidget(void (*_callback)()) noexcept
    :callback(_callback) {}

public:
  bool Click() noexcept override;
  void ReClick() noexcept override;

  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
};
