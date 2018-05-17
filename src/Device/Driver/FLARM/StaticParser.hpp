/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

class NMEAInputLine;
struct FlarmError;
struct FlarmVersion;
struct FlarmStatus;
struct TrafficList;

/**
 * Parses a PFLAE sentence (self-test results).
 */
void
ParsePFLAE(NMEAInputLine &line, FlarmError &error, double clock);

/**
 * Parses a PFLAV sentence (version information).
 */
void
ParsePFLAV(NMEAInputLine &line, FlarmVersion &version, double clock);

/**
 * Parses a PFLAU sentence
 * (Operating status and priority intruder and obstacle data)
 *
 * @param line A NMEAInputLine instance that can be used for parsing
 * @see http://flarm.com/support/manual/FLARM_DataportManual_v5.00E.pdf
 */
void
ParsePFLAU(NMEAInputLine &line, FlarmStatus &flarm, double clock);

/**
 * Parses a PFLAA sentence
 * (Data on other moving objects around)
 *
 * @param line A NMEAInputLine instance that can be used for parsing
 * @see http://flarm.com/support/manual/FLARM_DataportManual_v5.00E.pdf
 */
void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, double clock);

#endif
