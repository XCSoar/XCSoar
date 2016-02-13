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
    const ScopeLock protect(mutex);
    next_projection = _projection;
    StandbyThread::Trigger();
  }
}

void
TopographyThread::Tick()
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
