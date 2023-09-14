// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Route/RoutePolars.hpp"

class FlatProjection;
class RasterMap;

struct ReachFanParms {
  const RoutePolars &rpolars;
  const FlatProjection &projection;
  const RasterMap *terrain;
  int terrain_base;
  unsigned terrain_counter = 0;
  unsigned fan_counter = 0;
  unsigned vertex_counter = 0;
  unsigned char set_depth = 0;

  ReachFanParms(const RoutePolars& _rpolars,
                const FlatProjection &_projection,
                const short _terrain_base,
                const RasterMap *_terrain=nullptr)
    :rpolars(_rpolars), projection(_projection), terrain(_terrain),
     terrain_base(_terrain_base) {}

  [[gnu::pure]]
  FlatGeoPoint ReachIntercept(int index, const AFlatGeoPoint &flat_origin,
                              const GeoPoint &origin) const {
    return rpolars.ReachIntercept(index, flat_origin, origin,
                                  terrain, projection);
  }
};
