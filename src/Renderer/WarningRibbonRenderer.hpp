// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelSize;
struct PixelPoint;
class Canvas;
class Font;

/**
 * A "REMOVE BEFORE FLIGHT" streamer: white capitals on a red ribbon
 * that is barely taller than the letters.  The caller selects the
 * font and chooses where the ribbon sits.
 */
namespace WarningRibbonRenderer {

/** Outer size for this face.  Zero when there is nothing to paint. */
[[nodiscard]]
PixelSize Size(const Font &font, const char *text) noexcept;

/** Paint the streamer with this face. */
void Draw(Canvas &canvas, PixelPoint top_left,
          const Font &font, const char *text) noexcept;

} // namespace WarningRibbonRenderer
