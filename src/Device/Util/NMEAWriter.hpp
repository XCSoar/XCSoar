// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/** @file
 *
 * Internal header for driver implementations.
 */

#pragma once

#include "util/Compiler.h"

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

/**
 * Formats and writes one line of NMEA data.
 *
 * The formatted line is without asterisk, checksum and newline.
 *
 * Throws on error.
 */
void
PortWriteNMEAFormat(Port &port, OperationEnvironment &env,
                    const char *format, ...) gcc_printf(3, 4);
