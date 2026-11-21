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
 * Draw a lat/lon rectangle as a projected mesh.  A 4-corner quad
 * warps against airspace at far zoom.
 */
void
DrawGeoMesh(const GeoBounds &bounds, const Projection &projection,
            float u0, float v0, float u1, float v1);

#endif
