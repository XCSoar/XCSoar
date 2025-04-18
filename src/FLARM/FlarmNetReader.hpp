// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;
class FlarmNetDatabase;
class NLineReader;

/**
 * Handles parsing of the FlarmNet.org file
 */
namespace FlarmNetReader
{
  /**
   * Reads all records from the FlarmNet.org file
   *
   * @param reader A NLineReader instance to read from
   * @return the number of records read from the file
   */
  unsigned LoadFile(NLineReader &reader, FlarmNetDatabase &database);

  /**
   * Reads all records from the FlarmNet.org file
   *
   * @param path the path of the file
   * @return the number of records read from the file
   */
  unsigned LoadFile(Path path, FlarmNetDatabase &database);
};
