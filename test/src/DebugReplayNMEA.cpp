// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugReplayNMEA.hpp"
#include "io/FileLineReader.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Port/NullPort.hpp"
#include "Device/Parser.hpp"
#include "Device/Config.hpp"

static DeviceConfig config;
static NullPort port;

DebugReplayNMEA::DebugReplayNMEA(FileLineReaderA *_reader,
                                 const DeviceRegister *driver)
  :DebugReplayFile(_reader),
   device(driver->CreateOnPort != NULL
          ? driver->CreateOnPort(config, port)
          : NULL)
{
  clock.Reset();
}

DebugReplay*
DebugReplayNMEA::Create(Path input_file, const std::string &driver_name)
{
  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    fprintf(stderr, "No such driver: %s\n", driver_name.c_str());
    return nullptr;
  }

  FileLineReaderA *reader = new FileLineReaderA(input_file);
  return new DebugReplayNMEA(reader, driver);
}


bool
DebugReplayNMEA::Next()
{
  last_basic = computed_basic;

  const char *line;
  while ((line = reader->ReadLine()) != NULL) {
    raw_basic.clock = clock.NextClock(raw_basic.time_available
                                      ? raw_basic.time
                                      : TimeStamp::Undefined());

    if (!device || !device->ParseNMEA(line, raw_basic))
      parser.ParseLine(line, raw_basic);

    if (raw_basic.location_available != last_basic.location_available) {
      Compute();
      return true;
    }
  }

  if (computed_basic.time_available)
    flying_computer.Finish(calculated.flight, computed_basic.time);

  return false;
}
