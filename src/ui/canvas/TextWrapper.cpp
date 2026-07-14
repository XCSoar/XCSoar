// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextWrapper.hpp"
#include "Canvas.hpp"
#include "AnyCanvas.hpp"
#include "util/UTF8.hpp"

#include <algorithm>
#include <string_view>

/**
 * Return the byte length of the UTF-8 character starting at @p ch,
 * clamped to @p max_bytes.  Returns at least 1 even for malformed
 * input so callers always make forward progress.
 */
static std::size_t
ClampedSequenceLengthUTF8(char ch, std::size_t max_bytes) noexcept
{
  std::size_t n = SequenceLengthUTF8(ch);
  if (n == 0)
    /* malformed: skip one byte */
    n = 1;
  return std::min(n, max_bytes);
}

/**
 * Advance @p p by @p n whole UTF-8 characters, not past @p end.
 */
static const char *
AdvanceUTF8Chars(const char *p, const char *end, std::size_t n) noexcept
{
  while (n > 0 && p < end) {
    p += ClampedSequenceLengthUTF8(*p, end - p);
    --n;
  }
  return p;
}

/**
 * Count whole UTF-8 characters in [start, end).
 */
static std::size_t
CountUTF8Chars(const char *start, const char *end) noexcept
{
  std::size_t n = 0;
  for (const char *p = start; p < end;) {
    p += ClampedSequenceLengthUTF8(*p, end - p);
    ++n;
  }
  return n;
}

[[gnu::pure]]
static bool
TextFitsWidth(Canvas &canvas, const char *start, const char *end,
              unsigned width) noexcept
{
  if (start >= end)
    return true;

  const PixelSize size =
    canvas.CalcTextSize({start, std::size_t(end - start)});
  return size.width <= width;
}

/**
 * Break within [line_start, word_end) when a single word exceeds @p width.
 * Returns a pointer past the last UTF-8 character that still fits; at
 * least one character is included even when it is wider than @p width.
 */
static const char *
FindCharWrapBreak(Canvas &canvas, const char *line_start,
                  const char *word_end, unsigned width) noexcept
{
  const std::size_t char_count = CountUTF8Chars(line_start, word_end);
  if (char_count == 0)
    return line_start;

  std::size_t lo = 1;
  std::size_t hi = char_count;
  std::size_t best = 0;

  while (lo <= hi) {
    const std::size_t mid = (lo + hi) / 2;
    const char *mid_end = AdvanceUTF8Chars(line_start, word_end, mid);
    if (TextFitsWidth(canvas, line_start, mid_end, width)) {
      best = mid;
      lo = mid + 1;
    } else
      hi = mid - 1;
  }

  if (best == 0)
    return AdvanceUTF8Chars(line_start, word_end, 1);

  return AdvanceUTF8Chars(line_start, word_end, best);
}

/**
 * Find the next line break for text that does not fit on one line.
 * Breaks at the last word boundary that fits, or within a word when
 * needed.
 */
static const char *
FindWrapBreak(Canvas &canvas, const char *line_start, const char *para_end,
              unsigned width) noexcept
{
  const char *p = line_start;
  const char *break_point = nullptr;

  while (p < para_end) {
    while (p < para_end && *p == ' ')
      ++p;
    if (p >= para_end)
      break;

    const char *word_start = p;
    while (p < para_end && *p != ' ')
      ++p;
    const char *word_end = p;

    const char *extent = word_end;
    if (extent < para_end && *extent == ' ')
      ++extent;

    if (TextFitsWidth(canvas, line_start, extent, width)) {
      break_point = word_end;
      if (word_end < para_end && *word_end == ' ')
        break_point = word_end + 1;
      p = word_end;
      continue;
    }

    if (break_point != nullptr && break_point > line_start)
      return break_point;

    return FindCharWrapBreak(canvas, word_start, word_end, width);
  }

  if (break_point != nullptr && break_point > line_start)
    return break_point;

  return FindCharWrapBreak(canvas, line_start, para_end, width);
}

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
    const std::size_t remaining = para_end - line_start;
    if (TextFitsWidth(canvas, line_start, para_end, width)) {
      lines.push_back({
        text_offset + static_cast<std::size_t>(line_start - para_start),
        remaining
      });
      break;
    }

    const char *break_point =
      FindWrapBreak(canvas, line_start, para_end, width);
    if (break_point <= line_start) {
      const std::size_t seq_len =
        ClampedSequenceLengthUTF8(*line_start, para_end - line_start);
      break_point = line_start + seq_len;
    }

    std::size_t line_len = break_point - line_start;
    if (line_len > 0 && line_start[line_len - 1] == ' ')
      --line_len;

    lines.push_back({
      text_offset + static_cast<std::size_t>(line_start - para_start),
      line_len
    });

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
    const char *para_end = para_start;
    while (para_end < end && *para_end != '\n')
      ++para_end;

    const std::size_t text_offset = para_start - start;
    WrapParagraph(canvas, width, para_start, para_end - para_start,
                  text_offset, result.lines);

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
