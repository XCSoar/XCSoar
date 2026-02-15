// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

#include <fmt/format.h>

/**
 * Code specific to LXNav varios (e.g. V7).
 *
 * Source: LXNAV DataPort Specification v1.05
 */
namespace LXNAVVario {
  /**
   * Enable direct link with GPS port.
   */
  static inline void
  ModeDirect(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXV0,CONNECTION,W,DIRECT", env);
  }

  /**
   * Enable communication with the vario.
   */
  static inline void
  ModeNormal(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXV0,CONNECTION,W,VSEVEN", env);
  }

  /**
   * Set up the NMEA sentences sent by the vario:
   *
   * - PLXVF at 2 Hz
   * - PLXVS every 5 seconds
   * - LXWP0 every second
   * - LXWP1 every 60 seconds
   * - LXWP2 every 5 seconds (contains MC, ballast, bugs - more frequent for better sync)
   * - LXWP3 disabled (we don't parse it)
   * - LXWP5 disabled (we don't parse it)
   */
  static inline void
  SetupNMEA(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PLXV0,NMEARATE,W,2,5,1,60,5,0,0", env);
  }

  /**
   * Set the MC setting of the vario
   * @param mc in m/s
   */
  static inline void
  SetMacCready(Port &port, OperationEnvironment &env, double mc)
  {
    const auto buffer = fmt::format("PLXV0,MC,W,{:.1f}", mc);
    PortWriteNMEA(port, buffer.c_str(), env);
  }

  /**
   * Set the ballast setting of the vario
   * @param overload 1.0 - 1.4 (100 - 140%)
   */
  static inline void
  SetBallast(Port &port, OperationEnvironment &env, double overload)
  {
    const auto buffer = fmt::format("PLXV0,BAL,W,{:.2f}", overload);
    PortWriteNMEA(port, buffer.c_str(), env);
  }

  /**
   * Set the bugs setting of the vario
   * @param bugs 0 - 30 %
   */
  static inline void
  SetBugs(Port &port, OperationEnvironment &env, unsigned bugs)
  {
    const auto buffer = fmt::format("PLXV0,BUGS,W,{}", bugs);
    PortWriteNMEA(port, buffer.c_str(), env);
  }

  /**
   * Set the QNH setting of the vario
   */
  static inline void
  SetQNH(Port &port, OperationEnvironment &env,
         const AtmosphericPressure &qnh)
  {
    const unsigned qnh_pascal = uround(qnh.GetPascal());
    const auto buffer =
      fmt::format("PLXV0,QNH,W,{}", qnh_pascal);
    PortWriteNMEA(port, buffer.c_str(), env);
  }

  /**
   * Set the volume setting of the vario
   * @param volume 0 - 100 %
   */
  static inline void
  SetVolume(Port &port, OperationEnvironment &env, unsigned volume)
  {
    const auto buffer =
      fmt::format("PLXV0,VOL,W,{:.1f}", (double)volume);
    PortWriteNMEA(port, buffer.c_str(), env);
  }

  /**
   * Set the elevation setting of the vario
   * @param elevation elevation in meters
   */
  static inline void
  SetElevation(Port &port, OperationEnvironment &env, int elevation)
  {
    const auto buffer =
      fmt::format("PLXV0,ELEVATION,W,{}", elevation);
    PortWriteNMEA(port, buffer.c_str(), env);
  }

  /**
   * Send pilotevent to vario 
   * (needs S10x/S8x firmware 8.01 or newer)
   */
  static inline void
  PutPilotEvent(OperationEnvironment &env, Port &port)
  {
    const char *sentence = "PFLAI,PILOTEVENT";

    PortWriteNMEA(port, sentence, env);
  }

  /**
   * Set only the pilot weight in POLAR command, leaving all other fields empty
   * @param pilot_weight crew mass (kg)
   */
  static inline void
  SetPilotWeight(Port &port, OperationEnvironment &env, double pilot_weight)
  {
    /* POLAR format: PLXV0,POLAR,W,<a>,<b>,<c>,<polar load>,
       <polar weight>,<max weight>,<empty weight>,<pilot weight>,
       <name>,<stall> */
    /* Leave all fields empty except pilot_weight */
    const auto buffer =
      fmt::format("PLXV0,POLAR,W,,,,,,,,{:.2f},,", pilot_weight);
    PortWriteNMEA(port, buffer.c_str(), env);
  }

  /**
   * Set only the empty weight in POLAR command, leaving all other fields empty
   * @param empty_weight empty mass (kg)
   */
  static inline void
  SetEmptyWeight(Port &port, OperationEnvironment &env, double empty_weight)
  {
    /* POLAR format: PLXV0,POLAR,W,<a>,<b>,<c>,<polar load>,
       <polar weight>,<max weight>,<empty weight>,<pilot weight>,
       <name>,<stall> */
    /* Leave all fields empty except empty_weight */
    const auto buffer =
      fmt::format("PLXV0,POLAR,W,,,,,,,{:.2f},,,", empty_weight);
    PortWriteNMEA(port, buffer.c_str(), env);
  }
}
