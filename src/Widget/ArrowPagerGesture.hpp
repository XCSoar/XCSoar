// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ArrowPagerWidget.hpp"

#include <functional>

/**
 * Build the standard horizontal-swipe handler for
 * #VScrollWidget::SetGestureCallback (swipe R/L → next/previous,
 * respecting #ArrowPagerWidget::SetCanAdvanceCallback when set).
 *
 * #ArrowPagerWidget::Prepare() applies this to each page that is a
 * #VScrollWidget or #QuickGuidePageWidget; callers normally do not
 * need to invoke this.
 */
[[nodiscard]] inline std::function<void(bool)>
MakeArrowPagerSwipeCallback(ArrowPagerWidget *pager)
{
  return [pager](bool next) noexcept {
    if (pager == nullptr)
      return;
    if (next) {
      if (pager->CanAdvance())
        pager->Next(true);
    } else {
      pager->Previous(true);
    }
  };
}
