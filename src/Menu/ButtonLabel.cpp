/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Util/StringUtil.hpp"
#include "Util/CharUtil.hpp"
#include "Util/Macros.hpp"

#include <assert.h>
#include <algorithm>

static MenuBar *bar;

void
ButtonLabel::CreateButtonLabels(ContainerWindow &parent)
{
  bar = new MenuBar(parent);
}

void
ButtonLabel::SetFont(const Font &Font)
{
  bar->SetFont(Font);
}

void
ButtonLabel::Destroy()
{
  delete bar;
  bar = NULL;
}

/**
 * @return false if there is at least one ASCII letter in the string
 */
gcc_pure
static bool
LacksAlphaASCII(const TCHAR *s)
{
  while (*s) {
    if (IsAlphaASCII(*s))
      return false;

    s++;
  }
  return true;
}

ButtonLabel::Expanded
ButtonLabel::Expand(const TCHAR *text, TCHAR *buffer, size_t size)
{
  Expanded expanded;
  const TCHAR *dollar;

  if ((text == NULL) || (*text == _T('\0')) || (*text == _T(' '))) {
    expanded.visible = false;
    return expanded;
  } else if ((dollar = _tcschr(text, '$')) == NULL) {
    /* no macro, we can just translate the text */
    expanded.visible = true;
    expanded.enabled = true;
    const TCHAR *nl;
    if (((nl = _tcschr(text, '\n')) != NULL) && LacksAlphaASCII(nl + 1)) {
      /* Quick hack for skipping the translation for second line of a two line
         label with only digits and punctuation in the second line, e.g.
         for menu labels like "Config\n2/3" */

      /* copy the text up to the '\n' to a new buffer and translate it */
      TCHAR translatable[256];
      std::copy(text, nl, translatable);
      translatable[nl - text] = _T('\0');

      const TCHAR *translated = StringIsEmpty(translatable)
        ? _T("") : gettext(translatable);

      /* concatenate the translated text and the part starting with '\n' */
      _tcscpy(buffer, translated);
      _tcscat(buffer, nl);

      expanded.text = buffer;
    } else
      expanded.text = gettext(text);
    return expanded;
  } else {
    const TCHAR *macros = dollar;
    /* backtrack until the first non-whitespace character, because we
       don't want to translate whitespace between the text and the
       macro */
    while (macros > text && IsWhitespaceOrNull(macros[-1]))
      --macros;

    TCHAR s[100];
    expanded.enabled = !ExpandMacros(text, s, ARRAY_SIZE(s));
    if (s[0] == _T('\0') || s[0] == _T(' ')) {
      expanded.visible = false;
      return expanded;
    }

    /* copy the text (without trailing whitespace) to a new buffer and
       translate it */
    TCHAR translatable[256];
    std::copy(text, macros, translatable);
    translatable[macros - text] = _T('\0');

    const TCHAR *translated = StringIsEmpty(translatable)
      ? _T("") : gettext(translatable);

    /* concatenate the translated text and the macro output */
    _tcscpy(buffer, translated);
    _tcscat(buffer, s + (macros - text));

    expanded.visible = true;
    expanded.text = buffer;
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
    const MenuItem &item = overlay != NULL && (*overlay)[i].IsDefined()
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
  if (bar != NULL)
    bar->OnResize(rc);
}
