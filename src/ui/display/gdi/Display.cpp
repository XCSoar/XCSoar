// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Display.hpp"
#include "ui/canvas/gdi/RootDC.hpp"
#include "ui/dim/Size.hpp"
#include "Math/Point2D.hpp"

#include <wingdi.h>

namespace GDI {

PixelSize
Display::GetSize() const noexcept
{
  RootDC dc;
  return {GetDeviceCaps(dc, HORZRES), GetDeviceCaps(dc, VERTRES)};
}

UnsignedPoint2D
Display::GetDPI() const noexcept
{
  RootDC dc;
  return UnsignedPoint2D(GetDeviceCaps(dc, LOGPIXELSX),
                         GetDeviceCaps(dc, LOGPIXELSY));
}

} // namespace GDI
