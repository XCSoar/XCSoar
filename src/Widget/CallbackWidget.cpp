// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CallbackWidget.hpp"

bool
CallbackWidget::Click() noexcept
{
  callback();
  return false;
}

void
CallbackWidget::ReClick() noexcept
{
  callback();
}

#ifndef HAVE_CLIPPING
void
CallbackWidget::Show([[maybe_unused]] const PixelRect &rc) noexcept
{
}

void
CallbackWidget::Hide() noexcept
{
}
#endif
