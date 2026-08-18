// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Convert.hpp"
#include "Projection/WindowProjection.hpp"
#include "util/ScopeExit.hxx"

#ifdef ENABLE_OPENGL
#include "Topography/ShapeRenderer.hpp"
#include "Geo/FAISphere.hpp"
#include "Screen/Layout.hpp"
#endif

#include <zzip/lib.h>

#include <algorithm>
#include <stdexcept>

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

  const std::lock_guard lock{mutex};
  list.erase_after(prev);
  e.in_list = false;
  ++serial;
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

bool
TopographyFile::Update(const WindowProjection &map_projection)
{
  if (map_projection.GetMapScale() > scale_threshold)
    /* not visible, don't update cache now */
    return false;

  const GeoBounds screenRect =
    map_projection.GetScreenBounds();
  if (cache_bounds.IsValid() && cache_bounds.IsInside(screenRect))
    /* the cache is still fresh */
    return false;

  cache_bounds = screenRect.Scale(2);

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

  auto prev = list.before_begin();
  auto it = shapes.begin();
  for (std::size_t i = 0; i < file.size(); ++i, ++it) {
    const bool visible = msGetBit(status, i);

    if (visible && it->shape != nullptr &&
        it->clip_bounds.IsValid() &&
        !it->clip_bounds.IsInside(cache_bounds)) {
      /* clipped to an old viewport; reload so the new area is
         complete */
      if (it->in_list)
        UnlinkVisible(*it, prev);
      DropCached(*it);
    }

    if (!visible) {
      if (it->in_list)
        UnlinkVisible(*it, prev);
      continue;
    }

    if (it->shape == nullptr) {
      assert(!it->in_list);

      EvictOverflow();

      bool clipped = false;
      it->shape = LoadShape(file, center, i, label_field,
                            &cache_bounds, &clipped);
      it->clip_bounds = clipped ? cache_bounds : GeoBounds::Invalid();
      ++cached_shapes;
      TouchCache(*it);

#ifdef ENABLE_OPENGL
      /* Triangulate on the topography thread so Paint never
         ear-clips a newly panned-in fill.  Skip sub-pixel fills
         (same 1 px bbox rule as Paint). */
      if (it->shape->get_type() == MS_SHAPE_POLYGON) {
        const Angle min_span =
          map_projection.PixelsToAngle(SHAPE_MIN_BBOX_PX);
        const GeoBounds &b = it->shape->get_bounds();
        if (b.GetWidth() >= min_span || b.GetHeight() >= min_span) {
          const unsigned level =
            GetThinningLevel(map_projection.GetMapScale());
          const ShapeScalar min_distance =
            ShapeScalar(GetMinimumPointDistance(level))
            / (Layout::Scale(1) * FAISphere::REARTH);
          [[maybe_unused]] const auto indices =
            it->shape->GetIndices(int(level), min_distance);
        }
      }
#endif

      {
        const std::lock_guard lock{mutex};
        prev = list.insert_after(prev, *it);
        it->in_list = true;
        ++serial;
      }
    } else {
      TouchCache(*it);
      if (!it->in_list) {
        const std::lock_guard lock{mutex};
        prev = list.insert_after(prev, *it);
        it->in_list = true;
        ++serial;
      } else {
        ++prev;
        assert(&*prev == &*it);
      }
    }
  }

  assert(std::next(prev) == list.end());

  EvictOverflow();

  return true;
}

void
TopographyFile::LoadAll()
{
  // Iterate through the shapefile entries
  auto prev = list.before_begin();
  auto it = shapes.begin();
  for (std::size_t i = 0; i < file.size(); ++i, ++it) {
    if (it->shape == nullptr) {
      assert(&*std::next(prev) != &*it);
      // shape isn't cached yet -> cache the shape
      it->shape = LoadShape(file, center, i, label_field,
                            nullptr, nullptr);
      ++cached_shapes;
      TouchCache(*it);
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
