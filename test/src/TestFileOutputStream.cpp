// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TestUtil.hpp"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "util/SpanCast.hxx"
#include "util/StringAPI.hxx"

#include <fcntl.h>
#include <stdlib.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

static bool
WriteRaw(Path path, std::string_view text) noexcept
{
  const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) return false;

  const auto nbytes   = write(fd, text.data(), text.size());
  const bool complete = nbytes == static_cast<ssize_t>(text.size());
  return close(fd) == 0 && complete;
}

static bool
ReadEquals(Path path, const char *expected) noexcept
{
  char buffer[64];
  return File::ReadString(path, buffer, sizeof(buffer)) &&
         StringIsEqual(buffer, expected);
}

static bool
Replace(Path path, std::string_view text, FileOutputStream::Mode mode) noexcept
{
  try {
    FileOutputStream file(path, mode);
    file.Write(AsBytes(text));
    file.Commit();
    return true;
  } catch (...) {
    return false;
  }
}

/**
 * Removes the directory even when a test left it non-writable.
 */
class TempDirectory
{
  AllocatedPath path;

public:
  TempDirectory() noexcept
  {
    char tmpl[] = "/tmp/xcsoar-fos-XXXXXX";
    if (mkdtemp(tmpl) != nullptr) path = Path(tmpl);
  }

  ~TempDirectory() noexcept
  {
    if (path == nullptr) return;

    chmod(path.c_str(), 0700);
    File::Delete(AllocatedPath::Build(path, "data.txt"));
    File::Delete(AllocatedPath::Build(path, "data.txt.tmp"));
    rmdir(path.c_str());
  }

  bool IsDefined() const noexcept
  {
    return path != nullptr;
  }

  Path Get() const noexcept
  {
    return path;
  }
};

static void
TestCommit(Path dir)
{
  const auto path = AllocatedPath::Build(dir, "data.txt");
  ok1(WriteRaw(path, "original"));
  ok1(Replace(path, "committed", FileOutputStream::Mode::CREATE));
  ok1(ReadEquals(path, "committed"));
  ok1(!File::Exists(path + ".tmp"));
}

static void
TestCancel(Path dir)
{
  const auto path = AllocatedPath::Build(dir, "data.txt");
  ok1(WriteRaw(path, "original"));

  bool threw = false;
  try {
    FileOutputStream file(path);
    file.Write(AsBytes(std::string_view{"discarded"}));
  } catch (...) {
    threw = true;
  }

  ok1(!threw);
  ok1(ReadEquals(path, "original"));
  ok1(!File::Exists(path + ".tmp"));
}

static void
TestCreateVisible(Path dir)
{
  const auto path = AllocatedPath::Build(dir, "data.txt");
  ok1(WriteRaw(path, "original"));
  ok1(Replace(path, "visible", FileOutputStream::Mode::CREATE_VISIBLE));
  ok1(ReadEquals(path, "visible"));
}

static void
TestUnwritableDirectory(Path dir)
{
  /* Five results: two setup writes, the exception, and both files. */
  if (geteuid() == 0) {
    skip(5, 1, "root ignores directory write permission");
    return;
  }

  const auto path = AllocatedPath::Build(dir, "data.txt");
  const auto tmp  = path + ".tmp";
  ok1(WriteRaw(path, "original"));
  ok1(WriteRaw(tmp, "stale"));

  if (chmod(dir.c_str(), 0555) != 0) {
    skip(3, 0, "chmod failed");
    return;
  }

  struct Restore
  {
    Path directory;
    ~Restore() noexcept
    {
      chmod(directory.c_str(), 0700);
    }
  } restore{dir};

  bool threw = false;
  try {
    FileOutputStream file(path);
    file.Write(AsBytes(std::string_view{"lost"}));
    file.Commit();
  } catch (...) {
    threw = true;
  }

  ok1(threw);
  ok1(ReadEquals(path, "original"));
  ok1(ReadEquals(tmp, "stale"));
}

int
main()
{
  plan_tests(16);

  TempDirectory dir;
  if (!dir.IsDefined()) {
    skip(16, 0, "mkdtemp failed");
    return exit_status();
  }

  TestCommit(dir.Get());
  TestCancel(dir.Get());
  TestCreateVisible(dir.Get());
  TestUnwritableDirectory(dir.Get());
  return exit_status();
}
