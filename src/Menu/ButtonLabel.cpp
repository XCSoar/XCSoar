/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "ButtonLabel.hpp"
#include "MenuBar.hpp"
#include "MenuData.hpp"
#include "Language/Language.hpp"
#include "Util/StringAPI.hxx"
#include "Util/StringBuilder.hxx"
#include "Util/StringUtil.hpp"
#include "Util/CharUtil.hpp"
#include "Util/Macros.hpp"

#include <algorithm>

static MenuBar *bar;

void
ButtonLabel::CreateButtonLabels(ContainerWindow &parent, ButtonLook &look)
{
  bar = new MenuBar(parent, look);
}

void
ButtonLabel::Destroy()
{
  delete bar;
  bar = nullptr;
}

/**
 * @return false if there is at least one ASCII letter in the string
 */
gcc_pure
static bool
LacksAlphaASCII(const TCHAR *s)
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
gcc_pure
static const TCHAR *
GetTextN(const TCHAR *src, const TCHAR *src_end,
         TCHAR *buffer, size_t buffer_size)
{
  if (src == src_end)
    /* gettext("") returns the PO header, and thus we need to exclude
       this special case */
    return _T("");

  const size_t src_length = src_end - src;
  if (src_length >= buffer_size)
    /* buffer too small */
    return nullptr;

  /* copy to buffer, because gettext() expects a null-terminated
     string */
  *std::copy(src, src_end, buffer) = _T('\0');

  return gettext(buffer);
}

ButtonLabel::Expanded
ButtonLabel::Expand(const TCHAR *text, TCHAR *buffer, size_t size)
{
  Expanded expanded;
  const TCHAR *dollar;

  if (text == nullptr || *text == _T('\0') || *text == _T(' ')) {
    expanded.visible = false;
    return expanded;
  } else if ((dollar = StringFind(text, '$')) == nullptr) {
    /* no macro, we can just translate the text */
    expanded.visible = true;
    expanded.enabled = true;
    const TCHAR *nl = StringFind(text, '\n');
    if (nl != nullptr && LacksAlphaASCII(nl + 1)) {
      /* Quick hack for skipping the translation for second line of a two line
         label with only digits and punctuation in the second line, e.g.
         for menu labels like "Config\n2/3" */

      /* copy the text up to the '\n' to a new buffer and translate it */
      TCHAR translatable[256];
      const TCHAR *translated = GetTextN(text, nl, translatable,
                                         ARRAY_SIZE(translatable));
      if (translated == nullptr) {
        /* buffer too small: keep it untranslated */
        expanded.text = text;
        return expanded;
      }

      /* concatenate the translated text and the part starting with '\n' */
      expanded.text = BuildString(buffer, size, translated, nl);
    } else
      expanded.text = gettext(text);
    return expanded;
  } else {
    const TCHAR *macros = dollar;
    /* backtrack until the first non-whitespace character, because we
       don't want to translate whitespace between the text and the
       macro */
    macros = StripRight(text, macros);

    TCHAR s[100];
    expanded.enabled = !ExpandMacros(text, s, ARRAY_SIZE(s));
    if (s[0] == _T('\0') || s[0] == _T(' ')) {
      expanded.visible = false;
      return expanded;
    }

    /* copy the text (without trailing whitespace) to a new buffer and
       translate it */
    TCHAR translatable[256];
    const TCHAR *translated = GetTextN(text, macros, translatable,
                                       ARRAY_SIZE(translatable));
    if (translated == nullptr) {
      /* buffer too small: fail */
      // TODO: find a more clever fallback
      expanded.visible = false;
      return expanded;
    }

    /* concatenate the translated text and the macro output */
    expanded.visible = true;
    expanded.text = BuildString(buffer, size, translated, s + (macros - text));
    return expanded;
  }
}

void
ButtonLabel::SetLabelText(unsigned index, const TCHAR *text, unsigned event)
{
  TCHAR buffer[100];
  Expanded expanded = Expand(text, buffer, ARRAY_SIZE(buffer));
  if (expanded.visible)
    bar->ShowButton(index, expanded.enabled, expanded.text, event);
  else
    bar->HideButton(index);
}

void
ButtonLabel::Set(const Menu &menu, const Menu *overlay, bool full)
{
  for (unsigned i = 0; i < menu.MAX_ITEMS; ++i) {
    const MenuItem &item = overlay != nullptr && (*overlay)[i].IsDefined()
      ? (*overlay)[i]
      : menu[i];

    if (full || item.IsDynamic())
      SetLabelText(i, item.label, item.event);
  }
}

bool
ButtonLabel::IsEnabled(unsigned i)
{
  return bar->IsButtonEnabled(i);
}

void
ButtonLabel::OnResize(const PixelRect &rc)
{
  if (bar != nullptr)
    bar->OnResize(rc);
}
