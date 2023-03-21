// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Thread.hpp"
#include "TopographyStore.hpp"

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
  if (last_bounds.IsValid() && last_bounds.IsInside(new_bounds)) {
    /* still inside cache bounds - now check if we crossed a scale
       threshold for at least one file, which would mean we have to
       update a file which was not updated for the current cache
       bounds */
    if (scale_threshold < 0 ||
        _projection.GetMapScale() >= scale_threshold)
      /* the cache is still fresh */
      return;
  }

  last_bounds = new_bounds.Scale(1.1);
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

    const ScopeUnlock unlock(mutex);
    again = store.ScanVisibility(projection, 1) > 0;
  }

  /* notify the client that we have updated the topography cache */
  if (callback) {
    const ScopeUnlock unlock(mutex);
    callback();
  }
}
