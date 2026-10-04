// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;
class Port;
class RecordedFlightList;
struct RecordedFlightInfo;
class OperationEnvironment;

namespace Nano {
  /**
   * Parse a logbook entry, the part of a "PLXVC,LOGBOOK,A" sentence
   * after that prefix.  An end time the logger did not record is
   * returned as BrokenTime::Invalid().
   */
  bool ParseLogbookContent(const char *line, RecordedFlightInfo &info);

  bool ReadFlightList(Port &port, RecordedFlightList &flight_list,
                      OperationEnvironment &env);

  bool DownloadFlight(Port &port, const RecordedFlightInfo &flight,
                      Path path, OperationEnvironment &env);
}
