// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StatusPanel.hpp"

void
StatusPanel::Show(const PixelRect &rc) noexcept
{
  Refresh();
  RowFormWidget::Show(rc);
}
