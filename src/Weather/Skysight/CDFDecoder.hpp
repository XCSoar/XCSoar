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

#ifndef WEATHER_SKYSIGHTCDFDEC_HPP
#define WEATHER_SKYSIGHTCDFDEC_HPP


#include "APIGlue.hpp"
#include "Thread/StandbyThread.hpp"
#include "Util/tstring.hpp"
#include <map>
#include "Layers.hpp"
#include "OS/Path.hpp"


class CDFDecoder final : public StandbyThread {

public:
  
  enum class Result {Available, Requested, Error};

  enum class Status {Idle, Busy, Complete, Error};
  
  CDFDecoder(const tstring &&_path, const tstring &&_output, const tstring &&_varname,
             const uint64_t _time_index, const std::map<float, LegendColor> _legend, SkysightCallback _callback) : 
             StandbyThread("CDFDecoder"), path(AllocatedPath(_path.c_str())), output_path(AllocatedPath(_output.c_str())), 
             data_varname(_varname), time_index(_time_index), legend(_legend), callback(_callback), 
             status(Status::Idle) {};
  ~CDFDecoder() {};

  void DecodeAsync();
  void DecodeSync();
  void Done();
  Status GetStatus();
  
private:
  const AllocatedPath path;
  const AllocatedPath output_path;
  const tstring data_varname;
  const uint64_t time_index;
  const std::map<float, LegendColor> legend;
  SkysightCallback callback;
  Status status;
  void Tick() noexcept override; 
  bool Decode();
  void MakeCallback(bool result);
  bool DecodeError();
  bool DecodeSuccess();
};

#endif
