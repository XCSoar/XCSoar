// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Job/Job.hpp"
#include "system/Path.hpp"

class CAI302Device;

class CAI302WaypointUploader : public Job {
  const Path path;
  CAI302Device &device;

public:
  CAI302WaypointUploader(Path _path, CAI302Device &_device)
    :path(_path), device(_device) {}

  virtual void Run(OperationEnvironment &env);
};
