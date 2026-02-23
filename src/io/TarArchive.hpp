// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class Reader;
class OutputStream;

/**
 * Sequential tar archive reader.
 *
 * After Next() returns true, the caller must call either ReadData()
 * or Skip() before calling Next() again.
 */
class TarReader {
  Reader &input;
  uint64_t remaining = 0;

public:
  explicit TarReader(Reader &_input) noexcept
    : input(_input) {}

  /**
   * Advance to the next entry.
   * @return true if an entry was found; false at end-of-archive.
   */
  bool Next(std::string &name, uint64_t &size);

  /** Extract the current entry to the given output stream. */
  void ReadData(OutputStream &out);

  /** Skip the current entry. */
  void Skip();
};

/**
 * Sequential tar archive writer.
 *
 * Call Finish() after the last entry.
 */
class TarWriter {
  OutputStream &output;

public:
  explicit TarWriter(OutputStream &_output) noexcept
    : output(_output) {}

  /** Add a file from the given reader with the specified size. */
  void Add(std::string_view name, Reader &in, uint64_t size);

  /** Write end-of-archive marker. Must be called exactly once. */
  void Finish();
};
