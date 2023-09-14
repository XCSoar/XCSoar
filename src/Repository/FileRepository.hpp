// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AvailableFile.hpp"

#include <list>

/**
 * The announced contents of a remote file repository.  These are the
 * files that can be downloaded by the XCSoar user.
 */
struct FileRepository {
  typedef std::list<AvailableFile> FileList;
  typedef FileList::const_iterator const_iterator;

  FileList files;

  void Clear() {
    files.clear();
  }

  [[gnu::pure]]
  const_iterator begin() const {
    return files.begin();
  }

  [[gnu::pure]]
  const_iterator end() const {
    return files.end();
  }

  [[gnu::pure]]
  const AvailableFile *FindByName(const char *name) const;
};
