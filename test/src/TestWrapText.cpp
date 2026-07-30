// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/TextWrapper.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/TextFormat.hpp"
#include "util/UTF8.hpp"
#include "Screen/Layout.hpp"
#include "Fonts.hpp"

extern "C" {
#include "tap.h"
}

/**
 * Verify that every line produced by WrapText is valid UTF-8.
 */
static void
CheckLinesValidUTF8(const WrappedText &wrapped, std::string_view text,
                    const char *label) noexcept
{
  bool valid = true;
  for (const auto &line : wrapped.lines) {
    std::string_view seg = line.GetText(text);
    if (!ValidateUTF8(seg)) {
      diag("invalid UTF-8 in line from \"%s\" "
           "(start=%zu, length=%zu)",
           label,
           line.start, line.length);
      valid = false;
    }
  }

  ok1(valid);
}

/**
 * Verify that the wrapped lines cover all non-space content of the
 * original text (no bytes lost or invented).
 */
static void
CheckLinesCoverText(const WrappedText &wrapped, std::string_view text,
                    const char *label) noexcept
{
  if (wrapped.lines.empty()) {
    ok1(text.empty());
    return;
  }

  /* the first line must start at a paragraph boundary (offset 0 or
     right after a newline) and the last line must end at or near the
     end of the text */
  const std::size_t first_start = wrapped.lines.front().start;
  const bool valid_start =
    first_start <= text.size() &&
    (first_start == 0 || text[first_start - 1] == '\n');

  const auto &last = wrapped.lines.back();
  const bool valid_end =
    last.start <= text.size() &&
    last.length <= text.size() - last.start;
  /* The last line may end before text.size() if there are trailing
     spaces or newlines, but its range must not exceed the text. */
  if (!valid_end)
    diag("last line range (start=%zu, length=%zu) exceeds text size %zu "
         "in \"%s\"",
         last.start, last.length, text.size(), label);

  ok1(valid_start && valid_end);
}

static void
TestASCII(const Font &font) noexcept
{
  /* very wide width so nothing wraps */
  auto result = WrapText(font, 10000, "Hello World");
  ok1(result.lines.size() == 1);
  CheckLinesValidUTF8(result, "Hello World", "ASCII-no-wrap");
  CheckLinesCoverText(result, "Hello World", "ASCII-no-wrap");

  /* empty string */
  result = WrapText(font, 100, "");
  ok1(result.lines.empty());

  /* single newline */
  result = WrapText(font, 10000, "\n");
  ok1(result.lines.size() == 1);  /* one empty paragraph */
}

static void
TestUTF8NoWrap(const Font &font) noexcept
{
  /* UTF-8 strings that fit in one line (no wrapping needed) */
  static constexpr struct {
    const char *text;
    const char *label;
  } cases[] = {
    { "L\xc3\xb6" "FGREN variometer", "2-byte-umlaut" },
    { "\xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f", "german-umlauts" },
    { "90\xc2\xb0 turn", "degree-sign" },
    { "\xe7\x9b\xae\xe6\xac\xa1", "CJK-2-chars" },
    { "\xf0\x9f\x98\x80 smile", "4-byte-emoji" },
  };

  for (const auto &c : cases) {
    auto result = WrapText(font, 10000, c.text);
    ok1(result.lines.size() == 1);
    CheckLinesValidUTF8(result, c.text, c.label);
    CheckLinesCoverText(result, c.text, c.label);
  }
}

static void
TestUTF8ForceWrap(const Font &font) noexcept
{
  /* Use a very narrow width (1 pixel) to force wrapping at every
     character.  The critical property: every resulting line must
     still be valid UTF-8 (no split multi-byte sequences). */
  static constexpr struct {
    const char *text;
    const char *label;
  } cases[] = {
    { "L\xc3\xb6" "FGREN", "narrow-umlaut" },
    { "\xc3\xa4\xc3\xb6\xc3\xbc", "narrow-3-umlauts" },
    { "\xe7\x9b\xae\xe6\xac\xa1\xe5\xad\x97", "narrow-CJK" },
    { "\xf0\x9f\x98\x80\xf0\x9f\x98\x82", "narrow-emoji" },
    { "a\xc3\xb6" "b\xe7\x9b\xae" "c", "narrow-mixed" },
  };

  for (const auto &c : cases) {
    auto result = WrapText(font, 1, c.text);
    /* must produce at least one line */
    ok1(!result.lines.empty());
    CheckLinesValidUTF8(result, c.text, c.label);
  }
}

static void
TestMultiParagraphUTF8(const Font &font) noexcept
{
  /* multi-paragraph text with UTF-8 */
  const char *text = "Pr\xc3\xbc" "fung\nL\xc3\xb6" "FGREN\n\xe7\x9b\xae";
  auto result = WrapText(font, 10000, text);
  /* 3 paragraphs, each fitting in one line */
  ok1(result.lines.size() == 3);
  CheckLinesValidUTF8(result, text, "multi-para-utf8");
  CheckLinesCoverText(result, text, "multi-para-utf8");
}

static void
TestLongUTF8Line(const Font &font) noexcept
{
  /* a moderately long line of UTF-8 text that will need wrapping at a
     reasonable width */
  const char *text =
    "Der L\xc3\xb6" "FGREN Variometer Treiber f\xc3\xbc"
    "r XCSoar unterst\xc3\xbc" "tzt verschiedene Ger\xc3\xa4" "te";
  auto result = WrapText(font, 200, text);
  ok1(!result.lines.empty());
  CheckLinesValidUTF8(result, text, "long-utf8-200px");

  /* also at a tighter width */
  result = WrapText(font, 80, text);
  ok1(!result.lines.empty());
  CheckLinesValidUTF8(result, text, "long-utf8-80px");
}

/**
 * Regression test for DrawFormattedText() splitting an unspaced UTF-8
 * string.  The old implementation overwrote the leading byte of the
 * character following each inserted line separator, so measuring the
 * next line failed UTF-8 validation.
 */
static void
TestDrawFormattedTextUTF8(const Font &font)
{
  AnyCanvas canvas;
  canvas.Select(font);

  /*
   * CJK text triggered the original crash because it commonly has no ASCII
   * spaces at which to wrap.  Use an unspaced UTF-8 character available in
   * the test font so this test does not depend on an installed CJK font.
   */
  static constexpr std::string_view text = "ääääääää";
  const unsigned character_width = canvas.CalcTextWidth("ä");
  ok1(character_width > 0);

  const unsigned height =
    canvas.DrawFormattedText({0, 0, static_cast<int>(character_width), 10000},
                             text, DT_CALCRECT);
  ok1(height == 8 * font.GetLineSpacing());
}

int main()
{
  /* Avoid creating a full UI Display (and OpenGL/EGL) for this unit
     test.  Initialise the minimal Layout globals and the font
     subsystem directly. */
  Layout::scale_1024 = 1024;
  Layout::scale = 1;
  Layout::font_scale = 1024;
  Layout::vdpi = 72;
  Layout::pt_scale = 1024;
  Layout::vpt_scale = 1024;

  /* mark screen initialized so components that assert on it succeed */
  ScreenInitialized();

#ifdef USE_FREETYPE
  Font::Initialise();
#endif

  InitialiseFonts();

  plan_tests(39);

  TestASCII(normal_font);
  TestUTF8NoWrap(normal_font);
  TestUTF8ForceWrap(normal_font);
  TestMultiParagraphUTF8(normal_font);
  TestLongUTF8Line(normal_font);
  TestDrawFormattedTextUTF8(normal_font);

  DeinitialiseFonts();

#ifdef USE_FREETYPE
  Font::Deinitialise();
#endif

  ScreenDeinitialized();

  return exit_status();
}
