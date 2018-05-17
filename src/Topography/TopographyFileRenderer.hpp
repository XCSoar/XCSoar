/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef TOPOGRAPHY_FILE_RENDERER_HPP
#define TOPOGRAPHY_FILE_RENDERER_HPP

#include "Screen/Pen.hpp"
#include "Screen/Icon.hpp"
#include "Util/Serial.hpp"
#include "Geo/GeoBounds.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Surface.hpp"
#else
#include "Screen/Brush.hpp"
#include "Topography/ShapeRenderer.hpp"
#endif

#include <vector>

class TopographyFile;
class Canvas;
class GLFallbackArrayBuffer;
class WindowProjection;
class LabelBlock;
class XShape;
struct GeoPoint;
struct TopographyLook;

/**
 * Class used to manage and render vector topography layers
 */
class TopographyFileRenderer final
#ifdef ENABLE_OPENGL
  : GLSurfaceListener
#endif
{
  const TopographyFile &file;

  const TopographyLook &look;

#ifndef ENABLE_OPENGL
  mutable ShapeRenderer shape_renderer;
#endif

  Pen pen;

#ifndef ENABLE_OPENGL
  Brush brush;
#endif

  MaskedIcon icon;

  Serial visible_serial;
  GeoBounds visible_bounds;

  std::vector<const XShape *> visible_shapes, visible_labels;

#ifdef ENABLE_OPENGL
  GLFallbackArrayBuffer *array_buffer;
  Serial array_buffer_serial;
#endif

public:
  TopographyFileRenderer(const TopographyFile &file,
                         const TopographyLook &look);

  TopographyFileRenderer(const TopographyFileRenderer &) = delete;

  ~TopographyFileRenderer();

  /**
   * Paints the polygons, lines and points/icons in the TopographyFile
   * @param canvas The canvas to paint on
   * @param bitmap_canvas Temporary canvas for the icon
   * @param projection
   */
  void Paint(Canvas &canvas, const WindowProjection &projection);

  /**
   * Paints a topography label if the space is available in the LabelBlock
   * @param canvas The canvas to paint on
   * @param projection
   * @param label_block The LabelBlock class to use for decluttering
   * @param settings_map
   */
  void PaintLabels(Canvas &canvas,
                   const WindowProjection &projection, LabelBlock &label_block);

private:
  void UpdateVisibleShapes(const WindowProjection &projection);

#ifdef ENABLE_OPENGL
  void UpdateArrayBuffer();

  void PaintPoint(Canvas &canvas, const WindowProjection &projection,
                  const XShape &shape, const float *opengl_matrix) const;

  virtual void SurfaceCreated() override;
  virtual void SurfaceDestroyed() override;
#else
  void PaintPoint(Canvas &canvas, const WindowProjection &projection,
                  const unsigned short *lines, const unsigned short *end_lines,
                  const GeoPoint *points) const;
#endif
};

#endif
