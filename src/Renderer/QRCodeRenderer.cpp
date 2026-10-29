// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QRCodeRenderer.hpp"

#include "QR/QRCode.hpp"
#include "Resources.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Icon.hpp"
#include "ui/dim/Rect.hpp"

#include <algorithm>

void
DrawQRSymbol(Canvas &canvas,
             const PixelRect &rc,
             const QRSymbol &symbol,
             const MaskedIcon *centre) noexcept
{
  canvas.Clear(COLOR_WHITE);

  if (!symbol.IsValid() || rc.left >= rc.right || rc.top >= rc.bottom)
    return;

  const unsigned modules = symbol.width;
  const unsigned span = std::min(unsigned(rc.right - rc.left),
                                  unsigned(rc.bottom - rc.top));
  const unsigned scale = std::max(1u, span / modules);
  const unsigned drawn = scale * modules;
  const int origin_x = rc.left + int((rc.right - rc.left) - int(drawn)) / 2;
  const int origin_y = rc.top + int((rc.bottom - rc.top) - int(drawn)) / 2;

  /* One rectangle per horizontal run, not one per module.  A module
     each is enough draw calls to fault the OpenGL driver thread. */
  for (unsigned y = 0; y < symbol.height; ++y) {
    unsigned x = 0;
    while (x < symbol.width) {
      if (!symbol.IsBlack(x, y)) {
        ++x;
        continue;
      }

      const unsigned start = x;
      do
        ++x;
      while (x < symbol.width && symbol.IsBlack(x, y));

      canvas.DrawFilledRectangle(
        PixelRect{
          PixelPoint{origin_x + int(start * scale),
                     origin_y + int(y * scale)},
          PixelSize{(x - start) * scale, scale},
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

  /* One white module between the mark and the modules around it. */
  const unsigned pad = mark.size > 4 ? scale : 0;
  const unsigned logo = plate - 2 * pad;
  if (logo == 0)
    return;

  if (centre != nullptr && centre->IsDefined()) {
    const PixelSize native = centre->GetSize();
    unsigned target = logo;
    /* Draw() keeps the aspect ratio. A wider icon would leave
       the white centre and cover modules. */
    if (native.width > native.height && native.width > 0) {
      target = logo * native.height / native.width;
      if (target == 0)
        return;
    }

    const PixelPoint at{plate_x + int(plate / 2),
                        plate_y + int(plate / 2)};
    centre->Draw(canvas, at, target);
    return;
  }

  /* shortcut: reloads the logo each paint, cache on the dialog if
     the QR page is redrawn often */
  Bitmap bitmap;
  try {
    if (logo >= 180) bitmap.Load(IDB_LOGO_UHD);
    if (!bitmap.IsDefined() && logo >= 64) bitmap.Load(IDB_LOGO_HD);
    if (!bitmap.IsDefined()) bitmap.Load(IDB_LOGO);
  } catch (...) {
  }
  if (!bitmap.IsDefined())
    return;

  canvas.Stretch(PixelPoint{plate_x + int(pad), plate_y + int(pad)},
                 PixelSize{logo, logo},
                 bitmap);
}
