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
  if (parameter >= store.GetItemCount())
    return "";

  return store.GetItemInfo(parameter).name;
}

const char *
RaspCache::GetMapLabel() const
{
  if (parameter >= store.GetItemCount())
    return "";

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
  if (parameter >= store.GetItemCount())
    return;

  unsigned effective_time = time;
  if (effective_time == 0) {
    // "Now" time, so find time in quarter hours
    if (time_local.IsPlausible()) {
      effective_time = ToQuarterHours(time_local);
      assert(effective_time < RaspStore::MAX_WEATHER_TIMES);
    } else if (!store.IsDayField(parameter)) {
      /* can't update to current time if we don't know the current
         time; day fields have no time axis, so effective_time stays 0
         and still resolves to their single slot below */
      return;
    }
  }

  const unsigned requested_time = effective_time;
  const unsigned resolved_time =
    store.GetNearestTime(parameter, requested_time);
  if (resolved_time == RaspStore::MAX_WEATHER_TIMES)
    return;

  if (resolved_time == last_time)
    // no change, quick exit.
    return;

  if (resolved_time == failed_time)
    /* avoid retrying malformed/unsupported tiles every redraw */
    return;

  auto archive = store.OpenArchive();
  if (!archive)
    return;

  char new_name[MAX_PATH];
  if (!store.WeatherFilename(new_name, Path(store.GetItemInfo(parameter).name),
                             resolved_time))
    return;

  auto new_map = std::make_unique<RasterMap>();
  try {
    LoadTerrainOverview(archive->get(), new_name, nullptr,
                        new_map->GetTileCache(),
                        true, operation);
  } catch (...) {
    LogError(std::current_exception(), "Failed to load RASP file");
    failed_time = resolved_time;
    return;
  }

  new_map->UpdateProjection();

  map = std::move(new_map);
  last_time = resolved_time;
  failed_time = unsigned(-1);
}
