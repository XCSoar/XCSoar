// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef WEATHER_SKYSIGHTCDFDEC_HPP
#define WEATHER_SKYSIGHTCDFDEC_HPP


#include "APIGlue.hpp"
#include "thread/StandbyThread.hpp"
#include "util/tstring.hpp"
#include <map>
#include "Metrics.hpp"
#include "system/Path.hpp"

class CDFDecoder final : public StandbyThread {
public:
  enum class Status {Idle, Busy, Complete, Error};

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

public:
  enum class Result {Available, Requested, Error};

  CDFDecoder(const tstring &&_path, const tstring &&_output, const tstring &&_varname,
             const uint64_t _time_index, const std::map<float, LegendColor> &_legend, SkysightCallback _callback) : 
             StandbyThread("CDFDecoder"), path(AllocatedPath(_path.c_str())), output_path(AllocatedPath(_output.c_str())), 
             data_varname(_varname), time_index(_time_index), legend(_legend), callback(_callback), 
             status(Status::Idle) {};
  ~CDFDecoder() {};

  void DecodeAsync();
  void Done();
  Status GetStatus();
};

#endif
