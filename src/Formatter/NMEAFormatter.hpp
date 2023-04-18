// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Info.hpp"

/**
 * GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>
 * 
 * <1>  UTC Time of position fix, hhmmss format
 * <2>  Status, A = Valid position, V = NAV receiver warning
 * <3>  Latitude,ddmm.mmm format (leading zeros will be transmitted)
 * <4>  Latitude hemisphere, N or S
 * <5>  Longitude,dddmm.mmm format (leading zeros will be transmitted)
 * <6>  Longitude hemisphere, E or W
 * <7>  Speed over ground, 0.0 to 999.9 knots
 * <8>  Course over ground 000.0 to 359.9 degrees, true
 *      (leading zeros will be transmitted)
 * <9>  UTC date of position fix, ddmmyy format
 */
bool
FormatGPRMC(char *buffer, size_t buffer_size, const NMEAInfo &info);

/** Returns latitude, ddmm.mmm format. */
void
FormatLatitude(char *buffer, size_t buffer_size, Angle latitude);

/** Returns longitude, dddmm.mmm format. */
void
FormatLongitude(char *buffer, size_t buffer_size, Angle longitude);
