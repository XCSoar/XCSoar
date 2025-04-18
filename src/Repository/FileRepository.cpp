// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileRepository.hpp"

#include <algorithm>

struct AvailableFileNameComparator {
  const char *name;

  [[gnu::pure]]
  bool operator()(const AvailableFile &file) const {
    return file.name.compare(name) == 0;
  }
};

const AvailableFile *
FileRepository::FindByName(const char *name) const
{
  const AvailableFileNameComparator cmp{name};
  auto i = std::find_if(files.begin(), files.end(), cmp);
  return i != files.end()
    ? &*i
    : NULL;
}
