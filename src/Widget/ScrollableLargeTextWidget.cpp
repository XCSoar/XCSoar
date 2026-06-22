// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScrollableLargeTextWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/control/LargeTextWindow.hpp"
#include "util/UTF8.hpp"

#include <algorithm>
#include <string_view>

static constexpr unsigned ESTIMATED_LINE_WIDTH_CHARS = 28;

static unsigned
EstimateLineCount(const std::string &text) noexcept
{
  if (text.empty())
    return 1;

  unsigned line_count = 1;
  unsigned line_length = 0;
  for (const char *p = text.c_str(); *p != '\0';) {
    if (*p == '\n') {
      ++line_count;
      line_length = 0;
      ++p;
    } else {
      ++line_length;
      if (line_length == ESTIMATED_LINE_WIDTH_CHARS) {
        ++line_count;
        line_length = 0;
      }

      const std::size_t sequence_length = SequenceLengthUTF8(p);
      p += sequence_length > 0 ? sequence_length : 1;
    }
  }

  return line_count;
}

void
ScrollableLargeTextWidget::SetText(const char *_text)
{
  text = _text != nullptr ? _text : "";

  if (IsDefined()) {
    LargeTextWindow &w = (LargeTextWindow &)GetWindow();
    w.SetText(text.c_str());
  }
}

PixelSize
ScrollableLargeTextWidget::GetMinimumSize() const noexcept
{
  return PixelSize{0u, Layout::GetMinimumControlHeight()};
}

PixelSize
ScrollableLargeTextWidget::GetMaximumSize() const noexcept
{
  const Font &font = look.text_font;
  const unsigned padding = Layout::GetTextPadding() * 2;

  if (IsDefined()) {
    const LargeTextWindow &w = (const LargeTextWindow &)GetWindow();
    const unsigned width = w.GetSize().width;
    const unsigned text_width =
      width > padding * 2 ? width - padding * 2 : width;

    TextRenderer renderer;
    const unsigned text_height =
      renderer.GetHeight(font, text_width, std::string_view{text});
    const unsigned estimated_height =
      EstimateLineCount(text) * font.GetLineSpacing();

    return PixelSize{0u, std::max(text_height, estimated_height) +
                     padding * 2};
  }

  const unsigned line_count = EstimateLineCount(text);
  return PixelSize{0u, line_count * font.GetLineSpacing() + padding * 2};
}

void
ScrollableLargeTextWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc) noexcept
{
  LargeTextWidget::Prepare(parent, rc);

  try {
    LargeTextWindow &w = (LargeTextWindow &)GetWindow();
    w.SetText(text.c_str());
  } catch (...) {
    /* Preserve the noexcept Prepare() contract if copying text fails. */
  }
}
