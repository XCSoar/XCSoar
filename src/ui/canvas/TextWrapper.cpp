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

static const char *
AdvanceUTF8Chars(const char *p, const char *end, std::size_t n) noexcept
{
  while (n > 0 && p < end) {
    p += ClampedSequenceLengthUTF8(*p, end - p);
    --n;
  }
  return p;
}

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

/**
 * Glyph-width character budgets (one calibration per WrapText).
 * Short runs skip FreeType; very long runs skip a doomed full measure.
 */
struct WrapBudget {
  std::size_t safe_fit_chars;
  std::size_t must_wrap_chars;
};

[[gnu::pure]]
static WrapBudget
CalibrateWrapBudget(Canvas &canvas, unsigned width) noexcept
{
  const unsigned max_cw =
    std::max(1u, canvas.CalcTextSize("W").width);
  const unsigned min_cw =
    std::max(1u, canvas.CalcTextSize("i").width);

  std::size_t safe = width / max_cw;
  std::size_t must = width / min_cw + 1;
  if (must <= safe)
    must = safe + 1;

  return {safe, must};
}

[[gnu::pure]]
static bool
TextFitsWidth(Canvas &canvas, const char *start, const char *end,
              unsigned width, const WrapBudget &budget) noexcept
{
  if (start >= end)
    return true;

  const std::size_t chars = CountUTF8Chars(start, end);
  if (chars <= budget.safe_fit_chars)
    return true;
  if (chars >= budget.must_wrap_chars)
    return false;

  return canvas.CalcTextSize({start, std::size_t(end - start)}).width
    <= width;
}

static const char *
FindCharWrapBreak(Canvas &canvas, const char *line_start,
                  const char *word_end, unsigned width,
                  const WrapBudget &budget) noexcept
{
  const std::size_t char_count = CountUTF8Chars(line_start, word_end);
  if (char_count == 0)
    return line_start;

  if (char_count <= budget.safe_fit_chars)
    return word_end;

  std::size_t lo = 1;
  std::size_t hi = char_count;
  std::size_t best = 0;

  while (lo <= hi) {
    const std::size_t mid = (lo + hi) / 2;
    const char *mid_end = AdvanceUTF8Chars(line_start, word_end, mid);
    if (TextFitsWidth(canvas, line_start, mid_end, width, budget)) {
      best = mid;
      lo = mid + 1;
    } else
      hi = mid - 1;
  }

  if (best == 0)
    return AdvanceUTF8Chars(line_start, word_end, 1);

  return AdvanceUTF8Chars(line_start, word_end, best);
}

static const char *
FindWrapBreak(Canvas &canvas, const char *line_start, const char *para_end,
              unsigned width, const WrapBudget &budget) noexcept
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

    if (TextFitsWidth(canvas, line_start, extent, width, budget)) {
      break_point = word_end;
      if (word_end < para_end && *word_end == ' ')
        break_point = word_end + 1;
      p = word_end;
      continue;
    }

    if (break_point != nullptr && break_point > line_start)
      return break_point;

    return FindCharWrapBreak(canvas, word_start, word_end, width, budget);
  }

  if (break_point != nullptr && break_point > line_start)
    return break_point;

  return FindCharWrapBreak(canvas, line_start, para_end, width, budget);
}

/**
 * Wrap one paragraph.  Fast path: character budget or one full-line
 * FreeType measure (typical list bullets stay on one line).
 */
static void
WrapParagraph(Canvas &canvas, unsigned width, const WrapBudget &budget,
              const char *para_start, std::size_t para_len,
              std::size_t text_offset,
              std::vector<WrappedTextLine> &lines) noexcept
{
  if (para_len == 0) {
    lines.push_back({text_offset, 0});
    return;
  }

  const char *para_end = para_start + para_len;
  if (TextFitsWidth(canvas, para_start, para_end, width, budget)) {
    lines.push_back({text_offset, para_len});
    return;
  }

  const char *line_start = para_start;
  while (line_start < para_end) {
    if (TextFitsWidth(canvas, line_start, para_end, width, budget)) {
      std::size_t line_len = para_end - line_start;
      if (line_len > 0 && line_start[line_len - 1] == ' ')
        --line_len;
      lines.push_back({
        text_offset + static_cast<std::size_t>(line_start - para_start),
        line_len
      });
      break;
    }

    const char *break_point =
      FindWrapBreak(canvas, line_start, para_end, width, budget);
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

template<typename F>
static void
ForEachParagraph(std::string_view text, F &&fn) noexcept
{
  const char *start = text.data();
  const char *end = start + text.size();
  const char *para_start = start;

  while (para_start < end) {
    const char *para_end = para_start;
    while (para_end < end && *para_end != '\n')
      ++para_end;

    fn(para_start, para_end - para_start,
       static_cast<std::size_t>(para_start - start));

    para_start = para_end;
    if (para_start < end && *para_start == '\n')
      ++para_start;
  }
}

WrappedText
WrapText(Canvas &canvas, unsigned width, std::string_view text) noexcept
{
  WrappedText result;

  if (text.empty() || width == 0)
    return result;

  const WrapBudget budget = CalibrateWrapBudget(canvas, width);
  ForEachParagraph(text, [&](const char *para, std::size_t len,
                             std::size_t offset) {
    WrapParagraph(canvas, width, budget, para, len, offset, result.lines);
  });

  return result;
}

WrappedText
WrapText(const Font &font, unsigned width, std::string_view text) noexcept
{
  AnyCanvas canvas;
  canvas.Select(font);
  return WrapText(canvas, width, text);
}

unsigned
EstimateWrappedLineCount(const Font &font, unsigned width,
                         std::string_view text) noexcept
{
  if (text.empty() || width == 0)
    return 0;

  AnyCanvas canvas;
  canvas.Select(font);

  static constexpr char sample[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  const unsigned sample_chars = sizeof(sample) - 1;
  const unsigned avg_cw = std::max(
    1u,
    (std::max(1u, canvas.CalcTextSize(sample).width) + sample_chars - 1) /
      sample_chars);
  const std::size_t chars_per_line =
    std::max<std::size_t>(1, width / avg_cw);

  unsigned lines = 0;
  ForEachParagraph(text, [&](const char *para, std::size_t len,
                             [[maybe_unused]] std::size_t offset) {
    const std::size_t chars = CountUTF8Chars(para, para + len);
    if (chars == 0)
      ++lines;
    else
      lines += static_cast<unsigned>((chars + chars_per_line - 1) /
                                     chars_per_line);
  });

  return lines + (lines + 9) / 10;
}
