/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_FLARM_STATIC_PARSER_HPP
#define XCSOAR_FLARM_STATIC_PARSER_HPP

/** \file 
 * Specific parsers for Flarm NMEA records.
 * @see https://flarm.com/wp-content/uploads/man/FTD-012-Data-Port-Interface-Control-Document-ICD.pdf
 */

class TimeStamp;
class NMEAInputLine;
struct FlarmError;
struct FlarmVersion;
struct FlarmStatus;
struct TrafficList;

/**
 * Parses a PFLAE sentence (self-test results).
 * @param line The Flarm NMEA record to parse.
 * @param error The current Flarm error state which will be updated by this
 *              NMEA record.
 * @param clock The time now.
 */

void
ParsePFLAE(NMEAInputLine &line, FlarmError &error, TimeStamp clock) noexcept;

/**
 * Parses a PFLAV sentence (version information).
 * @param line The Flarm NMEA record to parse.
 * @param version The current Flarm version state which will be updated by
 *                this NMEA record.
 * @param clock The time now.
 */
void
ParsePFLAV(NMEAInputLine &line, FlarmVersion &version,
           TimeStamp clock) noexcept;

/**
 * Parses a PFLAU sentence
 * (Operating status and priority intruder and obstacle data)
 *
 * @param line The Flarm NMEA record to parse.
 * @param flarm The current Flarm status which will be updated by this NMEA
 *              record.
 * @param clock The time now.
 */
void
ParsePFLAU(NMEAInputLine &line, FlarmStatus &flarm, TimeStamp clock) noexcept;

/**
 * Parses a PFLAA sentence
 * (Data on other moving objects around)
 *
 * @param line The Flarm NMEA record to parse.
 * @param flarm The current Flarm status which will be updated by this NMEA
 *              record.
 * @param clock The time now.
 */
void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, TimeStamp clock) noexcept;

#endif
