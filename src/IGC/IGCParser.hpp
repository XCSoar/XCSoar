// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
