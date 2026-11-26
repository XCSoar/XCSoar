// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceFile.hpp"
#include "FakeLogFile.hpp"
#include "LocalPath.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "io/OutputStream.hxx"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "TestUtil.hpp"
#include "util/SpanCast.hxx"
#include "util/StringAPI.hxx"

#include <string_view>

#include <stdlib.h>

static bool
WriteTextFile(Path path, const char *content) noexcept
try {
  FileOutputStream out(path, FileOutputStream::Mode::CREATE);
  out.Write(AsBytes(std::string_view(content)));
  out.Commit();
  return true;
} catch (...) {
  return false;
}

static bool
FileContainsLine(Path path, const char *expected)
{
  FileLineReaderA reader(path);
  char *line;
  while ((line = reader.ReadLine()) != nullptr)
    if (StringIsEqual(line, expected))
      return true;

  return false;
}

/** create a fresh data directory and return the state file path */
static AllocatedPath
SetUpDataPath(char *template_path)
{
  ok1(mkdtemp(template_path) != nullptr);
  SetSingleDataPath(Path(template_path));
  return LocalPath("device.xcd");
}

static void
TestDefaultsWithoutFile()
{
  char template_path[] = "/tmp/xcsoar-state-XXXXXX";
  const auto state_path = SetUpDataPath(template_path);

  DeviceFile::Load();
  ok1(DeviceFile::GetStartupMode() == DeviceFile::StartupMode::ASK);

  /* nothing was modified, so Save() must not create the file */
  DeviceFile::Save();
  ok1(!File::Exists(state_path));
}

static void
TestRoundTrip()
{
  char template_path[] = "/tmp/xcsoar-state-XXXXXX";
  const auto state_path = SetUpDataPath(template_path);

  DeviceFile::Load();
  DeviceFile::SetStartupMode(DeviceFile::StartupMode::FLY);
  ok1(File::Exists(state_path));
  ok1(FileContainsLine(state_path, "StartupMode=\"1\""));

  DeviceFile::Load();
  ok1(DeviceFile::GetStartupMode() == DeviceFile::StartupMode::FLY);

  DeviceFile::SetStartupMode(DeviceFile::StartupMode::SIMULATOR);
  DeviceFile::Load();
  ok1(DeviceFile::GetStartupMode() == DeviceFile::StartupMode::SIMULATOR);

  /* SetStartupMode() has already saved; deleting the file and calling
     Save() again must not resurrect it */
  ok1(File::Delete(state_path));
  DeviceFile::Save();
  ok1(!File::Exists(state_path));
}

static void
TestPreservesUnknownKeys()
{
  char template_path[] = "/tmp/xcsoar-state-XXXXXX";
  const auto state_path = SetUpDataPath(template_path);

  ok1(WriteTextFile(state_path,
                    "FutureKey=\"hello\"\n"
                    "StartupMode=\"2\"\n"));

  DeviceFile::Load();
  ok1(DeviceFile::GetStartupMode() == DeviceFile::StartupMode::SIMULATOR);

  /* rewriting the file must keep keys this XCSoar version does not
     know about */
  DeviceFile::SetStartupMode(DeviceFile::StartupMode::FLY);
  ok1(FileContainsLine(state_path, "FutureKey=\"hello\""));
  ok1(FileContainsLine(state_path, "StartupMode=\"1\""));
}

static void
TestInvalidValues()
{
  char template_path[] = "/tmp/xcsoar-state-XXXXXX";
  const auto state_path = SetUpDataPath(template_path);

  ok1(WriteTextFile(state_path, "StartupMode=\"99\"\n"));
  DeviceFile::Load();
  ok1(DeviceFile::GetStartupMode() == DeviceFile::StartupMode::ASK);

  ok1(WriteTextFile(state_path, "StartupMode=\"garbage\"\n"));
  DeviceFile::Load();
  ok1(DeviceFile::GetStartupMode() == DeviceFile::StartupMode::ASK);
}

int
main()
{
  SetFakeLogFileQuiet(true);

  plan_tests(20);
  TestDefaultsWithoutFile();
  TestRoundTrip();
  TestPreservesUnknownKeys();
  TestInvalidValues();
  return exit_status();
}
