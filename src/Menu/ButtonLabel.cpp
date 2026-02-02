// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonLabel.hpp"
#include "MenuBar.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"
#include "util/StringBuilder.hxx"
#include "util/StringStrip.hxx"
#include "util/CharUtil.hxx"
#include "util/Macros.hpp"

#include <algorithm>
#include <cstring>

/**
 * @return false if there is at least one ASCII letter in the string
 */
static constexpr bool
LacksAlphaASCII(const char *s) noexcept
{
  for (; *s != 0; ++s)
    if (IsAlphaASCII(*s))
      return false;

  return true;
}

/**
 * Translate a portion of the source string.
 *
 * @return the translated string or nullptr if the buffer is too small
 */
[[gnu::pure]]
static const char *
GetTextN(const char *src, const char *src_end,
         char *buffer, size_t buffer_size) noexcept
{
  if (src == src_end)
    /* gettext("") returns the PO header, and thus we need to exclude
       this special case */
    return "";

  const size_t src_length = src_end - src;
  if (src_length >= buffer_size)
    /* buffer too small */
    return nullptr;

  /* copy to buffer, because gettext() expects a null-terminated
     string */
  *std::copy(src, src_end, buffer) = '\0';

  return gettext(buffer);
}

/**
 * Try to split translated text into 2 or 3 lines by spaces. Optionally
 * append macro_output to the last line. Returns true if buffer was used.
 */
static bool
TrySplitWords(const char *translated, const char *macro_output,
              std::span<char> buffer, ButtonLabel::Expanded &expanded) noexcept
{
  const char *space1 = StringFind(translated, ' ');
  if (space1 == nullptr || space1 <= translated || space1[1] == '\0')
    return false;

  const char *after1 = space1 + 1;
  const char *space2 = StringFind(after1, ' ');
  const bool three_words = space2 != nullptr && space2 > after1 &&
    space2[1] != '\0' &&
    StringFind(space2 + 1, ' ') == nullptr;
  const bool two_words = (space2 == nullptr);

  if (!two_words && !three_words)
    return false;

  const size_t len1 = space1 - translated;
  const size_t len2 = two_words ? std::strlen(after1) : (space2 - after1);
  const size_t len3 = three_words ? std::strlen(space2 + 1) : 0;
  const size_t macro_len =
    (macro_output != nullptr) ? std::strlen(macro_output) : 0;

  size_t need;
  if (three_words)
    need = len1 + 1 + len2 + 1 + len3 + 1 +
      (macro_len ? 1 + macro_len : 0) + 1;
  else
    need = len1 + 1 + len2 + 1 + (macro_len ? 1 + macro_len : 0) + 1;
  if (need > buffer.size())
    return false;

  char *p = buffer.data();
  std::copy(translated, space1, p);
  p[len1] = '\0';
  expanded.text = p;
  p += len1 + 1;

  if (three_words) {
    std::copy(after1, space2, p);
    p[len2] = '\0';
    expanded.text2 = p;
    p += len2 + 1;
    size_t i = 0;
    for (const char *t = space2 + 1; *t; ++t, ++i)
      p[i] = *t;
    if (macro_len) {
      if (len3 != 0)
        p[i++] = ' ';
      for (const char *m = macro_output; *m; ++m, ++i)
        p[i] = *m;
    }
    p[i] = '\0';
    expanded.text3 = p;
    return true;
  }

  /* two words */
  size_t i = 0;
  for (const char *t = after1; *t; ++t, ++i)
    p[i] = *t;
  if (macro_len) {
    p[i++] = ' ';
    for (const char *m = macro_output; *m; ++m, ++i)
      p[i] = *m;
  }
  p[i] = '\0';
  expanded.text2 = p;
  return true;
}

ButtonLabel::Expanded
ButtonLabel::Expand(const char *text, std::span<char> buffer) noexcept
{
  Expanded expanded;

  if (text == nullptr || *text == '\0' || *text == ' ') {
    expanded.visible = false;
    return expanded;
  }

  const char *dollar = StringFind(text, '$');
  if (dollar == nullptr) {
    /* no macro, we can just translate the text */
    expanded.visible = true;
    expanded.enabled = true;
    const char *nl = StringFind(text, '\n');
    if (nl != nullptr && LacksAlphaASCII(nl + 1)) {
      /* Quick hack for skipping the translation for second line of a two line
         label with only digits and punctuation in the second line, e.g.
         for menu labels like "Config\n2/3" */

      char translatable[256];
      const char *translated = GetTextN(text, nl, translatable,
                                         ARRAY_SIZE(translatable));
      if (translated == nullptr) {
        expanded.text = text;
        return expanded;
      }

      try {
        expanded.text = BuildString(buffer, translated, nl);
      } catch (BasicStringBuilder<char>::Overflow) {
        const char *full = gettext(text);
        if (TrySplitWords(full, nullptr, buffer, expanded))
          return expanded;
        expanded.text = full;
      }
      return expanded;
    }

    const char *translated = gettext(text);
    if (TrySplitWords(translated, nullptr, buffer, expanded))
      return expanded;
    expanded.text = translated;
    return expanded;
  }

  /* macro path */
  const char *macros = StripRight(text, dollar);
  char s[100];
  expanded.enabled = !ExpandMacros(text, std::span{s});
  if (s[0] == '\0' || s[0] == ' ') {
    expanded.visible = false;
    return expanded;
  }

  char translatable[256];
  const char *translated = GetTextN(text, macros, translatable,
                                    ARRAY_SIZE(translatable));
  if (translated == nullptr) {
    expanded.visible = false;
    return expanded;
  }

  const size_t prefix_len = dollar - text;
  const char *macro_output =
    (prefix_len < std::strlen(s)) ? s + prefix_len : "";

  expanded.visible = true;
  if (TrySplitWords(translated, macro_output, buffer, expanded))
    return expanded;
  if (translated[0] == '\0' && macro_output[0] != '\0' &&
      TrySplitWords(macro_output, nullptr, buffer, expanded))
    return expanded;

  size_t i = 0;
  for (const char *t = translated; *t && i < buffer.size() - 1; ++t, ++i)
    buffer[i] = *t;
  if (i >= buffer.size())
    return expanded;
  buffer[i++] = '\0';
  const size_t line1_end = i;
  for (const char *m = macro_output; *m && i < buffer.size() - 1; ++m, ++i)
    buffer[i] = *m;
  if (i < buffer.size())
    buffer[i] = '\0';
  expanded.text = buffer.data();
  expanded.text2 = (i > line1_end) ? buffer.data() + line1_end : nullptr;
  return expanded;
}
