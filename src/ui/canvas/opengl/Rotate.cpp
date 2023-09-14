// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Rotate.hpp"
#include "ui/opengl/Features.hpp"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "Globals.hpp"
#include "ui/dim/Rect.hpp"
#include "DisplayOrientation.hpp"

#include <algorithm>

void
OpenGL::ToViewport(PixelRect &rc)
{
  rc.left += translate.x;
  rc.top += translate.y;
  rc.right += translate.x;
  rc.bottom += translate.y;

  switch (display_orientation) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::LANDSCAPE:
    rc.top = viewport_size.y - rc.top;
    rc.bottom = viewport_size.y - rc.bottom;
    std::swap(rc.top, rc.bottom);
    break;

  case DisplayOrientation::PORTRAIT:
    std::swap(rc.left, rc.top);
    std::swap(rc.right, rc.bottom);
    break;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    rc.left = viewport_size.x - rc.left;
    rc.right = viewport_size.x - rc.right;
    std::swap(rc.left, rc.right);
    break;

  case DisplayOrientation::REVERSE_PORTRAIT:
    rc.top = viewport_size.y - rc.top;
    rc.bottom = viewport_size.y - rc.bottom;
    rc.left = viewport_size.x - rc.left;
    rc.right = viewport_size.x - rc.right;
    std::swap(rc.left, rc.right);
    std::swap(rc.top, rc.bottom);
    std::swap(rc.left, rc.top);
    std::swap(rc.right, rc.bottom);
    break;
  }
}

#endif
