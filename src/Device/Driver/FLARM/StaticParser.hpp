// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
