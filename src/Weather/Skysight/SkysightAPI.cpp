// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightAPI.hpp"
#include "SkysightRegions.hpp"

SkysightAPI::SkysightAPI(tstring email, tstring password, tstring _region)
{
  inited_regions = false;
  LoadDefaultRegions();

  region = (_region.empty()) ? "EUROPE" : _region;
  if (regions.find(region) == regions.end()) {
    region = "EUROPE";
  }
}

bool
SkysightAPI::IsInited()
{
  return inited_regions;
}

void
SkysightAPI::LoadDefaultRegions()
{
  for (auto r = skysight_region_defaults; r->id != nullptr; ++r)
    regions.emplace(std::pair<tstring, tstring>(r->id, r->name));
}
