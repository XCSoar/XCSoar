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

#ifndef XCSOAR_IGC_PARSER_HPP
#define XCSOAR_IGC_PARSER_HPP

struct IGCFix;
struct IGCHeader;
struct IGCExtensions;
struct IGCDeclarationHeader;
struct IGCDeclarationTurnpoint;
struct BrokenDate;
struct BrokenTime;
struct GeoPoint;

/**
 * Parse an IGC "A" record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseHeader(const char *line, IGCHeader &header);

/**
 * Parse an IGC "HFDTE" record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseDateRecord(const char *line, BrokenDate &date);

/**
 * Parse an IGC "I" record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseExtensions(const char *buffer, IGCExtensions &extensions);

/**
 * Parse a location in IGC file format. (DDMMmmm[N/S]DDDMMmmm[E/W])
 *
 * @return true on success, false if the location was not recognized
 */
bool
IGCParseLocation(const char *buffer, GeoPoint &location);

/**
 * Parse an IGC "B" record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseFix(const char *buffer, const IGCExtensions &extensions, IGCFix &fix);

/**
 * Parse a time in IGC file format (HHMMSS).
 *
 * @return true on success, false if the time was not recognized
 */
bool
IGCParseTime(const char *buffer, BrokenTime &time);

/**
 * Parse an IGC "C" header record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseDeclarationHeader(const char *line, IGCDeclarationHeader &header);

/**
 * Parse an IGC "C" turnpoint record.
 *
 * @return true on success, false if the line was not recognized
 */
bool
IGCParseDeclarationTurnpoint(const char *line, IGCDeclarationTurnpoint &tp);

#endif
