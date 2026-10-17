// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct QRSymbol;
class Canvas;
struct PixelRect;

/**
 * Draw a QR symbol as sharp black modules on white, with the swift
 * in the centre.
 *
 * The symbol is centred in @p rc and scaled by whole modules, so the
 * edges stay crisp on OpenGL and on the memory canvas.  The swift
 * covers only the square @ref QRCodeCentreMark() allows.
 */
void DrawQRSymbol(Canvas &canvas,
                  const PixelRect &rc,
                  const QRSymbol &symbol) noexcept;
