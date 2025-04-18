// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/** @file
 *
 * Internal header for driver implementations.
 */

#pragma once

class Port;
class OperationEnvironment;

/**
 * Writes one line of NMEA data.
 *
 * Throws on error.
 *
 * @param port the port to write to
 * @param line the line without asterisk, checksum and newline
 */
void
PortWriteNMEA(Port &port, const char *line, OperationEnvironment &env);
