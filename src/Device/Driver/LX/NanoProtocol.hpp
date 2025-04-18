// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"

/**
 * Code specific to the LXNAV Nano.
 *
 * Source: Nano Dataport specification Version 2.1
 */
namespace Nano {
  /**
   * Request basic Nano information.
   */
  static void
  RequestInfo(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXVC,INFO,R", env);
  }

  /**
   * Request basic Nano information through an LXNAV V7 (firmware >=
   * 2.01).
   */
  static void
  RequestForwardedInfo(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXVC,GPSINFO,R", env);
  }
}
