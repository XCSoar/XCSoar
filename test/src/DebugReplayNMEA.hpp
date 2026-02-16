// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DebugReplayFile.hpp"
#include "Device/Parser.hpp"
#include "time/ReplayClock.hpp"

#include <memory>

class FileLineReaderA;
class Device;
struct DeviceRegister;


class DebugReplayNMEA : public DebugReplayFile {
  std::unique_ptr<Device> device;

  NMEAParser parser;

  ReplayClock clock;

private:
  DebugReplayNMEA(FileLineReaderA *_reader, const DeviceRegister *driver);

public:
  virtual bool Next();

  static DebugReplay *Create(Path input_file, const std::string &driver_name);
};
