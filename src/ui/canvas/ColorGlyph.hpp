// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Bitmap;

/**
 * Render one character in the colors of its font, e.g. an emoji.
 *
 * The text stack of XCSoar draws glyphs as a coverage mask which is
 * then filled with the text color; a color glyph cannot be drawn that
 * way.  This function goes around it and returns an image instead,
 * which the caller draws like any other bitmap.
 *
 * @param text the character, UTF-8
 * @param size the width and the height of the image in pixels
 * @param bitmap receives the image
 * @return true if @p text has a color glyph and the bitmap has been
 * created; false on a platform whose fonts have no color glyphs, or
 * for a character which has none
 */
bool
RenderColorGlyph(const char *text, unsigned size, Bitmap &bitmap) noexcept;
