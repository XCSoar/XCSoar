// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyFileRenderer.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Look/TopographyLook.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "Screen/Layout.hpp"
#include "shapelib/mapserver.h"
#include "util/AllocatedArray.hxx"
#include "util/tstring.hpp"
#include "Geo/GeoClip.hpp"
#include "Geo/FAISphere.hpp"
#include "LogFile.hpp"

#include <cstddef>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/VertexPointer.hpp"
#include "ui/canvas/opengl/Buffer.hpp"
#include "ui/canvas/opengl/Dynamic.hpp"
#include "ui/canvas/opengl/Geo.hpp"

#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/Shaders.hpp"

#include <glm/gtc/type_ptr.hpp>
#endif

#include <algorithm>
#include <numeric>
#include <set>

TopographyFileRenderer::TopographyFileRenderer(const TopographyFile &_file,
                                               const TopographyLook &_look) noexcept
  :file(_file), look(_look),
   pen(Layout::ScaleFinePenWidth(file.GetPenWidth()), Color{file.GetColor()})
#ifndef ENABLE_OPENGL
  , brush(Color{file.GetColor()})
#endif
{
  ResourceId icon_ID = file.GetIcon();
  if (icon_ID.IsDefined())
    icon.LoadResource(icon_ID, file.GetBigIcon(), file.GetUltraIcon());
}

TopographyFileRenderer::~TopographyFileRenderer() noexcept = default;

void
TopographyFileRenderer::UpdateVisibleShapes(const WindowProjection &projection) noexcept
{
  if (file.GetSerial() == visible_serial &&
      visible_bounds.IsInside(projection.GetScreenBounds()) &&
      projection.GetScreenBounds().Scale(2).IsInside(visible_bounds))
    /* cache is clean */
    return;

  visible_serial = file.GetSerial();
  visible_bounds = projection.GetScreenBounds().Scale(1.2);
  visible_shapes.clear();
  visible_points.clear();
  visible_labels.clear();

  for (const XShape &shape : file) {
    if (!visible_bounds.Overlaps(shape.get_bounds()))
      continue;

    if (shape.get_type() != MS_SHAPE_NULL) {
      if (shape.get_type() == MS_SHAPE_POINT) {
        if (icon.IsDefined()) {
          const auto *points = shape.GetPoints();
          for (const unsigned line_size : shape.GetLines()) {
            const auto *end = points + line_size;
            for (; points < end; ++points) {
#ifdef ENABLE_OPENGL
              visible_points.push_back(file.ToGeoPoint(*points));
#else
              visible_points.push_back(*points);
#endif
            }
          }
        }
      } else
        visible_shapes.push_back(&shape);
    }

    if (shape.GetLabel() != nullptr)
      visible_labels.push_back(&shape);
  }
}

#ifdef ENABLE_OPENGL

inline void
TopographyFileRenderer::UpdateArrayBuffer() noexcept
{
  if (array_buffer == nullptr)
    array_buffer = std::make_unique<GLArrayBuffer>();
  else if (file.GetSerial() == array_buffer_serial)
    return;

  // Unbind any currently bound buffer before rebuilding to prevent
  // the OpenGL driver from accessing invalid buffer data during the update
  GLArrayBuffer::Unbind();

  array_buffer_serial = file.GetSerial();

  unsigned n = 0;
  for (auto &shape : file) {
    shape.SetOffset(n);
    const auto lines = shape.GetLines();
    const unsigned shape_points = std::accumulate(lines.begin(), lines.end(), 0u);
    n += shape_points;
  }

  if (n == 0)
    return;

  ShapePoint *p = (ShapePoint *)
    array_buffer->BeginWrite(n * sizeof(*p));
  assert (p != nullptr);

  for (const auto &shape : file) {
    const auto lines = shape.GetLines();
    const ShapePoint *src = shape.GetPoints();
    for (const auto n_points : lines) {
      p = std::copy_n(src, n_points, p);
      src += n_points;
    }
  }

  array_buffer->CommitWrite(n * sizeof(*p), p - n);
  
  // Ensure the buffer update is fully committed before any rendering can use it.
  // This prevents race conditions where rendering might access the buffer
  // while it's still being updated. The CommitWrite already unbinds, but we
  // need to ensure the GPU has finished processing the update.
  // Note: glFinish() is expensive but necessary to prevent crashes on some
  // OpenGL drivers (particularly MediaTek) that have race conditions.
  if (n > 0 && array_buffer != nullptr) {
    glFinish();
    // Re-bind the buffer after glFinish() to ensure it's ready for rendering
    array_buffer->Bind();
    GLArrayBuffer::Unbind();
  }
}

#endif

inline void
TopographyFileRenderer::PaintPoints(Canvas &canvas,
                                    const WindowProjection &projection) noexcept
{
  for (const auto &point : visible_points) {
    if (auto p = projection.GeoToScreenIfVisible(point))
      icon.Draw(canvas, *p);
  }
}

void
TopographyFileRenderer::Paint(Canvas &canvas,
                              const WindowProjection &projection) noexcept
{
  const std::lock_guard lock{file.mutex};

  const auto map_scale = projection.GetMapScale();
  if (!file.IsVisible(map_scale))
    return;

#ifdef ENABLE_OPENGL
  // Skip all processing if scale is too extreme to prevent performance issues.
  // When zoomed out very far, UpdateVisibleShapes() would process too many
  // shapes because the visible bounds become huge, causing severe slowdowns.
  const auto scale_r = projection.GetScale() * FAISphere::REARTH;
  constexpr double MAX_SAFE_SCALE_R = 1000000.0;  // Match ToGLM() limit
  if (scale_r > MAX_SAFE_SCALE_R)
    return;  // Skip all processing when zoomed out too far
#endif

  UpdateVisibleShapes(projection);
  PaintPoints(canvas, projection);

  if (visible_shapes.empty())
    return;

#ifdef ENABLE_OPENGL
  UpdateArrayBuffer();
  
  if (array_buffer == nullptr) {
    LogFormat("Topography: ERROR - array_buffer is null after UpdateArrayBuffer");
    return;
  }
  
  // Ensure shader is valid before use
  if (OpenGL::solid_shader == nullptr) {
    LogFormat("Topography: ERROR - solid_shader is null");
    return;
  }
  
  // Ensure clean OpenGL state before rendering. Terrain rendering (DrawGeoBitmap)
  // uses texture_shader and sets up vertex attributes that might interfere with
  // topography's VBO-based rendering. Explicitly switch to solid_shader and
  // ensure vertex attribute arrays are in a clean state.
  OpenGL::solid_shader->Use();
  
  // Disable any vertex attribute arrays that might have been left enabled
  // by previous rendering (e.g., TEXCOORD from terrain texture rendering)
  glDisableVertexAttribArray(OpenGL::Attribute::TEXCOORD);
  
  // Unbind any previously bound VBO to ensure clean state
  GLArrayBuffer::Unbind();
  
  // Re-bind buffer to ensure it's valid after potential updates
  array_buffer->Bind();
  
  // Verify buffer is still valid after binding
  if (array_buffer == nullptr) {
    LogFormat("Topography: ERROR - array_buffer became null after Bind");
    return;
  }

  pen.Bind();

  if (!pen.GetColor().IsOpaque()) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
#else
  shape_renderer.Configure(&pen, &brush);
#endif

  // get drawing info

#ifdef ENABLE_OPENGL
  const unsigned level = file.GetThinningLevel(map_scale);
  const ShapeScalar min_distance =
    ShapeScalar(file.GetMinimumPointDistance(level))
    / (Layout::Scale(1) * FAISphere::REARTH);

  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(ToGLM(projection, file.GetCenter())));
#else // !ENABLE_OPENGL
  const GeoClip clip(projection.GetScreenBounds().Scale(1.1));
  AllocatedArray<GeoPoint> geo_points;

  const unsigned iskip = file.GetSkipSteps(map_scale);
#endif

#ifdef ENABLE_OPENGL
  ScopeVertexPointer vp;
  // When using VBOs, glVertexAttribPointer interprets the pointer as a byte offset
  // So we use nullptr as base and add the byte offset
  const ShapePoint *const buffer = nullptr;

#ifdef GL_EXT_multi_draw_arrays
  std::vector<GLsizei> polygon_counts;
  std::vector<GLushort> polygon_indices;
#endif
#endif

  for (const XShape *shape_p : visible_shapes) {
    const XShape &shape = *shape_p;

    const auto lines = shape.GetLines();
#ifdef ENABLE_OPENGL
    // When using VBOs, glVertexAttribPointer interprets the pointer as a byte offset
    // Calculate the byte offset: shape.GetOffset() is in ShapePoint units
    const unsigned offset = shape.GetOffset();
    const std::size_t byte_offset = offset * sizeof(ShapePoint);
    const ShapePoint *points = (const ShapePoint *)(std::size_t)byte_offset;
#else // !ENABLE_OPENGL
    const GeoPoint *points = shape.GetPoints();
#endif

    switch (shape.get_type()) {
    case MS_SHAPE_NULL:
    case MS_SHAPE_POINT:
      break;

    case MS_SHAPE_LINE:
      {
#ifdef ENABLE_OPENGL
        vp.Update(GL_FLOAT, points);

        XShape::Indices indices;
        if (level == 0 ||
            (indices = shape.GetIndices(level, min_distance)).indices == nullptr) {
          // Validate VBO bounds for glDrawArrays
          unsigned total_vbo_points = 0;
          for (const auto &s : file) {
            const auto ls = s.GetLines();
            total_vbo_points += std::accumulate(ls.begin(), ls.end(), 0u);
          }
          unsigned draw_offset = 0;
          for (unsigned n : lines) {
            if (draw_offset + n > total_vbo_points) {
              LogFormat("Topography: Invalid LINE draw offset %u + %u > %u", 
                        draw_offset, n, total_vbo_points);
              break;
            }
            // Ensure buffer is ready before drawing
            glFlush();
            glDrawArrays(GL_LINE_STRIP, draw_offset, n);
            draw_offset += n;
          }
        } else {
          if (indices.indices == nullptr) {
            LogFormat("Topography: null indices.indices for LINE");
            break;
          }
          // Validate VBO bounds for glDrawElements
          unsigned total_vbo_points = 0;
          for (const auto &s : file) {
            const auto ls = s.GetLines();
            total_vbo_points += std::accumulate(ls.begin(), ls.end(), 0u);
          }
          for (unsigned n : std::span<const GLushort>{indices.count, lines.size()}) {
            // Check if any index in this draw call is out of bounds
            bool valid = true;
            for (unsigned i = 0; i < n; ++i) {
              if (indices.indices[i] >= total_vbo_points) {
                LogFormat("Topography: Invalid LINE index %u: %u >= %u", 
                          i, indices.indices[i], total_vbo_points);
                valid = false;
                break;
              }
            }
            if (!valid)
              break;
            // Ensure buffer is ready before drawing
            glFlush();
            glDrawElements(GL_LINE_STRIP, n, GL_UNSIGNED_SHORT,
                           indices.indices);
            indices.indices += n;
          }
        }
#else // !ENABLE_OPENGL
        for (unsigned msize : lines) {
        shape_renderer.Begin(msize);

        const GeoPoint *end = points + msize - 1;
        for (; points < end; ++points)
          shape_renderer.AddPointIfDistant(projection.GeoToScreen(*points));

        // make sure we always draw the last point
        shape_renderer.AddPoint(projection.GeoToScreen(*points));

        shape_renderer.FinishPolyline(canvas);
      }
#endif
      }
      break;

    case MS_SHAPE_POLYGON:
#ifdef ENABLE_OPENGL
      {
        const auto triangles = shape.GetIndices(level, min_distance);
        if (triangles.count == nullptr)
          break;
        if (triangles.indices == nullptr) {
          LogFormat("Topography: null triangles.indices");
          break;
        }
        const unsigned n = *triangles.count;

#ifdef GL_EXT_multi_draw_arrays
        const unsigned offset = shape.GetOffset();
        // Use MultiDrawElements optimization on non-MediaTek drivers for best performance.
        // MediaTek drivers crash with MultiDrawElements, so we batch indices and use
        // individual glDrawElements calls instead.
        if (!OpenGL::is_mediatek && GLExt::HaveMultiDrawElements() && offset + n < 0x10000) {
          /* postpone, draw many polygons with a single
             glMultiDrawElements() call */
          polygon_counts.push_back(n);
          const size_t size = polygon_indices.size();
          polygon_indices.resize(size + n, offset);
          if (triangles.indices == nullptr) {
            LogFormat("Topography: null triangles.indices in multi-draw path");
            break;
          }
          for (unsigned i = 0; i < n; ++i)
            polygon_indices[size + i] += triangles.indices[i];
          break;
        } else if (OpenGL::is_mediatek && offset + n < 0x10000) {
          // MediaTek workaround: batch indices but use EBO instead of MultiDrawElements
          /* postpone, batch many polygons together */
          polygon_counts.push_back(n);
          const size_t size = polygon_indices.size();
          polygon_indices.resize(size + n, offset);
          if (triangles.indices == nullptr) {
            LogFormat("Topography: null triangles.indices in batch path");
            break;
          }
          for (unsigned i = 0; i < n; ++i)
            polygon_indices[size + i] += triangles.indices[i];
          break;
        }
#endif

        // Validate indices before drawing
        if (triangles.indices == nullptr) {
          LogFormat("Topography: null triangles.indices in immediate draw path");
          break;
        }
        unsigned total_vbo_points = 0;
        for (const auto &s : file) {
          const auto ls = s.GetLines();
          total_vbo_points += std::accumulate(ls.begin(), ls.end(), 0u);
        }
        for (unsigned i = 0; i < n; ++i) {
          if (triangles.indices[i] >= total_vbo_points) {
            LogFormat("Topography: Invalid polygon index %u: %u >= %u", 
                      i, triangles.indices[i], total_vbo_points);
            break;
          }
        }

        vp.Update(GL_FLOAT, points);
        // Ensure buffer is ready before drawing
        glFlush();
        glDrawElements(GL_TRIANGLE_STRIP, n, GL_UNSIGNED_SHORT,
                       triangles.indices);
      }
#else // !ENABLE_OPENGL
      {
        const GeoPoint *src = &points[0];
        for (const unsigned n : lines) {
          unsigned msize = n / iskip;

          /* copy all polygon points into the geo_points array and
             clip them, to avoid integer overflows (as PixelPoint may
             store only 16 bit integers on some platforms) */

          geo_points.GrowDiscard(msize * 3);
          for (unsigned i = 0; i < msize; ++i)
            geo_points[i] = src[i * iskip];

          msize = clip.ClipPolygon(geo_points.data(),
                                   geo_points.data(), msize);
          if (msize < 3)
            continue;

          shape_renderer.Begin(msize);

          for (unsigned i = 0; i < msize; ++i) {
            GeoPoint g = geo_points[i];
            shape_renderer.AddPointIfDistant(projection.GeoToScreen(g));
          }

          shape_renderer.FinishPolygon(canvas);

          src += n;
        }
      }
#endif
      break;
    }
  }
#ifdef ENABLE_OPENGL

#ifdef GL_EXT_multi_draw_arrays
  if (!polygon_indices.empty()) {
    // Calculate total VBO size for validation
    unsigned total_vbo_points = 0;
    for (const auto &shape : file) {
      const auto lines = shape.GetLines();
      total_vbo_points += std::accumulate(lines.begin(), lines.end(), 0u);
    }

    // Validate all indices are within VBO bounds
    for (size_t idx = 0; idx < polygon_indices.size(); ++idx) {
      if (polygon_indices[idx] >= total_vbo_points) {
        LogFormat("Topography: Invalid index %zu: %u >= %u", 
                  idx, polygon_indices[idx], total_vbo_points);
        return;  // Skip rendering to prevent crash
      }
    }

    vp.Update(GL_FLOAT, buffer);

    if (OpenGL::is_mediatek) {
      // MediaTek workaround: Use Element Array Buffer (EBO) instead of MultiDrawElements.
      // This uploads all indices to GPU memory and uses byte offsets,
      // which avoids the MediaTek MultiDrawElements crash.
      const size_t indices_size = polygon_indices.size() * sizeof(GLushort);
      
      // Create or resize element buffer if needed
      if (element_buffer == nullptr || element_buffer_size < indices_size) {
        if (element_buffer == nullptr)
          element_buffer = std::make_unique<GLBuffer<GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW>>();
        
        element_buffer->Load(indices_size, polygon_indices.data());
        element_buffer_size = indices_size;
      } else {
        // Update existing buffer
        element_buffer->Load(indices_size, polygon_indices.data());
      }
      
      // Bind the element buffer
      element_buffer->Bind();
      
      // Draw each polygon using byte offsets into the EBO
      size_t byte_offset = 0;
      for (auto count : polygon_counts) {
        const size_t required_size = (byte_offset / sizeof(GLushort)) + count;
        if (required_size > polygon_indices.size()) {
          LogFormat("Topography: Batch bounds error offset=%zu count=%u size=%zu", 
                    byte_offset / sizeof(GLushort), count, polygon_indices.size());
          break;
        }
        
        // Ensure buffer is ready before drawing
        glFlush();
        
        // Draw using byte offset into the bound EBO
        glDrawElements(GL_TRIANGLE_STRIP, count, GL_UNSIGNED_SHORT,
                       (const GLvoid *)byte_offset);
        
        byte_offset += count * sizeof(GLushort);
      }
      
      // Unbind element buffer
      GLBuffer<GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW>::Unbind();
    } else {
      // Non-MediaTek: Use MultiDrawElements for best performance
      assert(GLExt::HaveMultiDrawElements());

      std::vector<const GLushort *> polygon_pointers;
      unsigned i = 0;
      for (auto count : polygon_counts) {
        if (i + count > polygon_indices.size()) {
          LogFormat("Topography: Multi-draw bounds error i=%u count=%u size=%zu", 
                    i, count, polygon_indices.size());
          return;
        }
        polygon_pointers.push_back(polygon_indices.data() + i);
        i += count;
      }

      if (i != polygon_indices.size()) {
        LogFormat("Topography: Multi-draw size mismatch i=%u size=%zu", 
                  i, polygon_indices.size());
        return;
      }

      // Ensure buffer is ready before multi-draw
      glFlush();

      GLExt::MultiDrawElements(GL_TRIANGLE_STRIP, polygon_counts.data(),
                               GL_UNSIGNED_SHORT,
                               (const GLvoid **)polygon_pointers.data(),
                               polygon_counts.size());
    }
  }
#endif

  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4(1)));
  if (!pen.GetColor().IsOpaque())
    glDisable(GL_BLEND);

  pen.Unbind();

  array_buffer->Unbind();
#else
  shape_renderer.Commit();
#endif
}

void
TopographyFileRenderer::PaintLabels(Canvas &canvas,
                                    const WindowProjection &projection,
                                    LabelBlock &label_block) noexcept
{
  const std::lock_guard lock{file.mutex};

  const auto map_scale = projection.GetMapScale();
  if (!file.IsVisible(map_scale) || !file.IsLabelVisible(map_scale))
    return;

  UpdateVisibleShapes(projection);

  if (visible_labels.empty())
    return;

  canvas.Select(file.IsLabelImportant(map_scale)
                ? look.important_label_font
                : look.regular_label_font);
  canvas.SetTextColor(file.IsLabelImportant(map_scale) ?
                COLOR_BLACK : COLOR_VERY_DARK_GRAY);
  canvas.SetBackgroundTransparent();

  // get drawing info

  int iskip = file.GetSkipSteps(map_scale);

  std::set<tstring> drawn_labels;

  // Iterate over all shapes in the file
  for (const XShape *shape_p : visible_labels) {
    const XShape &shape = *shape_p;

    // Skip shapes without a label
    const TCHAR *label = shape.GetLabel();
    assert(label != nullptr);

    const auto lines = shape.GetLines();
    const auto *points = shape.GetPoints();

    for (const unsigned n : lines) {
      int minx = canvas.GetWidth();
      int miny = canvas.GetHeight();

      const auto *end = points + n;
      for (; points < end; points += iskip) {
#ifdef ENABLE_OPENGL
        auto pt = projection.GeoToScreen(file.ToGeoPoint(*points));
#else
        auto pt = projection.GeoToScreen(*points);
#endif

        if (pt.x <= minx) {
          minx = pt.x;
          miny = pt.y;
        }
      }

      points = end;

      minx += 2;
      miny += 2;

      PixelSize tsize = canvas.CalcTextSize(label);
      PixelRect brect;
      brect.left = minx;
      brect.right = brect.left + tsize.width;
      brect.top = miny;
      brect.bottom = brect.top + tsize.height;

      if (!label_block.check(brect))
        continue;

      if (!drawn_labels.insert(label).second)
        continue;

      canvas.DrawText({minx, miny}, label);
    }
  }
}
