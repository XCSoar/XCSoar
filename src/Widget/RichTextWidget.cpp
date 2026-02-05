// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RichTextWidget.hpp"
#include "ui/control/RichTextWindow.hpp"
#include "Look/DialogLook.hpp"

void
RichTextWidget::SetText(const TCHAR *_text) noexcept
{
  text = _text;
  if (IsDefined()) {
    RichTextWindow &w = static_cast<RichTextWindow &>(GetWindow());
    w.SetText(text);
  }
}

void
RichTextWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  auto w = std::make_unique<RichTextWindow>();
  w->Create(parent, rc, style);
  w->SetFont(look.text_font);
  if (text != nullptr)
    w->SetText(text);

  SetWindow(std::move(w));
}

bool
RichTextWidget::SetFocus() noexcept
{
  GetWindow().SetFocus();
  return true;
}
