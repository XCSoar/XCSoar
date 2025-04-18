// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/StandbyThread.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/GeoBounds.hpp"

#include <functional>

class TopographyStore;

/**
 * A thread that loads topography files asynchronously.
 */
class TopographyThread final : private StandbyThread {
  TopographyStore &store;

  const std::function<void()> callback;

  WindowProjection next_projection;

  GeoBounds last_bounds;
  double scale_threshold;

public:
  TopographyThread(TopographyStore &_store, std::function<void()> &&_callback);
  ~TopographyThread();

  using StandbyThread::LockStop;

  void Trigger(const WindowProjection &_projection);

private:
  /* virtual methods from class StandbyThread*/
  void Tick() noexcept override;
};
