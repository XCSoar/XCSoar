// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IgcMetaCache.hpp"

#include "IGC/IGCExtensions.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "io/FileLineReader.hpp"

#include <chrono>

IgcMetaCache::CacheEntry *
IgcMetaCache::FindOrParse(Path path) noexcept
{
  for (auto &e : cache) {
    if (e.path == path)
      return &e;
  }

  CacheEntry entry;
  entry.path = path;

  try {
    FileLineReaderA reader(path);
    IGCExtensions ext;
    char *line;
    while ((line = reader.ReadLine()) != nullptr) {
      IGCFix fix;
      if (IGCParseExtensions(line, ext))
        continue;
      if (IGCParseFix(line, ext, fix) && fix.gps_valid &&
          fix.time.IsPlausible()) {
        if (!entry.meta.has_start) {
          entry.meta.start = fix.time;
          entry.meta.has_start = true;
        }
        entry.meta.end = fix.time;
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

  cache.push_back(entry);
  return &cache.back();
}

const char *
IgcMetaCache::GetCompactInfo(Path path) noexcept
{
  CacheEntry *entry = FindOrParse(path);
  return entry != nullptr ? entry->text.c_str() : nullptr;
}
