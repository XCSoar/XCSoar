// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/MemoryReader.hxx"
#include "io/StringOutputStream.hxx"
#include "io/TarArchive.hpp"
#ifdef HAVE_POSIX
#include "io/TarBackup.hpp"
#include "Operation/Operation.hpp"
#include "SystemTools.hpp"
#include "system/FileUtil.hpp"
#endif
#include "TestUtil.hpp"

#include <algorithm>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <string>

#ifdef HAVE_POSIX
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

static void
TestRoundTripMaxNameLength()
{
  const std::string payload = "abc";
  const std::string name(100, 'a');

  MemoryReader input(std::as_bytes(std::span(payload.data(), payload.size())));
  StringOutputStream archive_output;
  TarWriter writer(archive_output);
  writer.Add(name, input, payload.size(), 04755);
  writer.Finish();

  const std::string archive = archive_output.GetValue();
  MemoryReader archive_input(
    std::as_bytes(std::span(archive.data(), archive.size())));
  TarReader reader(archive_input);

  std::string parsed_name;
  uint64_t parsed_size = 0;
  unsigned parsed_mode = 0;
  ok1(reader.Next(parsed_name, parsed_size, parsed_mode));
  ok1(parsed_name == name);
  ok1(parsed_size == payload.size());
  ok1(parsed_mode == 0755);

  StringOutputStream extracted;
  reader.ReadData(extracted);
  ok1(extracted.GetValue() == payload);
  ok1(!reader.Next(parsed_name, parsed_size));
}

static void
TestRejectLongName()
{
  const std::string payload = "abc";
  const std::string name(101, 'b');

  MemoryReader input(std::as_bytes(std::span(payload.data(), payload.size())));
  StringOutputStream archive_output;
  TarWriter writer(archive_output);

  bool thrown = false;
  try {
    writer.Add(name, input, payload.size());
  } catch (const std::invalid_argument &) {
    thrown = true;
  }

  ok1(thrown);
}

#ifdef HAVE_POSIX

static void
TestRestoreUserTools(mode_t restore_umask, mode_t existing_mode = 0)
{
  char directory_template[] = "/tmp/xcsoar-restore-tools-XXXXXX";
  char resolved_directory[PATH_MAX];
  const char *const temporary = mkdtemp(directory_template);
  const char *const directory = temporary != nullptr
    ? realpath(temporary, resolved_directory) : nullptr;
  ok1(directory != nullptr);
  if (directory == nullptr) {
    skip(8, 0, "mkdtemp failed");
    return;
  }

  StringOutputStream archive_output;
  TarWriter writer(archive_output);
  const auto add = [&writer](std::string_view name, std::string_view contents,
                            unsigned mode) {
    MemoryReader input(std::as_bytes(std::span(contents)));
    writer.Add(name, input, contents.size(), mode);
  };
  add("tools/xcsoar-test-restored-command", "#!/bin/sh\nexit 0\n", 0777);
  add("tools/xcsoar-test-restored-command.tool",
      "name=Restored command\n"
      "description=Test restored command\n"
      "instructions=Run the command.\n"
      "mode=job\n", 0666);
  add("logs/test.igc", "test data\n", 0755);
  writer.Finish();

  const std::string archive = archive_output.GetValue();
  const auto tools_directory = AllocatedPath::Build(Path{directory}, "tools");
  if (existing_mode != 0) {
    Directory::Create(tools_directory);
    chmod(tools_directory.c_str(), existing_mode);
  }
  MemoryReader archive_input(std::as_bytes(std::span(archive)));
  NullOperationEnvironment env;
  unsigned restored_files = 0, failed_files = 0;
  std::string error;
  const mode_t previous_umask = umask(restore_umask);
  const bool restored = RestoreBackup(archive_input, Path{directory}, {}, env,
                                      restored_files, failed_files, error);
  umask(previous_umask);
  ok1(restored);
  ok1(restored_files == 3);
  ok1(failed_files == 0);

  const auto executable =
    AllocatedPath::Build(tools_directory, "xcsoar-test-restored-command");
  const auto descriptor = executable.WithSuffix(".tool");
  const auto data = AllocatedPath::Build(Path{directory}, "logs/test.igc");
  struct stat st;
  ok1(stat(tools_directory.c_str(), &st) == 0 &&
      S_ISDIR(st.st_mode) && (st.st_mode & 07777) ==
        (existing_mode != 0 ? (existing_mode & 0755) | 0700 : 0755));
  ok1(stat(executable.c_str(), &st) == 0 && (st.st_mode & 07777) == 0755);
  ok1(stat(descriptor.c_str(), &st) == 0 && (st.st_mode & 07777) == 0644);
  ok1(stat(data.c_str(), &st) == 0 && (st.st_mode & 0111) == 0);

  const auto tools = SystemTools::LoadInstalled({}, tools_directory);
  ok1(std::any_of(tools.begin(), tools.end(), [](const auto &tool) {
    return tool.id == "xcsoar-test-restored-command" &&
      tool.origin == SystemTools::Origin::USER;
  }));

  Directory::Remove(Path{directory});
}

static void
TestRejectSymlinkedTools()
{
  char directory_template[] = "/tmp/xcsoar-restore-symlink-XXXXXX";
  const char *const directory = mkdtemp(directory_template);
  ok1(directory != nullptr);
  if (directory == nullptr) {
    skip(5, 0, "mkdtemp failed");
    return;
  }

  const auto outside = AllocatedPath::Build(Path{directory}, "outside");
  const auto destination = AllocatedPath::Build(Path{directory}, "data");
  Directory::Create(outside);
  Directory::Create(destination);
  const auto tools = AllocatedPath::Build(destination, "tools");
  ok1(symlink(outside.c_str(), tools.c_str()) == 0);

  StringOutputStream archive_output;
  TarWriter writer{archive_output};
  const std::string payload = "#!/bin/sh\nexit 0\n";
  MemoryReader input{std::as_bytes(std::span{payload})};
  writer.Add("tools/command", input, payload.size(), 0755);
  writer.Finish();
  const auto archive = archive_output.GetValue();
  MemoryReader archive_input{std::as_bytes(std::span{archive})};
  NullOperationEnvironment env;
  unsigned restored = 0, failed = 0;
  std::string error;
  ok1(!RestoreBackup(archive_input, destination, {}, env,
                     restored, failed, error));
  ok1(restored == 0);
  ok1(!error.empty());
  ok1(!File::Exists(AllocatedPath::Build(outside, "command")));

  unlink(tools.c_str());
  Directory::Remove(Path{directory});
}

#endif

int
main()
{
#ifdef HAVE_POSIX
  plan_tests(7 + 5 * 9 + 6);
#else
  plan_tests(7);
#endif
  TestRoundTripMaxNameLength();
  TestRejectLongName();
#ifdef HAVE_POSIX
  TestRestoreUserTools(0002);
  TestRestoreUserTools(0000);
  TestRestoreUserTools(0002, 0777);
  TestRestoreUserTools(0002, 0700);
  TestRestoreUserTools(0002, 02750);
  TestRejectSymlinkedTools();
#endif
  return exit_status();
}
