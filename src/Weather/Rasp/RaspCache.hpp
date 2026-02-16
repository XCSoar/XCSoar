// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

#include <tchar.h>

struct BrokenTime;
struct GeoPoint;
class RaspStore;
class RasterMap;
class OperationEnvironment;

/**
 * Class to manage the raster weather map, to be loaded/selected from
 * a #RaspStore instance.
 */
class RaspCache {
  const RaspStore &store;

  const unsigned parameter;

  unsigned time = 0;
  unsigned last_time = 0;

  std::unique_ptr<RasterMap> map;

public:
  RaspCache(const RaspStore &_store, unsigned _parameter) noexcept;
  ~RaspCache() noexcept;

  const RaspStore &GetStore() const {
    return store;
  }

  [[gnu::pure]]
  const RasterMap *GetMap() const {
    return map.get();
  }

  /**
   * Returns the current map's name.
   */
  [[gnu::pure]]
  const char *GetMapName() const;

  /**
   * Returns the human-readable name for the current RASP map, or
   * nullptr if no RASP map is enabled.
   */
  [[gnu::pure]]
  const char *GetMapLabel() const;

  /**
   * Returns the index of the weather map being displayed.
   */
  [[gnu::pure]]
  unsigned GetParameter() const {
    return parameter;
  }

  [[gnu::pure]]
  bool IsInside(GeoPoint p) const;

  /**
   * @param day_time the local time, in seconds since midnight
   */
  void Reload(BrokenTime time_local, OperationEnvironment &operation);

  /**
   * Returns the current time index.
   */
  [[gnu::pure]]
  BrokenTime GetTime() const;

  /**
   * Sets the current time index.
   */
  void SetTime(BrokenTime t);
};
