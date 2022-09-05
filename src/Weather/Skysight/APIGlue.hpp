// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef WEATHER_SKYSIGHTAPIGLUE_HPP
#define WEATHER_SKYSIGHTAPIGLUE_HPP

#include "util/tstring.hpp"
#include "Operation/Operation.hpp"

typedef void (*SkysightCallback) (
  const tstring details,
  const bool  success, 
  const tstring layer,
  const uint64_t time_index
);

enum class SkysightCallType {
  Login,
  Regions,
  Layers,
  LastUpdates,
  DataDetails,
  Data,
  Image
};

struct SkysightRequestArgs {
  const tstring url;
  const tstring path;
  const bool to_file;
  const SkysightCallType calltype;
  const tstring region;
  const tstring layer;
  const uint64_t from;
  const uint64_t to;
  const SkysightCallback cb;
  SkysightRequestArgs(const tstring _url, const tstring _path,
		      const bool _to_file, const SkysightCallType _ct,
		      const tstring _region, const tstring _layer,
		      const uint64_t _from = 0, const uint64_t _to = 0,
		      const SkysightCallback _cb = nullptr):
    url(_url), path(_path), to_file(_to_file), calltype(_ct),
    region(_region), layer(_layer), from(_from), to(_to), cb(_cb) {};
};
#endif
