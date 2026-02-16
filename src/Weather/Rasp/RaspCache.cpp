// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspCache.hpp"
#include "RaspStore.hpp"
#include "Terrain/RasterMap.hpp"
#include "Terrain/Loader.hpp"
#include "Language/Language.hpp"
#include "system/Path.hpp"
#include "io/ZipArchive.hpp"
#include "LogFile.hpp"

#include <cassert>
#include <windef.h> // for MAX_PATH

RaspCache::RaspCache(const RaspStore &_store, unsigned _parameter) noexcept
  :store(_store), parameter(_parameter) {}

RaspCache::~RaspCache() noexcept = default;

static constexpr unsigned
ToQuarterHours(BrokenTime t)
{
  return t.hour * 4u + t.minute / 15;
}

const char *
RaspCache::GetMapName() const
{
  return store.GetItemInfo(parameter).name;
}

const char *
RaspCache::GetMapLabel() const
{
  const auto &info = store.GetItemInfo(parameter);
  return info.label != nullptr
    ? gettext(info.label)
    : info.name.c_str();
}

void
RaspCache::SetTime(BrokenTime t)
{
  unsigned i = t.IsPlausible() ? ToQuarterHours(t) : 0;
  time = i;
}

BrokenTime
RaspCache::GetTime() const
{
  return time == 0
    ? BrokenTime::Invalid()
    : RaspStore::IndexToTime(time);
}

bool
RaspCache::IsInside(GeoPoint p) const
{
  return map != nullptr && map->IsInside(p);
}

void
RaspCache::Reload(BrokenTime time_local, OperationEnvironment &operation)
{
  unsigned effective_time = time;
  if (effective_time == 0) {
    // "Now" time, so find time in half hours
    if (!time_local.IsPlausible())
      /* can't update to current time if we don't know the current
         time */
      return;

    effective_time = ToQuarterHours(time_local);
    assert(effective_time < RaspStore::MAX_WEATHER_TIMES);
  }

  if (effective_time == last_time)
    // no change, quick exit.
    return;

  last_time = effective_time;

  effective_time = store.GetNearestTime(parameter, effective_time);
  if (effective_time == RaspStore::MAX_WEATHER_TIMES)
    return;

  map.reset();

  auto archive = store.OpenArchive();
  if (!archive)
    return;

  char new_name[MAX_PATH];
  store.WeatherFilename(new_name, Path(store.GetItemInfo(parameter).name),
                              effective_time);

  auto new_map = std::make_unique<RasterMap>();
  try {
    LoadTerrainOverview(archive->get(), new_name, nullptr,
                        new_map->GetTileCache(),
                        true, operation);
  } catch (...) {
    LogError(std::current_exception(), "Failed to load RASP file");
    return;
  }

  new_map->UpdateProjection();

  map = std::move(new_map);
}
