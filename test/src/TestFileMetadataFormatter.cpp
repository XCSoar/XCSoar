// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/FileMetadataFormatter.hpp"
#include "Formatter/ByteSizeFormatter.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

int main()
{
  plan_tests(10);

  // Use a real file with non-zero size for meaningful assertions
  const Path existing = Path("test/data/01lz1hq1.igc");
  const Path nonexistent = Path("test/data/does_not_exist.xyz");

  // Compute expected size string from the actual file
  char expected_size[32];
  FormatByteSize(expected_size, sizeof(expected_size),
                 File::GetSize(existing));

  // Test Build + Find with an existing file
  {
    FileMetadataFormatter fmt;
    std::vector<Path> paths = {existing};
    fmt.Build(paths);

    const auto *entry = fmt.Find(existing);
    ok1(entry != nullptr);
    ok1(StringIsEqual(entry->size.c_str(), expected_size));
    // ISO 8601: "YYYY-MM-DDTHH:MM:SSZ" — 20 chars with trailing Z
    ok1(entry->last_modified.length() == 20);

    // GetSizeText / GetLastModifiedText convenience
    ok1(StringIsEqual(fmt.GetSizeText(existing), expected_size));
    ok1(fmt.GetLastModifiedText(existing) != nullptr);
  }

  // Test Find returns nullptr for a path not in the cache
  {
    FileMetadataFormatter fmt;
    std::vector<Path> paths = {existing};
    fmt.Build(paths);

    ok1(fmt.Find(nonexistent) == nullptr);
    ok1(fmt.GetSizeText(nonexistent) == nullptr);
    ok1(fmt.GetLastModifiedText(nonexistent) == nullptr);
  }

  // Test with empty input
  {
    FileMetadataFormatter fmt;
    std::vector<Path> paths;
    fmt.Build(paths);
    ok1(fmt.Find(existing) == nullptr);
  }

  // Test with nullptr path in input (should be skipped)
  {
    FileMetadataFormatter fmt;
    std::vector<Path> paths = {nullptr, existing};
    fmt.Build(paths);
    ok1(fmt.Find(existing) != nullptr);
  }

  return exit_status();
}
