// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ActionWidget.hpp"

bool
ActionWidget::Click() noexcept
{
  callback();
  return false;
}

void
ActionWidget::ReClick() noexcept
{
  callback();
}

#ifndef HAVE_CLIPPING
void
ActionWidget::Show([[maybe_unused]] const PixelRect &rc) noexcept
{
}

void
ActionWidget::Hide() noexcept
{
}
#endif
