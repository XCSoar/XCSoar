// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Info.hpp"

/**
 * GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>
 * 
 * Recommended Minimum Navigation Information
 * NMEA 2.00 Standard
 * 
 * <1>  UTC Time of position fix, hhmmss format
 * <2>  Status, A = Valid position, V = NAV receiver warning
 * <3>  Latitude, ddmm.mmm format (leading zeros will be transmitted)
 * <4>  Latitude hemisphere, N or S
 * <5>  Longitude, dddmm.mmm format (leading zeros will be transmitted)
 * <6>  Longitude hemisphere, E or W
 * <7>  Speed over ground, 0.0 to 999.9 knots
 * <8>  Course over ground 000.0 to 359.9 degrees, true
 *      (leading zeros will be transmitted)
 * <9>  UTC date of position fix, ddmmyy format
 * <10> Magnetic variation, 000.0 to 180.0 degrees
 *      (leading zeros will be transmitted)
 * <11> Magnetic variation direction, E or W
 *      (westerly variation adds to true course)
 */
void
FormatGPRMC(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept;

/**
 * GPGGA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>,<14>
 * 
 * Global Positioning System Fix Data
 * NMEA 2.00 Standard
 * 
 * <1>  Universal Time Coordinated (UTC), hhmmss.ss
 * <2>  Latitude,ddmm.mmm format (leading zeros will be transmitted)
 * <3>  Latitude hemisphere, N or S
 * <4>  Longitude,dddmm.mmm format (leading zeros will be transmitted)
 * <5>  Longitude hemisphere, E or W
 * <6>  GPS Quality Indicator,
 *      0 - fix not available,
 *      1 - GPS fix,
 *      2 - Differential GPS fix
 *      (values above 2 are 2.3 features)
 *      3 = PPS fix
 *      4 = Real Time Kinematic
 *      5 = Float RTK
 *      6 = estimated (dead reckoning)
 *      7 = Manual input mode
 *      8 = Simulation mode
 * <7>  Number of satellites in view, 00 - 12
 * <8>  Horizontal Dilution of precision (meters)
 * <9>  Antenna Altitude above/below mean-sea-level (geoid) (in meters)
 * <10> Units of antenna altitude, meters
 * <11> Geoidal separation, the difference between the WGS-84 earth
 *      ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level
 *      below ellipsoid
 *      (not implemented)
 * <12> Units of geoidal separation, meters 
 *      (not implemented)
 * <13> Age of differential GPS data, time in seconds since last SC104
 *      type 1 or 9 update, null field when DGPS is not used 
 *      (not implemented)
 * <14> Differential reference station ID, 0000-1023
 */
void
FormatGPGGA(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept;

/*
 * GPGSA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>,<14>,<15>,<16>,<17>
 * 
 * GPS DOP and active satellites
 * NMEA 2.00 Standard
 * 
 *  <1> Selection mode
 *        M=Manual, forced to operate in 2D or 3D
 *        A=Automatic, 3D/2D
 *  <2> Mode (1 = no fix, 2 = 2D fix, 3 = 3D fix)
 *  <3> ID of 1st satellite used for fix
 *  <4> ID of 2nd satellite used for fix
 *  ...
 *  <14> ID of 12th satellite used for fix
 *  <15> PDOP
 *  <16> HDOP
 *  <17> VDOP
 */
void
FormatGPGSA(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept;

/*
 * PGRMZ,<1>,<2>,<3>
 *
 * Garmin proprietary NMEA sentence for altitude information
 *
 *  <1> Barometric altitude
 *  <2> Unit of altitude (f = feet, m = metres)
 *  <3> Mode (1 = no fix, 2 = 2D fix, 3 = 3D fix)
 */
void
FormatPGRMZ(char *buffer, size_t buffer_size, const NMEAInfo &info) noexcept;

/** Returns latitude, ddmm.mmm format. */
void
FormatLatitude(char *buffer, size_t buffer_size, Angle latitude) noexcept;

/** Returns longitude, dddmm.mmm format. */
void
FormatLongitude(char *buffer, size_t buffer_size, Angle longitude) noexcept;

/** Returns magnetic variation including variation direction (west or east) */
void
FormatVariation(char *buffer, size_t buffer_size, Angle variation) noexcept;

/** Returns time, hhmmss.00 format. */
void
FormatTime(char *buffer, size_t buffer_size, BrokenDateTime time) noexcept;

/** Returns date, ddmmyy format. */
void
FormatDate(char *buffer, size_t buffer_size, BrokenDateTime time) noexcept;
