// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Convert.hpp"
#include "Projection/WindowProjection.hpp"
#include "util/ScopeExit.hxx"
#include "util/TruncateString.hpp"
#include "util/StringCompare.hxx"

#ifdef ENABLE_OPENGL
#include "Topography/ShapeRenderer.hpp"
#include "Geo/FAISphere.hpp"
#endif

#include <zzip/lib.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

static void
CopyShapeBaseName(char *dest, std::size_t dest_size,
                  const char *path) noexcept
{
  const char *base = path;
  for (const char *p = path; *p != '\0'; ++p)
    if (*p == '/' || *p == '\\')
      base = p + 1;

  CopyTruncateString(dest, dest_size, base);

  const std::size_t n = StringLength(dest);
  if (n >= 4 && StringIsEqualIgnoreCase(dest + n - 4, ".shp"))
    dest[n - 4] = '\0';
}

TopographyFile::TopographyFile(zzip_dir *_dir, const char *filename,
                               double _threshold,
                               double _label_threshold,
                               double _important_label_threshold,
                               const BGRA8Color _color,
                               int _label_field,
                               ResourceId _icon, ResourceId _big_icon,
                               ResourceId _ultra_icon,
                               unsigned _pen_width)
  :dir(_dir),
   file(dir, filename),
   label_field(_label_field),
   icon(_icon), big_icon(_big_icon), ultra_icon(_ultra_icon),
   pen_width(_pen_width),
   color(_color), scale_threshold(_threshold),
   label_threshold(_label_threshold),
   important_label_threshold(_important_label_threshold)
{
  CopyShapeBaseName(name, sizeof(name), filename);

  const std::size_t n_shapes = file.size();
  constexpr std::size_t MAX_SHAPES = 16 * 1024 * 1024;
  if (n_shapes == 0)
    throw std::runtime_error{"Empty shapefile"};

  if (n_shapes > MAX_SHAPES)
    throw std::runtime_error{"Too many shapes in shapefile"};

  const auto file_bounds = ImportRect(file.GetBounds());
  if (!file_bounds.Check())
    throw std::runtime_error{"Malformed shapefile bounds"};

  center = file_bounds.GetCenter();

  shapes.ResizeDiscard(n_shapes);

  if (dir != nullptr)
    ++dir->refcount;

  ++serial;
}

TopographyFile::~TopographyFile() noexcept
{
  if (dir != nullptr) {
    --dir->refcount;
    zzip_dir_free(dir);
  }
}

void
TopographyFile::ClearCache() noexcept
{
  const std::lock_guard lock{mutex};
  cache_list.clear();
  list.clear();
  cache_bounds.SetInvalid();
  cache_overscan = false;

  for (auto &i : shapes) {
    i.in_list = false;
    i.clip_bounds.SetInvalid();
    i.shape.reset();
  }

  cached_shapes = 0;
  ++serial;
}

void
TopographyFile::UnlinkVisible(ShapeEnvelope &e,
                              ShapeList::iterator &prev) noexcept
{
  assert(e.in_list);
  assert(&*std::next(prev) == &e);

  list.erase_after(prev);
  e.in_list = false;
}

void
TopographyFile::DropCached(ShapeEnvelope &e) noexcept
{
  assert(!e.in_list);
  assert(e.shape != nullptr);

  if (e.cache_hook.is_linked())
    cache_list.erase(cache_list.iterator_to(e));

  e.shape.reset();
  e.clip_bounds.SetInvalid();
  --cached_shapes;
}

void
TopographyFile::TouchCache(ShapeEnvelope &e) noexcept
{
  if (e.cache_hook.is_linked())
    cache_list.erase(cache_list.iterator_to(e));
  cache_list.push_front(e);
}

void
TopographyFile::EvictOverflow() noexcept
{
  if (cached_shapes <= MAX_CACHED_SHAPES)
    return;

  auto it = cache_list.end();
  while (cached_shapes > CACHE_KEEP_SHAPES &&
         it != cache_list.begin()) {
    --it;
    ShapeEnvelope &e = *it;
    if (e.in_list)
      continue;

    it = cache_list.erase(it);
    e.shape.reset();
    e.clip_bounds.SetInvalid();
    --cached_shapes;
  }
}

static std::unique_ptr<XShape>
LoadShape(ShapeFile &file, GeoPoint &center, std::size_t i, int label_field,
          const GeoBounds *clip, bool *clipped)
{
  shapeObj shape;
  msInitShape(&shape);
  AtScopeExit(&shape) { msFreeShape(&shape); };
  file.ReadShape(shape, i);

  const char *label = label_field >= 0
    ? file.ReadLabel(i, label_field)
    : nullptr;

  return std::make_unique<XShape>(shape, center, label, clip, clipped);
}

#ifdef ENABLE_OPENGL
/**
 * Ear-clip polygons on the topography thread so Paint never
 * triangulates a newly panned-in fill.
 */
static void
PrepareOpenGLShape(const XShape &shape, const TopographyFile &file,
                   const WindowProjection &map_projection,
                   unsigned layout_scale) noexcept
{
  if (shape.get_type() != MS_SHAPE_POLYGON)
    return;

  const Angle min_span =
    map_projection.PixelsToAngle(SHAPE_MIN_BBOX_PX);
  const GeoBounds &b = shape.get_bounds();
  if (b.GetWidth() < min_span && b.GetHeight() < min_span)
    return;

  const unsigned level =
    file.GetThinningLevel(map_projection.GetMapScale());
  if (layout_scale == 0)
    layout_scale = 1;
  const ShapeScalar min_distance =
    ShapeScalar(file.GetMinimumPointDistance(level))
    / (layout_scale * FAISphere::REARTH);
  [[maybe_unused]] const auto indices =
    shape.GetIndices(int(level), min_distance);
}
#endif

bool
TopographyFile::Update(const WindowProjection &map_projection,
                       [[maybe_unused]] unsigned layout_scale)
{
  if (map_projection.GetMapScale() > scale_threshold)
    /* not visible, don't update cache now */
    return false;

  const GeoBounds screenRect =
    map_projection.GetScreenBounds();
  if (cache_bounds.IsValid() && cache_bounds.IsInside(screenRect) &&
      cache_overscan)
    /* 2× cache still covers the screen */
    return false;

  /* First pass: screen only, so on-map roads appear before the
     surround is read from the zip.  Second pass: 2× overscan. */
  const bool fill_overscan =
    cache_bounds.IsValid() && cache_bounds.IsInside(screenRect);
  cache_bounds = screenRect.Scale(fill_overscan ? CACHE_BOUNDS_SCALE : 1);
  cache_overscan = fill_overscan;

  // Test which shapes are inside the given bounds and save the
  // status to file.status
  switch (file.WhichShapes(dir, ConvertRect(cache_bounds))) {
  case MS_FAILURE:
    ClearCache();
    throw std::runtime_error{"Failed to update shapefile"};

  case MS_DONE:
    /* screen is outside of map bounds */
    return false;

  case MS_SUCCESS:
    break;
  }

  const auto status = file.GetStatus();
  assert(status != nullptr);

  struct PendingReload {
    ShapeEnvelope *envelope;
    std::unique_ptr<XShape> shape;
    GeoBounds clip_bounds;
  };
  std::vector<PendingReload> reloads;

  auto it = shapes.begin();
  for (std::size_t i = 0; i < file.size(); ++i, ++it) {
    const bool visible = msGetBit(status, i);

    if (visible && it->shape != nullptr &&
        it->clip_bounds.IsValid() &&
        !it->clip_bounds.IsInside(cache_bounds)) {
      /* clipped to an old viewport; reload so the new area is
         complete.  Keep in-list geometry until the mutex commit
         so Paint cannot see a dangling XShape. */
      bool clipped = false;
      auto neu = LoadShape(file, center, i, label_field,
                           &cache_bounds, &clipped);
#ifdef ENABLE_OPENGL
      PrepareOpenGLShape(*neu, *this, map_projection, layout_scale);
#endif
      if (it->in_list) {
        reloads.push_back({
          &*it, std::move(neu),
          clipped ? cache_bounds : GeoBounds::Invalid(),
        });
        continue;
      }

      DropCached(*it);
      it->shape = std::move(neu);
      it->clip_bounds = clipped ? cache_bounds : GeoBounds::Invalid();
      ++cached_shapes;
      TouchCache(*it);
      continue;
    }

    if (!visible)
      continue;

    if (it->shape == nullptr) {
      assert(!it->in_list);

      bool clipped = false;
      auto neu = LoadShape(file, center, i, label_field,
                           &cache_bounds, &clipped);
#ifdef ENABLE_OPENGL
      PrepareOpenGLShape(*neu, *this, map_projection, layout_scale);
#endif
      it->shape = std::move(neu);
      it->clip_bounds = clipped ? cache_bounds : GeoBounds::Invalid();
      ++cached_shapes;
      TouchCache(*it);
    } else {
      TouchCache(*it);
    }
  }

  {
    const std::lock_guard lock{mutex};

    for (auto &r : reloads) {
      if (r.envelope->cache_hook.is_linked())
        cache_list.erase(cache_list.iterator_to(*r.envelope));
      r.envelope->shape = std::move(r.shape);
      r.envelope->clip_bounds = r.clip_bounds;
      TouchCache(*r.envelope);
    }

    auto prev = list.before_begin();
    it = shapes.begin();
    for (std::size_t i = 0; i < file.size(); ++i, ++it) {
      const bool visible = msGetBit(status, i);

      if (!visible) {
        if (it->in_list)
          UnlinkVisible(*it, prev);
        continue;
      }

      if (!it->in_list) {
        /* LRU may have dropped this envelope during an earlier
           load in this pass; skip rather than linking a null
           shape. */
        if (it->shape == nullptr)
          continue;

        prev = list.insert_after(prev, *it);
        it->in_list = true;
      } else {
        ++prev;
        assert(&*prev == &*it);
      }
    }

    assert(std::next(prev) == list.end());
    ++serial;
  }

  EvictOverflow();

  return true;
}

void
TopographyFile::LoadAll()
{
  /* After Update(), an envelope can be cached but not in #list
     (off-screen LRU), or cached with a viewport clip.  Walk
     in_list, not shape, and reload clipped geometry unclipped. */
  auto prev = list.before_begin();
  auto it = shapes.begin();
  for (std::size_t i = 0; i < file.size(); ++i, ++it) {
    if (it->shape != nullptr && it->clip_bounds.IsValid()) {
      {
        const std::lock_guard lock{mutex};
        if (it->in_list)
          UnlinkVisible(*it, prev);
      }
      DropCached(*it);
    }

    if (it->shape == nullptr) {
      assert(!it->in_list);
      it->shape = LoadShape(file, center, i, label_field,
                            nullptr, nullptr);
      ++cached_shapes;
      TouchCache(*it);
      const std::lock_guard lock{mutex};
      prev = list.insert_after(prev, *it);
      it->in_list = true;
    } else if (!it->in_list) {
      TouchCache(*it);
      const std::lock_guard lock{mutex};
      prev = list.insert_after(prev, *it);
      it->in_list = true;
    } else {
      ++prev;
      assert(&*prev == &*it);
    }
  }

  assert(std::next(prev) == list.end());

  const std::lock_guard lock{mutex};
  ++serial;
}

unsigned
TopographyFile::GetSkipSteps(double map_scale) const noexcept
{
  if (map_scale > scale_threshold * 0.75)
    return 4;
  if (2 * map_scale > scale_threshold)
    return 3;
  if (4 * map_scale > scale_threshold)
    return 2;
  return 1;
}

#ifdef ENABLE_OPENGL

unsigned
TopographyFile::GetThinningLevel(double map_scale) const noexcept
{
  if (2 * map_scale > scale_threshold)
    return 3;
  if (map_scale * 3 > scale_threshold)
    return 2;
  if (4 * map_scale > scale_threshold)
    return 1;

  return 0;
}

unsigned
TopographyFile::GetMinimumPointDistance(unsigned level) const noexcept
{
  switch (level) {
    case 1:
      return (unsigned)(4 * scale_threshold / 30);
    case 2:
      return (unsigned)(6 * scale_threshold / 30);
    case 3:
      return (unsigned)(9 * scale_threshold / 30);
  }
  return 1;
}

#endif
