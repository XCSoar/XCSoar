// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GestureLook.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

void
GestureLook::Initialise()
{
  if (IsDithered()) {
    /* colour is not useful on e-ink screens */
    color = COLOR_BLACK;
    invalid_color = COLOR_GRAY;
  } else {
    color = Color(0xf0, 0x14, 0x2d);

#ifdef ENABLE_OPENGL
    /* "not (yet) recognised": the same red, slightly translucent;
       a subtle difference, the label names the recognised gesture */
    invalid_color = color.WithAlpha(0xb3);
#else
    /* no translucency without OpenGL */
    invalid_color = Color(0xa8, 0xa8, 0xa8);
#endif
  }

  width = Layout::ScalePenWidth(6);
}
