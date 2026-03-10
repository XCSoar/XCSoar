// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "TestUtil.hpp"

#include <cstdint>

static constexpr auto N_CONTENT_TYPES =
  static_cast<uint8_t>(FileType::COUNT) - 1;

static void
TestDefaultDirs()
{
  // Every content type must return either nullptr or a valid
  // relative path (never absolute, never empty)
  for (uint8_t i = 1;
       i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    auto dir = GetFileTypeDefaultDir(static_cast<FileType>(i));
    if (dir != nullptr) {
      ok(!dir.empty() && !dir.IsAbsolute(),
         "FileType %u dir is relative", unsigned(i));
    } else {
      ok(true, "FileType %u has no dir", unsigned(i));
    }
  }

  // Sentinel types return nullptr
  ok1(GetFileTypeDefaultDir(FileType::UNKNOWN) == nullptr);
  ok1(GetFileTypeDefaultDir(FileType::COUNT) == nullptr);
}

static void
TestPatterns()
{
  // Every content type should have at least one non-empty pattern
  for (uint8_t i = 1;
       i < static_cast<uint8_t>(FileType::COUNT); ++i) {
    const char *p = GetFileTypePatterns(static_cast<FileType>(i));
    ok(p != nullptr && p[0] != '\0',
       "FileType %u has patterns", unsigned(i));
  }

  // Sentinel types return empty pattern
  const char *p = GetFileTypePatterns(FileType::UNKNOWN);
  ok1(p != nullptr && p[0] == '\0');

  p = GetFileTypePatterns(FileType::COUNT);
  ok1(p != nullptr && p[0] == '\0');
}

int main()
{
  plan_tests(N_CONTENT_TYPES + 2 + N_CONTENT_TYPES + 2);

  TestDefaultDirs();
  TestPatterns();

  return exit_status();
}
