// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef WEATHER_SKYSIGHT_HPP
#define WEATHER_SKYSIGHT_HPP

#include "Weather/Skysight/SkysightAPI.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"

class Skysight final: private NullBlackboardListener {
public:
  tstring region = tstring(_("EUROPE"));

  static void APIInited(const tstring &&details,  const bool success,  
			const tstring &&layer_id,  const uint64_t time_index);

  std::map<tstring, tstring> GetRegions() {
    return api->regions;
  }

  tstring GetRegion() {
    return api->region;
  }

  Skysight();

  void Init();

protected:
  SkysightAPI *api;

private:
  tstring email;
  tstring password;
};

#endif
