// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Icon.hpp"
#include "util/Serial.hpp"
#include "Geo/GeoBounds.hpp"

#ifdef ENABLE_OPENGL
#include "ui/opengl/System.hpp"
#else
#include "ui/canvas/Brush.hpp"
#include "Topography/ShapeRenderer.hpp"
#endif

#include <memory>
#include <vector>

class TopographyFile;
class Canvas;
class GLArrayBuffer;
template<GLenum target, GLenum usage> class GLBuffer;
class WindowProjection;
class LabelBlock;
class XShape;
struct GeoPoint;
struct TopographyLook;

/**
 * Class used to manage and render vector topography layers
 */
class TopographyFileRenderer final
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

  std::vector<GeoPoint> visible_points;

#ifdef ENABLE_OPENGL
  std::unique_ptr<GLArrayBuffer> array_buffer;
  Serial array_buffer_serial;
  
  // Element Array Buffer for batched polygon indices
  // This allows efficient rendering without MultiDrawElements
  std::unique_ptr<GLBuffer<GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW>> element_buffer;
  size_t element_buffer_size = 0;
#endif

public:
  TopographyFileRenderer(const TopographyFile &file,
                         const TopographyLook &look) noexcept;

  TopographyFileRenderer(const TopographyFileRenderer &) = delete;

  ~TopographyFileRenderer() noexcept;

  /**
   * Paints the polygons, lines and points/icons in the TopographyFile
   * @param canvas The canvas to paint on
   * @param bitmap_canvas Temporary canvas for the icon
   * @param projection
   */
  void Paint(Canvas &canvas, const WindowProjection &projection) noexcept;

  /**
   * Paints a topography label if the space is available in the LabelBlock
   * @param canvas The canvas to paint on
   * @param projection
   * @param label_block The LabelBlock class to use for decluttering
   * @param settings_map
   */
  void PaintLabels(Canvas &canvas, const WindowProjection &projection,
                   LabelBlock &label_block) noexcept;

private:
  void UpdateVisibleShapes(const WindowProjection &projection) noexcept;

#ifdef ENABLE_OPENGL
  void UpdateArrayBuffer() noexcept;
#endif

  void PaintPoints(Canvas &canvas, const WindowProjection &projection) noexcept;
};
