// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/TextRowRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"

class Canvas;
struct PixelRect;

/** Empty placeholder for single-row lists. */
void DrawEmptyDownloadHint(TextRowRenderer &renderer, Canvas &canvas,
                           PixelRect rc) noexcept;

/** Lay out @p renderer for #DrawEmptyDownloadHint and return row height. */
unsigned LayoutEmptyDownloadRow(TextRowRenderer &renderer) noexcept;

/** Empty placeholder for two-row lists (None / Press here to download). */
void DrawEmptyDownloadHint(TwoTextRowsRenderer &renderer, Canvas &canvas,
                           PixelRect rc) noexcept;

/** Lay out @p renderer for #DrawEmptyDownloadHint and return row height. */
unsigned LayoutEmptyDownloadRow(TwoTextRowsRenderer &renderer) noexcept;
