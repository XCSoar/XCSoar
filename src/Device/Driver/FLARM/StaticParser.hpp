// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once
#include <cstdint>

/** \file
 * Specific parsers for Flarm NMEA records.
 * @see https://flarm.com/wp-content/uploads/man/FTD-012-Data-Port-Interface-Control-Document-ICD.pdf
 */

class TimeStamp;
class NMEAInputLine;
struct FlarmError;
struct FlarmProgress;
struct FlarmState;
struct FlarmVersion;
struct FlarmStatus;
struct TrafficList;

struct RangeFilter {
  uint16_t horizontal;
  uint16_t vertical;
};

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
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, TimeStamp clock, RangeFilter &range) noexcept;

/**
 * Parses a PFLAJ sentence (flight and IGC recording state).
 *
 * @param line The Flarm NMEA record to parse.
 * @param state The current Flarm state which will be updated.
 * @param clock The time now.
 */
void
ParsePFLAJ(NMEAInputLine &line, FlarmState &state,
           TimeStamp clock) noexcept;

/**
 * Parses a PFLAQ sentence (operations progress information).
 *
 * @param line The Flarm NMEA record to parse.
 * @param progress The current Flarm progress which will be updated.
 * @param clock The time now.
 */
void
ParsePFLAQ(NMEAInputLine &line, FlarmProgress &progress,
           TimeStamp clock) noexcept;

/**
 * Parses a PFLAM sentence (messaging data).
 *
 * @param line The Flarm NMEA record to parse.
 */
void
ParsePFLAM(NMEAInputLine &line) noexcept;
