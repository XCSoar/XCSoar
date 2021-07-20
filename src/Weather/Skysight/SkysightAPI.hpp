// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#ifndef WEATHER_SKYSIGHTAPI_HPP
#define WEATHER_SKYSIGHTAPI_HPP

#include "Metrics.hpp"
#include "util/tstring.hpp"

class SkysightAPI final {
public:
  tstring region;
  std::map<tstring, tstring> regions;

  SkysightAPI(tstring email, tstring password, tstring _region);
  ~SkysightAPI();
  
  bool IsInited();
  
protected:
  bool inited_regions;

  void LoadDefaultRegions();
};

#endif
