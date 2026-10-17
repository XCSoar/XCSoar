// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QRCodeRenderer.hpp"

#include "QR/QRCode.hpp"
#include "Resources.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/dim/Rect.hpp"

#include <algorithm>

static const Bitmap &
SwiftLogo(unsigned pixels) noexcept
{
  static Bitmap huge, big, small;
  static bool ready = false;
  if (!ready) {
    ready = true;
    try {
      huge.Load(IDB_LOGO_UHD);
      big.Load(IDB_LOGO_HD);
      small.Load(IDB_LOGO);
    } catch (...) {
    }
  }

  if (pixels >= 180 && huge.IsDefined()) return huge;
  if (pixels >= 64 && big.IsDefined()) return big;
  return small;
}

void
DrawQRSymbol(Canvas &canvas,
             const PixelRect &rc,
             const QRSymbol &symbol) noexcept
{
  canvas.Clear(COLOR_WHITE);

  if (!symbol.IsValid() || rc.GetWidth() == 0 || rc.GetHeight() == 0) return;

  const unsigned modules = symbol.width;
  const unsigned span    = std::min(rc.GetWidth(), rc.GetHeight());
  const unsigned scale   = std::max(1u, span / modules);
  const unsigned drawn   = scale * modules;
  const int origin_x = rc.left + static_cast<int>((rc.GetWidth() - drawn) / 2);
  const int origin_y = rc.top + static_cast<int>((rc.GetHeight() - drawn) / 2);

  for (unsigned y = 0; y < symbol.height; ++y) {
    for (unsigned x = 0; x < symbol.width; ++x) {
      if (!symbol.IsBlack(x, y)) continue;

      canvas.DrawFilledRectangle(
        PixelRect{
          PixelPoint{origin_x + static_cast<int>(x * scale),
                     origin_y + static_cast<int>(y * scale)},
          PixelSize{scale, scale},
        },
        COLOR_BLACK);
    }
  }

  const QRCentre mark = QRCodeCentreMark(symbol);
  if (mark.size == 0) return;

  const unsigned plate = mark.size * scale;
  const int plate_x    = origin_x + static_cast<int>(mark.left * scale);
  const int plate_y    = origin_y + static_cast<int>(mark.top * scale);
  canvas.DrawFilledRectangle(
    PixelRect{
      PixelPoint{plate_x, plate_y},
      PixelSize{plate, plate},
    },
    COLOR_WHITE);

  /* One white module between the bird and the modules around it. */
  const unsigned pad   = mark.size > 4 ? scale : 0;
  const unsigned logo  = plate - 2 * pad;
  const Bitmap &bitmap = SwiftLogo(logo);
  if (logo == 0 || !bitmap.IsDefined()) return;

  canvas.Stretch(PixelPoint{plate_x + static_cast<int>(pad),
                            plate_y + static_cast<int>(pad)},
                 PixelSize{logo, logo},
                 bitmap);
}
