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

#ifndef WEATHER_SKYSIGHT_METRICS_HPP
#define WEATHER_SKYSIGHT_METRICS_HPP

#include "Util/tstring.hpp"
#include "Time/BrokenDateTime.hpp"
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
 // std::map<uint64_t, tstring> datafiles;

public:
  SkysightMetric(tstring _id, tstring _name, tstring _desc) : id(_id), name(_name), desc(_desc) {}
  SkysightMetric(const SkysightMetric &m) : id(m.id), name(m.name), desc(m.desc), last_update(m.last_update), legend(m.legend) {}
};


struct SkysightActiveMetric {
  SkysightMetric *metric;
  double from = 0;
  double to = 0;
  double mtime = 0;
  bool updating = false;
  
public:
  SkysightActiveMetric(SkysightMetric *_metric, uint64_t _from, uint64_t _to, uint64_t _mtime) : 
                                        metric(_metric), from(_from), to(_to), mtime(_mtime) {}
  SkysightActiveMetric(const SkysightActiveMetric &m) : metric(m.metric), from(m.from), 
to(m.to), mtime(m.mtime), updating(m.updating) {}
};

struct DisplayedMetric {
  SkysightMetric *metric;
  BrokenDateTime forecast_index;
  DisplayedMetric() {
    metric = nullptr;
  };
  DisplayedMetric(SkysightMetric *_metric, BrokenDateTime _fc_index) : metric(_metric),
                                                                  forecast_index(_fc_index) {};
  void clear() {
    metric = nullptr;
  }
  bool operator==(const DisplayedMetric &d) {
    return (*this == d.metric->id.c_str());
  };

  bool operator==(const TCHAR *const id) {
    if(!metric || !id)
      return false;

    return (metric->id.compare(id) == 0);
  };

  bool operator==(const BrokenDateTime &t) {

    if(!forecast_index.IsPlausible())
      return false;

    return (forecast_index.ToUnixTimeUTC() == t.ToUnixTimeUTC());
  }

  // self > t?
  bool operator>(const BrokenDateTime &t) {
    if(!forecast_index.IsPlausible())
      return false;

    return (forecast_index.ToUnixTimeUTC() > t.ToUnixTimeUTC());
  }

  // self < t?
  bool operator<(const BrokenDateTime &t) {
    if(!forecast_index.IsPlausible())
      return false;

    return (forecast_index.ToUnixTimeUTC() < t.ToUnixTimeUTC());
  }
  
};





#endif
