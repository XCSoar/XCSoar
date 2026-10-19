// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct QRSymbol;
class Canvas;
class MaskedIcon;
struct PixelRect;

/**
 * Draw a QR symbol as sharp black modules on white.
 *
 * The symbol is centred in @p rc and scaled by whole modules, so the
 * edges stay crisp on OpenGL and on the memory canvas.  The centre
 * covers only the square @ref QRCodeCentreMark() allows.  Without
 * @p centre the swift is drawn there.
 */
void DrawQRSymbol(Canvas &canvas,
                  const PixelRect &rc,
                  const QRSymbol &symbol,
                  const MaskedIcon *centre = nullptr) noexcept;
