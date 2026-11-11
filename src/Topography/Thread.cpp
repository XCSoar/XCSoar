// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Thread.hpp"
#include "TopographyStore.hpp"
#include "TopographyFile.hpp"
#include "Projection/MapLod.hpp"
#include "Screen/Layout.hpp"

static unsigned
ScanLodLookahead(TopographyStore &store, const WindowProjection &src,
                 unsigned target, unsigned layout_scale) noexcept
{
  if (target >= MAP_LOD_COUNT)
    return 0;

  WindowProjection p = src;
  const double factor = MapLodRepresentativeMapScale(target) /
    src.GetMapScale();
  if (factor <= 0)
    return 0;

  p.SetScale(src.GetScale() / factor);
  p.UpdateScreenBounds();
  return store.ScanVisibility(p, 1, layout_scale);
}

TopographyThread::TopographyThread(TopographyStore &_store,
                                   std::function<void()> &&_callback)
  :StandbyThread("Topography"),
   store(_store),
   callback(std::move(_callback)),
   last_bounds(GeoBounds::Invalid()) {}

TopographyThread::~TopographyThread()
{
}

void
TopographyThread::Trigger(const WindowProjection &_projection)
{
  assert(_projection.IsValid());

  const GeoBounds new_bounds = _projection.GetScreenBounds();
  const unsigned lod = MapLod(_projection.GetMapScale());
  if (last_lod == lod &&
      MapLodQuadInside(last_bounds, _projection.GetGeoQuadrilateral())) {
    /* still inside cache bounds - now check if we crossed a scale
       threshold for at least one file, which would mean we have to
       update a file which was not updated for the current cache
       bounds */
    if (scale_threshold < 0 ||
        _projection.GetMapScale() >= scale_threshold)
      /* the cache is still fresh */
      return;
  }

  last_bounds = new_bounds.Scale(TopographyFile::CACHE_BOUNDS_SCALE);
  last_lod = lod;
  scale_threshold = store.GetNextScaleThreshold(_projection.GetMapScale());

  {
    const std::lock_guard lock{mutex};
    next_projection = _projection;
    StandbyThread::Trigger();
  }
}

void
TopographyThread::Tick() noexcept
{
  // TODO: call only once
  SetIdlePriority();

  bool again = true;
  while (next_projection.IsValid() && again && !IsStopped()) {
    const WindowProjection projection = next_projection;

    unsigned n_updated;
    {
      const ScopeUnlock unlock(mutex);
      n_updated = store.ScanVisibility(projection, 1, Layout::Scale(1u));
    }
    again = n_updated > 0;

    /* Redraw after each layer so the screen pass is visible before
       the 2× overscan finishes. */
    if (callback) {
      const ScopeUnlock unlock(mutex);
      callback();
    }
  }

  if (next_projection.IsValid() && !IsStopped()) {
    const WindowProjection projection = next_projection;
    const unsigned lod = MapLod(projection.GetMapScale());
    const unsigned layout_scale = Layout::Scale(1u);
    unsigned extra = 0;

    {
      const ScopeUnlock unlock(mutex);
      if (lod > 0)
        extra += ScanLodLookahead(store, projection, lod - 1,
                                  layout_scale);
      if (lod + 1 < MAP_LOD_COUNT)
        extra += ScanLodLookahead(store, projection, lod + 1,
                                  layout_scale);
    }

    if (extra > 0 && callback) {
      const ScopeUnlock unlock(mutex);
      callback();
    }
  }
}
