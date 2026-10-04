// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelSize;
class RawBitmap;
class GeoBounds;
class Projection;

#ifdef ENABLE_OPENGL

/**
 * Draw an opaque georeferenced bitmap to the current OpenGL context.
 *
 * @param bitmap_size use this size instead of #GLTexture::GetSize()
 * @param bounds #RawBitmap's geo reference
 * @param project a projection used to translate GeoPoints to screen
 * coordinates
 */
void
DrawGeoBitmap(const RawBitmap &bitmap, PixelSize bitmap_size,
              const GeoBounds &bounds,
              const Projection &projection);

/**
 * Draw a latitude/longitude rectangle as a triangle strip.
 *
 * A single quad is wider toward the equator under the cosine
 * projection.  Its diagonal then samples a longitude that drifts
 * as the view changes size.  Strips follow constant latitude, which
 * is linear on the screen.
 *
 * @param u1 texture coordinate of the east edge
 * @param v1 texture coordinate of the south edge
 */
void
DrawGeoQuad(const GeoBounds &bounds, const Projection &projection,
            float u1, float v1) noexcept;

#endif
