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

ButtonLabel::Expanded
ButtonLabel::Expand(const char *text, std::span<char> buffer) noexcept
{
  Expanded expanded;
  const char *dollar;

  if (text == nullptr || *text == '\0' || *text == ' ') {
    expanded.visible = false;
    return expanded;
  } else if ((dollar = StringFind(text, '$')) == nullptr) {
    /* no macro, we can just translate the text */
    expanded.visible = true;
    expanded.enabled = true;
    const char *nl = StringFind(text, '\n');
    if (nl != nullptr && LacksAlphaASCII(nl + 1)) {
      /* Quick hack for skipping the translation for second line of a two line
         label with only digits and punctuation in the second line, e.g.
         for menu labels like "Config\n2/3" */

      /* copy the text up to the '\n' to a new buffer and translate it */
      char translatable[256];
      const char *translated = GetTextN(text, nl, translatable,
                                         ARRAY_SIZE(translatable));
      if (translated == nullptr) {
        /* buffer too small: keep it untranslated */
        expanded.text = text;
        return expanded;
      }

      /* concatenate the translated text and the part starting with '\n' */
      try {
        expanded.text = BuildString(buffer, translated, nl);
      } catch (BasicStringBuilder<char>::Overflow) {
        expanded.text = gettext(text);
      }
    } else
      expanded.text = gettext(text);
    return expanded;
  } else {
    const char *macros = dollar;
    /* backtrack until the first non-whitespace character, because we
       don't want to translate whitespace between the text and the
       macro */
    macros = StripRight(text, macros);

    char s[100];
    expanded.enabled = !ExpandMacros(text, std::span{s});
    if (s[0] == '\0' || s[0] == ' ') {
      expanded.visible = false;
      return expanded;
    }

    /* copy the text (without trailing whitespace) to a new buffer and
       translate it */
    char translatable[256];
    const char *translated = GetTextN(text, macros, translatable,
                                       ARRAY_SIZE(translatable));
    if (translated == nullptr) {
      /* buffer too small: fail */
      // TODO: find a more clever fallback
      expanded.visible = false;
      return expanded;
    }

    /* concatenate the translated text and the macro output */
    expanded.visible = true;
    expanded.text = BuildString(buffer, translated, s + (macros - text));
    return expanded;
  }
}
