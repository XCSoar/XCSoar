// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class NMEAInputLine;
struct NMEAInfo;

/* LXWP parsers exposed for reuse by sibling drivers (e.g. LX160).
   The full LX driver wires these into LXDevice::ParseNMEA in Parser.cpp;
   the same helpers can be invoked directly by simpler drivers that share
   the LXNAV NMEA dialect but not the rest of the LXDevice surface. */

namespace LX {

bool
LXWP0(NMEAInputLine &line, NMEAInfo &info);

bool
LXWP2(NMEAInputLine &line, NMEAInfo &info);

bool
LXWP3(NMEAInputLine &line, NMEAInfo &info);

} // namespace LX
