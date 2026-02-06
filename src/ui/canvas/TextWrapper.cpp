// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextWrapper.hpp"
#include "Canvas.hpp"
#include "AnyCanvas.hpp"

#include <string_view>

/**
 * Wrap a single paragraph (no embedded newlines) into lines.
 */
static void
WrapParagraph(Canvas &canvas, unsigned width,
              const char *para_start, std::size_t para_len,
              std::size_t text_offset,
              std::vector<WrappedTextLine> &lines) noexcept
{
  if (para_len == 0) {
    // Empty paragraph (blank line)
    lines.push_back({text_offset, 0});
    return;
  }

  const char *para_end = para_start + para_len;
  const char *line_start = para_start;

  while (line_start < para_end) {
    // Measure remaining text
    const std::size_t remaining = para_end - line_start;
    std::string_view remaining_text(line_start, remaining);
    const PixelSize full_size = canvas.CalcTextSize(remaining_text);

    if (full_size.width <= width) {
      // Remaining text fits on one line
      lines.push_back({
        text_offset + static_cast<std::size_t>(line_start - para_start),
        remaining
      });
      break;
    }

    // Need to wrap - find last space that fits
    const char *break_point = nullptr;
    const char *last_space = nullptr;

    for (const char *p = line_start; p < para_end; ++p) {
      std::string_view test_text(line_start, p + 1 - line_start);
      const PixelSize test_size = canvas.CalcTextSize(test_text);
      if (test_size.width > width) {
        // Text up to p+1 doesn't fit
        if (last_space != nullptr) {
          // Break at last space
          break_point = last_space;
        } else if (p > line_start) {
          // No space found - break at previous character
          break_point = p;
        } else {
          // Single character too wide - include it anyway
          break_point = p + 1;
        }
        break;
      }

      if (*p == ' ')
        last_space = p + 1;  // Break after space

      break_point = p + 1;
    }

    if (break_point == nullptr || break_point <= line_start)
      break_point = line_start + 1;  // Ensure progress

    // Calculate line length (exclude trailing space if we broke at space)
    std::size_t line_len = break_point - line_start;
    if (line_len > 0 && line_start[line_len - 1] == ' ')
      --line_len;

    lines.push_back({
      text_offset + static_cast<std::size_t>(line_start - para_start),
      line_len
    });

    // Move to next line, skipping any spaces at break point
    line_start = break_point;
    while (line_start < para_end && *line_start == ' ')
      ++line_start;
  }
}

WrappedText
WrapText(Canvas &canvas, unsigned width, std::string_view text) noexcept
{
  WrappedText result;

  if (text.empty() || width == 0)
    return result;

  const char *start = text.data();
  const char *end = start + text.size();
  const char *para_start = start;

  while (para_start < end) {
    // Find end of current paragraph (newline or end of text)
    const char *para_end = para_start;
    while (para_end < end && *para_end != '\n')
      ++para_end;

    const std::size_t text_offset = para_start - start;
    WrapParagraph(canvas, width, para_start, para_end - para_start,
                  text_offset, result.lines);

    // Skip past newline
    para_start = para_end;
    if (para_start < end && *para_start == '\n')
      ++para_start;
  }

  return result;
}

WrappedText
WrapText(const Font &font, unsigned width, std::string_view text) noexcept
{
  AnyCanvas canvas;
  canvas.Select(font);
  return WrapText(canvas, width, text);
}
