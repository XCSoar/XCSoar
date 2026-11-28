// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/Stamp.hpp"

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
 * The length of every record laid out by an "I" (B records) or "J"
 * (K records) declaration: the fixed part, or the last byte an
 * extension occupies, whichever is further.  The fixed part is 35 for
 * a B record and 7 for a K record.
 */
[[gnu::pure]]
unsigned
IGCRecordLength(const IGCExtensions &extensions, unsigned fixed) noexcept;

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
 * B-record summary of one IGC file: how many B lines it has, and the
 * time of day of the first and last B line whose time parses.
 */
struct IGCRecordingSpan {
  unsigned fix_lines = 0;
  TimeStamp start_time = TimeStamp::Undefined();
  TimeStamp end_time = TimeStamp::Undefined();
};

/**
 * Count one IGC line. A B record increments #IGCRecordingSpan::fix_lines
 * even when its time of day is missing or not plausible. The first
 * plausible time becomes the start, and each later one the end.
 */
void
IGCScanBRecord(IGCRecordingSpan &span, const char *line) noexcept;

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
