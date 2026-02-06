// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RichTextWidget.hpp"
#include "RichTextWindowWithInternalLinks.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

PixelSize
RichTextWidget::GetMinimumSize() const noexcept
{
  return PixelSize{Layout::FastScale(200), Layout::FastScale(100)};
}

PixelSize
RichTextWidget::GetMaximumSize() const noexcept
{
  // If window is prepared, use actual content height
  if (IsDefined()) {
    const auto &w = static_cast<const RichTextWindow &>(GetWindow());
    unsigned content_height = w.GetContentHeight();
    if (content_height > 0)
      return PixelSize{Layout::FastScale(600),
                       static_cast<int>(content_height)};
  }

  // Estimate height from text length before window is prepared
  if (!text.empty()) {
    const Font &font = GetLook().text_font;
    const unsigned line_height = font.GetLineSpacing();

    // Count newlines for initial estimate
    unsigned line_count = 1;
    for (const char c : text) {
      if (c == '\n')
        ++line_count;
    }

    // Add extra for word wrapping (rough estimate: 50% more lines)
    line_count = line_count * 3 / 2;

    const unsigned padding = Layout::GetTextPadding() * 2;
    const int estimated_height =
      static_cast<int>(line_count * line_height + padding * 2);

    return PixelSize{Layout::FastScale(600), estimated_height};
  }

  return PixelSize{Layout::FastScale(600), Layout::FastScale(200)};
}

void
RichTextWidget::SetText(const char *_text) noexcept
{
  text = _text != nullptr ? _text : "";
  if (IsDefined()) {
    auto &w = static_cast<RichTextWindow &>(GetWindow());
    w.SetText(text.c_str(), parse_links);
  }
}

const DialogLook &
RichTextWidget::GetLook() const noexcept
{
  if (look != nullptr)
    return *look;
  return UIGlobals::GetDialogLook();
}

void
RichTextWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  auto w = std::make_unique<RichTextWindowWithInternalLinks>();
  w->Create(parent, rc, style);

  const auto &dialog_look = GetLook();
  w->SetFont(dialog_look.text_font, &dialog_look.bold_font, &dialog_look.bold_font);

  if (!text.empty())
    w->SetText(text.c_str(), parse_links);

  SetWindow(std::move(w));
}

bool
RichTextWidget::SetFocus() noexcept
{
  if (!IsDefined())
    return false;
  GetWindow().SetFocus();
  return true;
}

bool
RichTextWidget::KeyPress(unsigned key_code) noexcept
{
  // Forward key events to the window
  if (IsDefined())
    return GetWindow().InjectKeyPress(key_code);
  return false;
}
