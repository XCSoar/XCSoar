// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScrollBarConfig.hpp"
#include "UISettings.hpp"
#include "ui/control/ScrollBar.hpp"
#include "Asset.hpp"

[[gnu::pure]]
static ScrollBar::Style
ResolveScrollBarStyle(UISettings::ScrollBars setting) noexcept
{
  switch (setting) {
  case UISettings::ScrollBars::WHEN_SCROLLING:
    return ScrollBar::Style::OVERLAY;

  case UISettings::ScrollBars::SLIM:
    return ScrollBar::Style::SLIM;

  case UISettings::ScrollBars::STANDARD:
    return ScrollBar::Style::STANDARD;

  case UISettings::ScrollBars::AUTO:
    break;
  }

  /* a bar that appears and disappears costs an e-paper screen a
     refresh each time and leaves ghosts behind; a permanent one is
     drawn once and then only moves */
  if (HasEPaper())
    return ScrollBar::Style::STANDARD;

  /* on a touch screen, the content itself is dragged and a scroll bar
     would only take away space from it */
  if (HasTouchScreen())
    return ScrollBar::Style::OVERLAY;

  /* without a pointer, the arrow buttons cannot be pressed at all;
     a slim bar shows the position and wastes no width on them */
  if (!HasPointer())
    return ScrollBar::Style::SLIM;

  return ScrollBar::Style::STANDARD;
}

void
ApplyScrollBars(const UISettings &settings) noexcept
{
  ScrollBar::SetGlobalStyle(ResolveScrollBarStyle(settings.scroll_bars));
}
