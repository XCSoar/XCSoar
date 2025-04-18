// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Features.hpp"

#ifdef HAVE_CLIPPING
#include "PanelWidget.hpp"
#else
#include "Widget.hpp"
#endif

#include <functional>

/**
 * A #Widget implementation that calls a function when clicked.
 */
class ActionWidget
#ifdef HAVE_CLIPPING
/* need PanelWidget on GDI so dialog background gets rendered in the
   Widget area just in case this Widget becomes "visible", to avoid
   uninitialised screen area */
  : public PanelWidget
#else
/* on OpenGL, we can avoid the overhead of creating a panel window */
  : public NullWidget
#endif
{
  const std::function<void()> callback;

public:
  explicit ActionWidget(std::function<void()> _callback) noexcept
    :callback(std::move(_callback)) {}

public:
  bool Click() noexcept override;
  void ReClick() noexcept override;

#ifndef HAVE_CLIPPING
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
#endif
};
