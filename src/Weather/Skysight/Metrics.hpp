// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef WEATHER_SKYSIGHT_METRICS_HPP
#define WEATHER_SKYSIGHT_METRICS_HPP

#include "util/tstring.hpp"
#include "time/BrokenDateTime.hpp"
#include <map>

struct LegendColor {
  unsigned char Red;
  unsigned char Green;
  unsigned char Blue;
};

struct SkysightMetric {
  const tstring id;
  const tstring name;
  const tstring desc;
  uint64_t last_update = 0;
  std::map<float, LegendColor> legend;

public:
  SkysightMetric(tstring _id, tstring _name, tstring _desc):
    id(_id), name(_name), desc(_desc) {}
  SkysightMetric(const SkysightMetric &m):
    id(m.id), name(m.name), desc(m.desc), last_update(m.last_update),
    legend(m.legend) {}
};

/*
 * Skysight chart which is overlaid
 */
struct SkysightActiveMetric {
  SkysightMetric *metric;
  double from = 0;
  double to = 0;
  double mtime = 0;
  bool updating = false;

public:
  SkysightActiveMetric(SkysightMetric *_metric, uint64_t _from,
		       uint64_t _to, uint64_t _mtime): 
    metric(_metric), from(_from), to(_to), mtime(_mtime) {}
  SkysightActiveMetric(const SkysightActiveMetric &m):
    metric(m.metric), from(m.from), to(m.to), mtime(m.mtime),
    updating(m.updating) {}
};

struct DisplayedMetric {
  SkysightMetric *metric;
  BrokenDateTime forecast_index;

  DisplayedMetric() { metric = nullptr; };

  DisplayedMetric(SkysightMetric *_metric, BrokenDateTime _fc_index):
    metric(_metric), forecast_index(_fc_index) {};

  void clear() { metric = nullptr; }

  bool operator == (const TCHAR *const id) {
    if (!metric || !id)
      return false;

    return (metric->id.compare(id) == 0);
  };

  bool operator < (const BrokenDateTime &t) {
    if (!forecast_index.IsPlausible())
      return false;

    return (
      std::chrono::system_clock::to_time_t(forecast_index.ToTimePoint()) <
      std::chrono::system_clock::to_time_t(t.ToTimePoint()));
  }
};

#endif
