// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../XCTracer/Internal.hpp"
#include "Device/Driver/XCTracer.hpp"

static Device *
XCTracerCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  return new XCTracerDevice();
}

const struct DeviceRegister xctracer_driver = {
  "XCTracer",
  "XC-Tracer Vario",
  0,
  XCTracerCreateOnPort,
};
