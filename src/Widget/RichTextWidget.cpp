// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RichTextWidget.hpp"
#include "RichTextWindowWithInternalLinks.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/TextWrapper.hpp"

#include <algorithm>

PixelSize
RichTextWidget::GetMinimumSize() const noexcept
{
  return PixelSize{Layout::FastScale(200), Layout::FastScale(100)};
}

PixelSize
RichTextWidget::GetMaximumSize() const noexcept
{
  PixelSize size{Layout::FastScale(600), Layout::FastScale(200)};
  bool from_content = false;

  if (IsDefined()) {
    const auto &w = static_cast<const RichTextWindow &>(GetWindow());
    const unsigned content_height = w.GetContentHeight();
    if (content_height > 0) {
      size.height = content_height;
      from_content = true;
    }
  }

  if (!from_content && !text.empty()) {
    const Font &font = GetLook().text_font;
    const unsigned line_height = font.GetLineSpacing();
    const unsigned estimate_width = Layout::FastScale(600);
    const unsigned line_count =
      std::max(1u, EstimateWrappedLineCount(font, estimate_width, text));
    const unsigned padding = Layout::GetTextPadding();
    size.height = line_count * line_height + padding * 2;
  }

  /* TwoWidgets requires maximum >= minimum. A short code and URL
     would otherwise be shorter than GetMinimumSize(). */
  const PixelSize minimum = GetMinimumSize();
  if (size.width < minimum.width)
    size.width = minimum.width;
  if (size.height < minimum.height)
    size.height = minimum.height;
  return size;
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
  w->SetFont(dialog_look.text_font, &dialog_look.bold_font,
             &dialog_look.heading1_font, &dialog_look.heading2_font);
  w->SetDarkMode(dialog_look.dark_mode, dialog_look.background_color);
  w->SetDialogLook(dialog_look);

  if (link_return_callback)
    w->SetInternalLinkReturnCallback(link_return_callback);

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
  /* Only when the window has focus.  Otherwise Up/Down/Enter would
     move the link cursor or activate a link while Close (or another
     control) is focused; same rule as #ListWidget. */
  if (!IsDefined() || !HasFocus())
    return false;

  return GetWindow().InjectKeyPress(key_code);
}
