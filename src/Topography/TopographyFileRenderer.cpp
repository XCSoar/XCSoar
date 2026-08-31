// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyFileRenderer.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Topography/ShapeRenderer.hpp"
#include "Look/TopographyLook.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "Screen/Layout.hpp"
#include "LogFile.hpp"
#include "shapelib/mapserver.h"
#include "util/AllocatedArray.hxx"
#include "Geo/GeoClip.hpp"
#include "Geo/FAISphere.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/VertexPointer.hpp"
#include "ui/canvas/opengl/Buffer.hpp"
#include "ui/canvas/opengl/Dynamic.hpp"
#include "ui/canvas/opengl/Geo.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/opengl/System.hpp"
#include "ui/event/Idle.hpp"
#include "time/PeriodClock.hpp"

#include <glm/gtc/type_ptr.hpp>
#endif

#include <string>
#include <algorithm>
#include <numeric>
#include <set>
#include <vector>
#ifdef ENABLE_OPENGL
#include <chrono>
#include <cstdint>
#include <span>
#endif

#ifdef ENABLE_OPENGL

static constexpr auto TOPO_STATS_PERIOD = std::chrono::seconds(2);

struct TopographyGpuStatsState {
  PeriodClock log_clock;
  unsigned frames = 0;
  unsigned layers = 0;
  unsigned vis_rebuilds = 0, vbo_rebuilds = 0;
  unsigned line_draws = 0, fill_draws = 0, points = 0;
  uint64_t paint_us = 0, paint_max_us = 0;
  uint64_t label_us = 0, label_max_us = 0;
  uint64_t gpu_sync_us = 0;
  unsigned last_sw = 0, last_sh = 0;
  unsigned last_lines = 0, last_fills = 0;
  double last_scale = 0;
  bool last_idle = false;
  bool have_gpu_sync = false;

  void Reset() noexcept {
    frames = layers = 0;
    vis_rebuilds = vbo_rebuilds = 0;
    line_draws = fill_draws = points = 0;
    paint_us = paint_max_us = 0;
    label_us = label_max_us = 0;
    gpu_sync_us = 0;
    have_gpu_sync = false;
  }

  void AddUs(uint64_t us, uint64_t &sum, uint64_t &mx) noexcept {
    sum += us;
    if (us > mx)
      mx = us;
  }

  void Flush() noexcept {
    if (frames == 0)
      return;

    const auto elapsed = log_clock.Elapsed();
    const double sec =
      elapsed.count() > 0
      ? std::chrono::duration<double>(elapsed).count()
      : 2.0;
    const double fps = frames / sec;
    const double paint_avg = (paint_us / 1000.0) / frames;
    const double label_avg =
      (label_us / 1000.0) / std::max(1u, frames);

    LogFmt("OpenGL: Topo {:.1f}s frames={} ({:.0f}/s) idle={} "
           "scale={:.0f}m view={}x{}",
           sec, frames, fps, last_idle ? 1 : 0,
           last_scale, last_sw, last_sh);
    LogFmt("OpenGL: Topo cpu paint avg/max {:.2f}/{:.2f} ms  "
           "labels avg/max {:.2f}/{:.2f} ms  gpu_sync={:.1f}ms",
           paint_avg, paint_max_us / 1000.0,
           label_avg, label_max_us / 1000.0,
           have_gpu_sync ? gpu_sync_us / 1000.0 : -1.0);
    LogFmt("OpenGL: Topo layers/frame={:.1f} line_draws={} "
           "(avg {:.0f}) fill_draws={} (avg {:.1f}) points={}  "
           "vis_rebuild={} vbo_rebuild={}",
           layers / double(frames),
           line_draws, line_draws / double(frames),
           fill_draws, fill_draws / double(frames),
           points, vis_rebuilds, vbo_rebuilds);
    LogFmt("OpenGL: Topo last frame lines={} fills={}",
           last_lines, last_fills);

    Reset();
    log_clock.Update();
  }
};

static TopographyGpuStatsState topo_stats;
static uint64_t topo_frame_paint_us;
static unsigned topo_frame_lines, topo_frame_fills;

static uint64_t
TopoSteadyUsSince(std::chrono::steady_clock::time_point t0) noexcept
{
  return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0)
                    .count());
}

void
TopographyGpuStatsBeginDraw() noexcept
{
  topo_frame_paint_us = 0;
  topo_frame_lines = 0;
  topo_frame_fills = 0;
}

static void
TopoAddLayer(unsigned cpu_us, unsigned line_draws, unsigned fill_draws,
             unsigned point_n, bool vis_rebuild, bool vbo_rebuild) noexcept
{
  topo_stats.layers++;
  topo_frame_paint_us += cpu_us;
  topo_stats.line_draws += line_draws;
  topo_stats.fill_draws += fill_draws;
  topo_frame_lines += line_draws;
  topo_frame_fills += fill_draws;
  topo_stats.points += point_n;
  if (vis_rebuild)
    topo_stats.vis_rebuilds++;
  if (vbo_rebuild)
    topo_stats.vbo_rebuilds++;
}

void
TopographyGpuStatsEndDraw(const WindowProjection &projection) noexcept
{
  topo_stats.frames++;
  topo_stats.AddUs(topo_frame_paint_us, topo_stats.paint_us,
                   topo_stats.paint_max_us);
  topo_stats.last_lines = topo_frame_lines;
  topo_stats.last_fills = topo_frame_fills;
  topo_stats.last_scale = projection.GetMapScale();
  const auto view = projection.GetScreenSize();
  topo_stats.last_sw = view.width;
  topo_stats.last_sh = view.height;
  topo_stats.last_idle = IsUserIdle(750);

  if (!topo_stats.log_clock.IsDefined())
    topo_stats.log_clock.Update();
  else if (topo_stats.log_clock.Check(TOPO_STATS_PERIOD)) {
    const auto g0 = std::chrono::steady_clock::now();
    glFinish();
    topo_stats.gpu_sync_us = TopoSteadyUsSince(g0);
    topo_stats.have_gpu_sync = true;
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR)
      LogFmt("OpenGL: Topo glGetError=0x{:x}", unsigned(err));
    topo_stats.Flush();
  }
}

void
TopographyGpuStatsAddLabels(unsigned cpu_us) noexcept
{
  topo_stats.AddUs(cpu_us, topo_stats.label_us, topo_stats.label_max_us);
}

#endif

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

/**
 * True if a feature's geographic box is smaller than
 * #SHAPE_MIN_BBOX_PX on screen.  Cheap (angle spans only).
 */
[[gnu::pure]]
static bool
ShapeTooSmall(const GeoBounds &bounds, Angle min_span) noexcept
{
  return bounds.GetWidth() < min_span && bounds.GetHeight() < min_span;
}

[[gnu::pure]]
static bool
ShapeTooSmallToDraw(const XShape &shape, Angle min_span) noexcept
{
  /* Lines: never skip.  At 120 km, 1 px is ~150 m; OSM road sticks
     shorter than that would leave a gapped network.  Polygons: skip
     sub-pixel fills that would still cost ear-clip. */
  return shape.get_type() == MS_SHAPE_POLYGON &&
    ShapeTooSmall(shape.get_bounds(), min_span);
}

#ifdef ENABLE_OPENGL

/** GLES2 indices are 16-bit; batch shapes that share a 64k vertex window. */
static constexpr unsigned GLUSHORT_WINDOW = 0x10000;

[[gnu::pure]]
static unsigned
CountLineVertices(std::span<const uint16_t> lines) noexcept
{
  unsigned n = 0;
  for (const unsigned nv : lines)
    n += nv;
  return n;
}

[[gnu::pure]]
static constexpr bool
FitsGLushortWindow(unsigned offset, unsigned n_verts) noexcept
{
  const unsigned local = offset % GLUSHORT_WINDOW;
  return n_verts <= GLUSHORT_WINDOW - local;
}

static constexpr unsigned
GLushortWindowBase(unsigned offset) noexcept
{
  return offset - offset % GLUSHORT_WINDOW;
}

static void
AppendOffsetStrip(std::vector<GLsizei> &counts,
                  std::vector<GLushort> &indices,
                  unsigned base, unsigned n) noexcept
{
  if (n < 2)
    return;

  counts.push_back(GLsizei(n));
  const size_t size = indices.size();
  indices.resize(size + n);
  for (unsigned i = 0; i < n; ++i)
    indices[size + i] = GLushort(base + i);
}

static void
AppendIndexedStrip(std::vector<GLsizei> &counts,
                   std::vector<GLushort> &indices,
                   unsigned offset,
                   const GLushort *src, unsigned n) noexcept
{
  if (n < 2)
    return;

  counts.push_back(GLsizei(n));
  const size_t size = indices.size();
  indices.resize(size + n, GLushort(offset));
  for (unsigned i = 0; i < n; ++i)
    indices[size + i] += src[i];
}

/**
 * Expand a triangle strip to independent triangles, skipping
 * degenerates used as strip restarts.  One glDrawElements then
 * covers many polygons when MultiDrawElements is unavailable.
 */
static void
AppendStripAsTriangles(std::vector<uint16_t> &triangles,
                       const GLushort *strip, unsigned n) noexcept
{
  if (n < 3)
    return;

  for (unsigned i = 0; i + 2 < n; ++i) {
    const GLushort a = strip[i];
    const GLushort b = strip[i + 1];
    const GLushort c = strip[i + 2];
    if (a == b || b == c || a == c)
      continue;

    if (i & 1) {
      triangles.push_back(b);
      triangles.push_back(a);
      triangles.push_back(c);
    } else {
      triangles.push_back(a);
      triangles.push_back(b);
      triangles.push_back(c);
    }
  }
}

/**
 * Expand a line strip to independent segments.  Used when the
 * driver cannot MultiDraw (PowerVR GE8300 SIGSEGV in
 * glMultiDrawElementsEXT).  Core glDrawElements(GL_LINES) is the
 * same path as the fill triangle list that already runs there.
 */
static void
AppendStripAsLines(std::vector<uint16_t> &segments,
                   const GLushort *strip, unsigned n) noexcept
{
  if (n < 2)
    return;

  for (unsigned i = 0; i + 1 < n; ++i) {
    const GLushort a = strip[i];
    const GLushort b = strip[i + 1];
    if (a == b)
      continue;

    segments.push_back(a);
    segments.push_back(b);
  }
}

static void
ExpandLines(std::vector<uint16_t> &segments,
            const std::vector<GLsizei> &counts,
            const std::vector<GLushort> &indices) noexcept
{
  unsigned i = 0;
  for (auto count : counts) {
    AppendStripAsLines(segments, indices.data() + i, count);
    i += count;
  }
}

static void
ExpandFills(std::vector<uint16_t> &triangles,
            const std::vector<GLsizei> &counts,
            const std::vector<GLushort> &indices) noexcept
{
  unsigned i = 0;
  for (auto count : counts) {
    AppendStripAsTriangles(triangles, indices.data() + i, count);
    i += count;
  }
}

static void
DrawCachedWindow(ScopeVertexPointer &vp, const ShapePoint *buffer,
                 const TopographyFileRenderer::CachedWindow &w,
                 unsigned &line_draws, unsigned &fill_draws) noexcept
{
  vp.Update(GL_FLOAT, buffer + w.window_base);

#ifdef GL_EXT_multi_draw_arrays
  if (!w.line_counts.empty() && GLExt::HaveMultiDrawElements()) {
    std::vector<const GLushort *> pointers;
    unsigned i = 0;
    for (auto count : w.line_counts) {
      pointers.push_back(w.lines.data() + i);
      i += unsigned(count);
    }
    GLExt::MultiDrawElements(GL_LINE_STRIP, w.line_counts.data(),
                             GL_UNSIGNED_SHORT,
                             (const GLvoid **)pointers.data(),
                             w.line_counts.size());
    ++line_draws;
  } else
#endif
  if (!w.lines.empty()) {
    glDrawElements(GL_LINES, GLsizei(w.lines.size()),
                   GL_UNSIGNED_SHORT, w.lines.data());
    ++line_draws;
  }

#ifdef GL_EXT_multi_draw_arrays
  if (!w.fill_counts.empty() && GLExt::HaveMultiDrawElements()) {
    std::vector<const GLushort *> pointers;
    unsigned i = 0;
    for (auto count : w.fill_counts) {
      pointers.push_back(w.fills.data() + i);
      i += unsigned(count);
    }
    GLExt::MultiDrawElements(GL_TRIANGLE_STRIP, w.fill_counts.data(),
                             GL_UNSIGNED_SHORT,
                             (const GLvoid **)pointers.data(),
                             w.fill_counts.size());
    ++fill_draws;
  } else
#endif
  if (!w.fills.empty()) {
    glDrawElements(GL_TRIANGLES, GLsizei(w.fills.size()),
                   GL_UNSIGNED_SHORT, w.fills.data());
    ++fill_draws;
  }
}

struct TopoShapeBatch {
  std::vector<GLsizei> line_counts;
  std::vector<GLushort> line_indices;
  std::vector<GLsizei> polygon_counts;
  std::vector<GLushort> polygon_indices;
  unsigned window_base = 0;
  bool active = false;

  void FlushTo(std::vector<TopographyFileRenderer::CachedWindow> &out) noexcept {
    if (!active)
      return;

    TopographyFileRenderer::CachedWindow w;
    w.window_base = window_base;

#ifdef GL_EXT_multi_draw_arrays
    if (GLExt::HaveMultiDrawElements()) {
      w.line_counts.assign(line_counts.begin(), line_counts.end());
      w.lines.assign(line_indices.begin(), line_indices.end());
      w.fill_counts.assign(polygon_counts.begin(), polygon_counts.end());
      w.fills.assign(polygon_indices.begin(), polygon_indices.end());
    } else
#endif
    {
      w.lines.reserve(line_indices.size() * 2);
      w.fills.reserve(polygon_indices.size() * 3);
      ExpandLines(w.lines, line_counts, line_indices);
      ExpandFills(w.fills, polygon_counts, polygon_indices);
    }

    if (!w.lines.empty() || !w.fills.empty() ||
        !w.line_counts.empty() || !w.fill_counts.empty())
      out.push_back(std::move(w));

    line_counts.clear();
    line_indices.clear();
    polygon_counts.clear();
    polygon_indices.clear();
    active = false;
  }

  void EnsureWindow(unsigned window,
                    std::vector<TopographyFileRenderer::CachedWindow> &out) noexcept {
    if (active && window != window_base)
      FlushTo(out);

    window_base = window;
    active = true;
  }
};

#endif

bool
TopographyFileRenderer::UpdateVisibleShapes(const WindowProjection &projection) noexcept
{
  const double scale = projection.GetScale();
  const GeoBounds screen = projection.GetScreenBounds();
  if (file.GetSerial() == visible_serial &&
      scale <= visible_scale &&
      visible_bounds.IsValid() &&
      visible_bounds.IsInside(screen))
    /* still inside the last 2× viewport; pan only reprojects */
    return false;

  visible_serial = file.GetSerial();
  visible_scale = scale;
  visible_bounds = screen.Scale(TopographyFile::CACHE_BOUNDS_SCALE);
  visible_shapes.clear();
  visible_points.clear();
  visible_labels.clear();

  const Angle min_span =
    projection.PixelsToAngle(SHAPE_MIN_BBOX_PX);

  for (const XShape &shape : file) {
    if (!visible_bounds.Overlaps(shape.get_bounds()))
      continue;

    const bool too_small = ShapeTooSmallToDraw(shape, min_span);

    if (shape.get_type() != MS_SHAPE_NULL && !too_small) {
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

    if (shape.GetLabel() != nullptr && !too_small)
      visible_labels.push_back(&shape);
  }

  return true;
}

#ifdef ENABLE_OPENGL

inline bool
TopographyFileRenderer::UpdateArrayBuffer() noexcept
{
  if (array_buffer == nullptr)
    array_buffer = std::make_unique<GLArrayBuffer>();
  else if (file.GetSerial() == array_buffer_serial)
    return true;

  unsigned n = 0;
  for (auto &shape : file) {
    shape.SetOffset(n);

    const auto lines = shape.GetLines();
    n = std::accumulate(lines.begin(), lines.end(), n);
  }

  if (n == 0)
    return false;

  ShapePoint *p = (ShapePoint *)
    array_buffer->BeginWrite(n * sizeof(*p));
  if (p == nullptr) {
    LogFmt("Topography: {} failed to allocate {} vertices",
           file.GetName(), n);
    GLArrayBuffer::Unbind();
    return false;
  }

  for (const auto &shape : file) {
    const auto lines = shape.GetLines();
    const ShapePoint *src = shape.GetPoints();
    for (const auto n_points : lines) {
      p = std::copy_n(src, n_points, p);
      src += n_points;
    }
  }

  array_buffer->CommitWrite(n * sizeof(*p), p - n);
  array_buffer_serial = file.GetSerial();
  return true;
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
  const auto t0 = std::chrono::steady_clock::now();
  unsigned line_draws = 0, fill_draws = 0;
#endif
  const bool vis_rebuild = UpdateVisibleShapes(projection);
  PaintPoints(canvas, projection);

  if (visible_shapes.empty()) {
#ifdef ENABLE_OPENGL
    draw_cache_valid = false;
    TopoAddLayer(TopoSteadyUsSince(t0), 0, 0,
                 unsigned(visible_points.size()), vis_rebuild, false);
#endif
    return;
  }

#ifdef ENABLE_OPENGL
  OpenGL::solid_shader->Use();

  const bool vbo_rebuild =
    array_buffer == nullptr || file.GetSerial() != array_buffer_serial;
  if (!UpdateArrayBuffer()) {
    TopoAddLayer(TopoSteadyUsSince(t0), 0, 0,
                 unsigned(visible_points.size()), vis_rebuild, vbo_rebuild);
    return;
  }
  array_buffer->Bind();
  const ShapePoint *const buffer = nullptr;

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

  const bool cache_ok = draw_cache_valid &&
    !vis_rebuild && !vbo_rebuild &&
    draw_cache_thinning == level;

  if (cache_ok) {
    for (const auto &w : draw_windows)
      DrawCachedWindow(vp, buffer, w, line_draws, fill_draws);
  } else {
    draw_windows.clear();
    TopoShapeBatch batch;
    bool index_cache_complete = true;
#endif

  const Angle min_span =
    projection.PixelsToAngle(SHAPE_MIN_BBOX_PX);

  for (const XShape *shape_p : visible_shapes) {
    const XShape &shape = *shape_p;

    if (ShapeTooSmallToDraw(shape, min_span))
      continue;

    const auto lines = shape.GetLines();
#ifdef ENABLE_OPENGL
    const ShapePoint *points = buffer + shape.GetOffset();
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
        const unsigned offset = shape.GetOffset();
        const unsigned n_verts = CountLineVertices(lines);
        XShape::Indices indices{};
        const bool have_thin =
          level != 0 &&
          (indices = shape.GetIndices(level, min_distance)).indices != nullptr;

        if (FitsGLushortWindow(offset, n_verts)) {
          /* postpone: one draw per 64k-vertex window */
          const unsigned window = GLushortWindowBase(offset);
          batch.EnsureWindow(window, draw_windows);
          const unsigned local_base = offset - window;
          if (!have_thin) {
            unsigned local = 0;
            for (unsigned n : lines) {
              AppendOffsetStrip(batch.line_counts, batch.line_indices,
                                local_base + local, n);
              local += n;
            }
          } else {
            for (unsigned n : std::span<const GLushort>{
                   indices.count, lines.size()}) {
              AppendIndexedStrip(batch.line_counts, batch.line_indices,
                                 local_base, indices.indices, n);
              indices.indices += n;
            }
          }
          break;
        }

        if (n_verts <= GLUSHORT_WINDOW) {
          /* Shape spans a 64k VBO window; address it from its own
             base so indices stay 16-bit. */
          batch.EnsureWindow(offset, draw_windows);
          if (!have_thin) {
            unsigned local = 0;
            for (unsigned n : lines) {
              AppendOffsetStrip(batch.line_counts, batch.line_indices,
                                local, n);
              local += n;
            }
          } else {
            for (unsigned n : std::span<const GLushort>{
                   indices.count, lines.size()}) {
              AppendIndexedStrip(batch.line_counts, batch.line_indices,
                                 0, indices.indices, n);
              indices.indices += n;
            }
          }
          break;
        }

        vp.Update(GL_FLOAT, points);

        if (!have_thin) {
          unsigned local = 0;
          for (unsigned n : lines) {
            glDrawArrays(GL_LINE_STRIP, local, n);
            ++line_draws;
            local += n;
          }
        } else {
          for (unsigned n : std::span<const GLushort>{
                 indices.count, lines.size()}) {
            glDrawElements(GL_LINE_STRIP, n, GL_UNSIGNED_SHORT,
                           indices.indices);
            ++line_draws;
            indices.indices += n;
          }
        }
        index_cache_complete = false;
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
        if (triangles.indices == nullptr || triangles.count == nullptr ||
            *triangles.count == 0)
          break;

        const unsigned n = *triangles.count;

        const unsigned offset = shape.GetOffset();
        const unsigned n_verts = CountLineVertices(lines);
        /* GLushort indices relative to the 64k window that
           contains this shape.  A strip that crosses a window
           is drawn unbatched so indices cannot wrap. */
        if (FitsGLushortWindow(offset, n_verts)) {
          const unsigned window = GLushortWindowBase(offset);
          batch.EnsureWindow(window, draw_windows);
          const unsigned local_base = offset - window;
          batch.polygon_counts.push_back(n);
          const size_t size = batch.polygon_indices.size();
          batch.polygon_indices.resize(size + n, local_base);
          for (unsigned i = 0; i < n; ++i)
            batch.polygon_indices[size + i] += triangles.indices[i];
          break;
        }

        if (n_verts <= GLUSHORT_WINDOW) {
          batch.EnsureWindow(offset, draw_windows);
          batch.polygon_counts.push_back(n);
          const size_t size = batch.polygon_indices.size();
          batch.polygon_indices.resize(size + n);
          for (unsigned i = 0; i < n; ++i)
            batch.polygon_indices[size + i] = triangles.indices[i];
          break;
        }

        vp.Update(GL_FLOAT, points);
        glDrawElements(GL_TRIANGLE_STRIP, n, GL_UNSIGNED_SHORT,
                       triangles.indices);
        ++fill_draws;
        index_cache_complete = false;
      }
#else // !ENABLE_OPENGL
      {
        const GeoPoint *src = &points[0];
        for (const unsigned n : lines) {
          unsigned msize = n / iskip;

          /* copy all polygon points into the geo_points array and
             clip them, to avoid integer overflows (as PixelPoint may
             store only 16 bit integers on some platforms) */

          geo_points.GrowDiscard(msize * 4);
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

    batch.FlushTo(draw_windows);
    for (const auto &w : draw_windows)
      DrawCachedWindow(vp, buffer, w, line_draws, fill_draws);
    draw_cache_valid = index_cache_complete;
    draw_cache_thinning = level;
  }

  glUniformMatrix4fv(OpenGL::solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4(1)));
  if (!pen.GetColor().IsOpaque())
    glDisable(GL_BLEND);

  pen.Unbind();

  array_buffer->Unbind();

  TopoAddLayer(TopoSteadyUsSince(t0), line_draws, fill_draws,
               unsigned(visible_points.size()), vis_rebuild, vbo_rebuild);
#else
  shape_renderer.Commit();
#endif
}

/**
 * Map scale (metres) at which labels use the largest font (circuit).
 */
static constexpr double LABEL_LARGE_SCALE = 2000;

/**
 * Fraction of the layer's label range at which labels step up to the
 * medium font (well inside the range, not just as they appear).
 */
static constexpr double LABEL_MEDIUM_RANGE_FRACTION = 0.25;

[[gnu::pure]]
static TopographyLook::LabelSize
LabelSizeForScale(double map_scale, double label_threshold,
                  MS_SHAPE_TYPE type) noexcept
{
  /* Lines (roads, rivers) stay SMALL.  Each font size is a separate
     TextCache key; 256 GPU textures.  Upsizing every street name at
     circuit scale misses the cache and uploads glyphs on Mali-400. */
  if (type != MS_SHAPE_POINT)
    return TopographyLook::LabelSize::SMALL;

  if (map_scale <= LABEL_LARGE_SCALE)
    return TopographyLook::LabelSize::LARGE;

  if (label_threshold > 0 &&
      map_scale <= label_threshold * LABEL_MEDIUM_RANGE_FRACTION)
    return TopographyLook::LabelSize::MEDIUM;

  return TopographyLook::LabelSize::SMALL;
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

  const bool important = file.IsLabelImportant(map_scale);
  const auto size = LabelSizeForScale(map_scale,
                                      file.GetLabelThreshold(),
                                      visible_labels.front()->get_type());
  canvas.Select(look.GetLabelFont(important, size));
  canvas.SetTextColor(important ? COLOR_BLACK : COLOR_VERY_DARK_GRAY);
  canvas.SetBackgroundTransparent();

  std::set<std::string> drawn_labels;

  const Angle min_span =
    projection.PixelsToAngle(SHAPE_MIN_BBOX_PX);

  for (const XShape *shape_p : visible_labels) {
    const XShape &shape = *shape_p;

    if (ShapeTooSmallToDraw(shape, min_span))
      continue;

    const char *label = shape.GetLabel();
    assert(label != nullptr);
    if (label[0] == '\0')
      continue;

    /* Geographic centre, not the leftmost vertex: on compact shapes
       that vertex flips while panning/rotating, so the text jumps
       and loses the LabelBlock contest. */
    const GeoPoint center = shape.get_bounds().GetCenter();
    if (!center.IsValid())
      continue;

    const auto pt = projection.GeoToScreenIfVisible(center);
    if (!pt)
      continue;

    if (drawn_labels.contains(label))
      continue;

    const PixelSize tsize = canvas.CalcTextSize(label);
    PixelRect brect;
    if (shape.get_type() == MS_SHAPE_POINT && icon.IsDefined()) {
      /* Sit the name under the icon so it does not cover the symbol.
         Fills and roads stay centred on the hook. */
      const int pad = Layout::GetTextPadding();
      const PixelPoint origin{
        pt->x - int(tsize.width) / 2,
        pt->y + int(icon.GetSize().height + 1) / 2 + pad,
      };
      brect = PixelRect{origin, tsize};
    } else {
      brect = PixelRect::Centered(*pt, tsize);
    }
    if (!label_block.check(brect))
      continue;

    drawn_labels.emplace(label);
    canvas.DrawText(brect.GetTopLeft(), label);
  }
}
