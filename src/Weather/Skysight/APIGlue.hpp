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

#ifndef WEATHER_SKYSIGHTAPIGLUE_HPP
#define WEATHER_SKYSIGHTAPIGLUE_HPP

#include "Util/tstring.hpp"
#include "Operation/Operation.hpp"


typedef void (*SkysightCallback)(
  const tstring &&details,
  const bool  success, 
  const tstring &&layer,
  const uint64_t time_index
);


enum class SkysightCallType {Login, Regions, Layers, LastUpdates, DataDetails, Data, Image};

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
  SkysightRequestArgs(const tstring &&_url, const tstring &&_path, const bool _to_file, 
       const SkysightCallType _ct, const tstring &&_region, const tstring &&_layer,
       const uint64_t _from = 0, const uint64_t _to = 0,
       const SkysightCallback _cb = nullptr) :
      url(_url), path(_path), to_file(_to_file), calltype(_ct), region(_region), layer(_layer), 
      from(_from), to(_to), cb(_cb) {};
};

#endif
