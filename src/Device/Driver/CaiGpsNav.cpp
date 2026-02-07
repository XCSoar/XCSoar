// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

//


// CAUTION!
// caiGpsNavParseNMEA is called from com port read thread
// all other functions are called from windows message loop thread

#include "Device/Driver/CaiGpsNav.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"

#include <tchar.h>

static constexpr char CtrlC = '\x03';

class CaiGpsNavDevice : public AbstractDevice {
private:
  Port &port;

public:
  CaiGpsNavDevice(Port &_port):port(_port) {}

public:
  bool EnableNMEA(OperationEnvironment &env) override;
};

bool
CaiGpsNavDevice::EnableNMEA(OperationEnvironment &env)
{
  port.Write(CtrlC);
  env.Sleep(std::chrono::milliseconds(200));
  port.FullWrite("NMEA\r", env, std::chrono::milliseconds{100});

  // This is for a slightly different mode, that
  // apparently outputs pressure info too...
  //port.Write("PNP\r\n");
  //port.Write("LOG 0\r\n");

  return true;
}

static Device *
CaiGpsNavCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &com_port)
{
  return new CaiGpsNavDevice(com_port);
}

const struct DeviceRegister gps_nav_driver = {
  "CAI GPS-NAV",
  "Cambridge CAI GPS-NAV",
  0,
  CaiGpsNavCreateOnPort,
};
