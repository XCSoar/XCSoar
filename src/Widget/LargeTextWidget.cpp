// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LargeTextWidget.hpp"
#include "ui/control/LargeTextWindow.hpp"
#include "Look/DialogLook.hpp"

void
LargeTextWidget::SetText(const char *text) noexcept
{
  LargeTextWindow &w = (LargeTextWindow &)GetWindow();
  w.SetText(text);
}

void
LargeTextWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  LargeTextWindowStyle style;
  style.Hide();
  style.TabStop();

  auto w = std::make_unique<LargeTextWindow>();
  w->Create(parent, rc, style);
#ifndef USE_WINUSER
  w->SetFont(look.text_font);
#endif
  w->SetColors(look.ReadOnlyValueBackground(), look.list.text_color,
               look.ReadOnlyValueBorderColor());
  if (text != nullptr)
    w->SetText(text);

  SetWindow(std::move(w));
}

bool
LargeTextWidget::SetFocus() noexcept
{
  GetWindow().SetFocus();
  return true;
}
