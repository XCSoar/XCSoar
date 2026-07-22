// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class NMEAInputLine;
struct NMEAInfo;
struct DeviceInfo;

/* LXWP parsers exposed for reuse by sibling drivers (e.g. LX160).
   The full LX driver wires these into LXDevice::ParseNMEA in Parser.cpp;
   the same helpers can be invoked directly by simpler drivers that share
   the LXNAV NMEA dialect but not the rest of the LXDevice surface. */

namespace LX {

/**
 * Read six LXWP0 vario samples and apply the 5th-order low-pass FIR
 * filter used by LX EOS.
 */
bool
ReadFilteredLXWP0Vario(NMEAInputLine &line, double &vario);

/**
 * @param provide_altitude_vario When false, skip pressure altitude and
 * TE vario (used once $PLXVF is active on LXNAV varios).
 */
bool
LXWP0(NMEAInputLine &line, NMEAInfo &info,
      bool provide_altitude_vario=true);

void
LXWP1(NMEAInputLine &line, DeviceInfo &device);

bool
LXWP2(NMEAInputLine &line, NMEAInfo &info);

bool
LXWP3(NMEAInputLine &line, NMEAInfo &info);

} // namespace LX
