// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TopographyFile.hpp"
#include "util/NonCopyable.hpp"

#include <forward_list>

class Path;
class WindowProjection;
class NLineReader;
struct zzip_dir;

/**
 * Class used to manage and render vector topography layers
 */
class TopographyStore : private NonCopyable {
  std::forward_list<TopographyFile> files;

  /**
   * This number is incremented each time this object is modified.
   */
  unsigned serial = 0;

public:
  TopographyStore() noexcept;
  ~TopographyStore() noexcept;

  /**
   * Returns a serial for the current state.  The serial gets
   * incremented each time the list of warnings is modified.
   */
  unsigned GetSerial() const noexcept {
    return serial;
  }

  auto begin() const noexcept {
    return files.begin();
  }

  auto end() const noexcept {
    return files.end();
  }

  /**
   * @see TopographyFile::GetNextScaleThreshold()
   */
  [[gnu::pure]]
  double GetNextScaleThreshold(double map_scale) const noexcept;

  /**
   * @param max_update the maximum number of files updated in this
   * call
   * @return the number of files which were updated
   */
  unsigned ScanVisibility(const WindowProjection &m_projection,
                          unsigned max_update=1024) noexcept;

  /**
   * Load all shapes of all files into memory.  For debugging
   * purposes.
   */
  void LoadAll() noexcept;

  void Load(NLineReader &reader,
            Path directory, struct zzip_dir *zdir = nullptr) noexcept;
  void Reset() noexcept;
};
