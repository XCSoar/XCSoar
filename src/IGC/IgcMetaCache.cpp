// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IgcMetaCache.hpp"

#include "IGC/IGCParser.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "io/FileLineReader.hpp"
#include "ui/event/Notify.hpp"
#include "co/InvokeTask.hxx"
#include "io/async/AsioThread.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "util/BindMethod.hxx"

#include <chrono>
#include <utility>

/**
 * Lightweight B-record parser that extracts only the time and GPS
 * validity flag, skipping the expensive location, altitude, and
 * extension parsing that IGCParseFix() performs.
 */
static bool
ParseBRecordTime(const char *line, BrokenTime &time,
                 bool &gps_valid) noexcept
{
  if (line[0] != 'B')
    return false;

  /* time is at offset 1..6, validity char at offset 24 */
  if (std::strlen(line) < 25)
    return false;

  if (!IGCParseTime(line + 1, time))
    return false;

  if (line[24] == 'A')
    gps_valid = true;
  else if (line[24] == 'V')
    gps_valid = false;
  else
    return false;

  return true;
}

IgcMetaCache::~IgcMetaCache() noexcept
{
  CancelBackgroundFill();
}

IgcMetaCache::CacheEntry
IgcMetaCache::ParseEntry(Path path) noexcept
{
  CacheEntry entry;
  entry.path = path;

  try {
    FileLineReaderA reader(path);
    char *line;
    while ((line = reader.ReadLine()) != nullptr) {
      BrokenTime time;
      bool gps_valid;
      if (ParseBRecordTime(line, time, gps_valid) && gps_valid) {
        if (!entry.meta.has_start) {
          entry.meta.start = time;
          entry.meta.has_start = true;
        }
        entry.meta.end = time;
        entry.meta.has_end = true;
      }
    }
  } catch (...) {
    // ignore parse errors
  }

  entry.text = "";

  if (entry.meta.has_start && entry.meta.has_end) {
    StaticString<32> lbuf;
    lbuf.Format("%02u:%02u - %02u:%02u",
                (unsigned)entry.meta.start.hour,
                (unsigned)entry.meta.start.minute,
                (unsigned)entry.meta.end.hour,
                (unsigned)entry.meta.end.minute);
    entry.text = lbuf.c_str();

    int64_t s = (int64_t)entry.meta.start.GetSecondOfDay();
    int64_t e = (int64_t)entry.meta.end.GetSecondOfDay();
    int64_t diff = e - s;
    if (diff < 0)
      diff += 24 * 3600;
    auto dur = FormatTimespanSmart(std::chrono::seconds(diff), 2);
    entry.text.append(" (");
    entry.text.append(dur.c_str());
    entry.text.append(")");
  }

  return entry;
}

IgcMetaCache::CacheEntry *
IgcMetaCache::FindOrParse(Path path) noexcept
{
  {
    const std::lock_guard lock{cache_mutex};
    for (auto &e : cache) {
      if (e.path == path)
        return &e;
    }
  }

  CacheEntry entry = ParseEntry(path);

  const std::lock_guard lock{cache_mutex};
  for (auto &e : cache) {
    if (e.path == path)
      return &e;
  }

  cache.push_back(std::move(entry));
  return &cache.back();
}

std::string
IgcMetaCache::GetCompactInfo(Path path) noexcept
{
  CacheEntry *entry = FindOrParse(path);
  return entry != nullptr ? std::string(entry->text.c_str()) : std::string();
}

const char *
IgcMetaCache::GetCompactInfoPtr(Path path) noexcept
{
  CacheEntry *entry = FindOrParse(path);
  return entry != nullptr ? entry->text.c_str() : nullptr;
}

Co::InvokeTask
IgcMetaCache::FillCacheCoro(std::vector<AllocatedPath> paths) noexcept
{
  for (const auto &path : paths) {
    GetCompactInfo(Path(path.c_str()));
  }

  co_return;
}

void
IgcMetaCache::OnFillComplete([[maybe_unused]] std::exception_ptr error) noexcept
{
  // Notify UI that fill is complete (ignore any errors)
  if (auto *notify = current_notify.exchange(nullptr))
    notify->SendNotification();
}

void
IgcMetaCache::StartBackgroundFill(std::vector<AllocatedPath> paths,
                                  UI::Notify *notify) noexcept
{
  if (!inject_task)
    inject_task = std::make_unique<Co::InjectTask>(asio_thread->GetEventLoop());

  if (*inject_task) {
    CancelBackgroundFill();
    inject_task.reset();
    inject_task = std::make_unique<Co::InjectTask>(asio_thread->GetEventLoop());
  }

  current_notify.store(notify);
  inject_task->Start(FillCacheCoro(std::move(paths)), BIND_THIS_METHOD(OnFillComplete));
}

void
IgcMetaCache::CancelBackgroundFill() noexcept
{
  if (!inject_task)
    return;

  current_notify.store(nullptr);
  inject_task->Cancel();
}

void
IgcMetaCache::PollBackgroundFill() noexcept
{
  if (!inject_task || !*inject_task)
    return;

  // No synchronous wait available; completion is reported via OnFillComplete().
}
