// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "ButtonLabel.hpp"
#include "MenuBar.hpp"
#include "MenuData.hpp"

namespace MenuGlue {

void
SetLabelText(MenuBar &bar, unsigned index,
             const char *text, unsigned event) noexcept
{
  char buffer[100];
  const auto expanded = ButtonLabel::Expand(text, std::span{buffer});
  if (expanded.visible)
    bar.ShowButton(index, expanded.enabled, expanded.text, event);
  else
    bar.HideButton(index);
}

void
Set(MenuBar &bar, const Menu &menu, const Menu *overlay, bool full) noexcept
{
  for (unsigned i = 0; i < menu.MAX_ITEMS; ++i) {
    const MenuItem &item = overlay != nullptr && (*overlay)[i].IsDefined()
      ? (*overlay)[i]
      : menu[i];

    if (full || item.IsDynamic())
      SetLabelText(bar, i, item.label, item.event);
  }
}

} // namespace MenuGlue
