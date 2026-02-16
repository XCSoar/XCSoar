// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AutoFont.hpp"
#include "FontDescription.hpp"
#include "ui/canvas/Font.hpp"

#include <algorithm>

void
AutoSizeFont(FontDescription &d, unsigned width, const char *text)
{
  // JMW algorithm to auto-size info window font.
  // this is still required in case title font property doesn't exist.

  /* reasonable start value (ignoring the original input value) */
  d.SetHeight(std::max(10u, unsigned(width) / 4u));

  Font font;
  font.Load(d);

  /* double the font size until it is large enough */

  PixelSize tsize = font.TextSize(text);
  while (tsize.width < width) {
    d.SetHeight(d.GetHeight() * 2);
    font.Load(d);

    tsize = font.TextSize(text);
  }

  /* decrease font size until it fits exactly */

  do {
    if (d.GetHeight() <= 6)
      /* this is the lower bound; don't go smaller than that, or else
         the size will underflow eventually */
      break;

    d.SetHeight(d.GetHeight() - 1);

    Font font;
    font.Load(d);

    tsize = font.TextSize(text);
  } while (tsize.width > width);

  d.SetHeight(d.GetHeight() + 1);
}
