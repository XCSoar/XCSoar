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

#ifndef WEATHER_SKYSIGHT_HPP
#define WEATHER_SKYSIGHT_HPP

#include "Weather/Skysight/SkysightAPI.hpp"
#include "Blackboard/BlackboardListener.hpp"

class Skysight final: private NullBlackboardListener {
public:
  tstring region = "EUROPE";

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
