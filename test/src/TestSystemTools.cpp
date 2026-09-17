// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemTools.hpp"
#include "io/BufferedLineReader.hpp"
#include "io/MemoryReader.hxx"
#include "TestUtil.hpp"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <span>
#include <stdexcept>
#include <string>

#ifdef HAVE_POSIX
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

static bool
Rejects(char *input)
{
  MemoryReader memory{std::as_bytes(std::span{input, std::strlen(input)})};
  BufferedLineReader reader{memory};
  try {
    SystemTools::ParseDescriptor(reader);
    return false;
  } catch (const std::runtime_error &) {
    return true;
  }
}

#ifdef HAVE_POSIX

static bool
HasTool(const std::vector<SystemTools::InstalledTool> &tools,
        std::string_view id, SystemTools::Origin origin)
{
  return std::any_of(tools.begin(), tools.end(),
                     [id, origin](const auto &tool) {
                       return tool.id == id && tool.origin == origin;
                     });
}

static bool
WriteTestFile(const std::string &path, const char *contents, mode_t mode)
{
  FILE *file = std::fopen(path.c_str(), "w");
  if (file == nullptr)
    return false;

  bool success = std::fputs(contents, file) >= 0;
  success = std::fclose(file) == 0 && success;
  return chmod(path.c_str(), mode) == 0 && success;
}

static void
TestUserCatalogue()
{
  char directory_template[] = "/tmp/xcsoar-tools-XXXXXX";
  char resolved_directory[PATH_MAX];
  const char *const temporary = mkdtemp(directory_template);
  const char *const directory = temporary != nullptr
    ? realpath(temporary, resolved_directory) : nullptr;
  ok1(directory != nullptr);
  if (directory == nullptr) {
    skip(48 + (geteuid() == 0 ? 19 : 0), 0, "mkdtemp failed");
    return;
  }

  // BSD systems inherit the parent's group, not the process's group.
  ok1(chown(directory, static_cast<uid_t>(-1), getegid()) == 0);

  const std::string id{"xcsoar-test-user-command"};
  const std::string executable = std::string{directory} + "/" + id;
  const std::string descriptor = executable + ".tool";
  static constexpr const char descriptor_contents[] =
    "name=Test user command\n"
    "description=Test the user catalogue\n"
    "instructions=Run the test command.\n"
    "mode=job\n";

  ok1(WriteTestFile(executable, "#!/bin/sh\nexit 0\n", 0700));
  ok1(WriteTestFile(descriptor, descriptor_contents, 0600));

  const uid_t owner = geteuid();
  if (owner == 0) {
    const auto root_owned =
      SystemTools::LoadInstalled({}, Path{directory});
    ok1(HasTool(root_owned, id, SystemTools::Origin::USER));

    ok1(chown(directory, 65534, 65534) == 0);
    ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
                 SystemTools::Origin::USER));
    ok1(chown(directory, owner, getegid()) == 0);

    ok1(chown(executable.c_str(), 65534, 65534) == 0);
    ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
                 SystemTools::Origin::USER));
    ok1(chown(executable.c_str(), owner, getegid()) == 0);
  }

  auto tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(HasTool(tools, id, SystemTools::Origin::USER));

  const int cwd_fd = open(".", O_RDONLY);
  bool relative_loaded = false;
  if (cwd_fd >= 0) {
    if (chdir("/tmp") == 0) {
      relative_loaded = HasTool(
        SystemTools::LoadInstalled({},
                                   Path{std::strrchr(directory, '/') + 1}),
        id,
        SystemTools::Origin::USER);
      relative_loaded = fchdir(cwd_fd) == 0 && relative_loaded;
    }
    close(cwd_fd);
  }
  ok1(relative_loaded);

  const std::string nested = std::string{directory} + "/tools";
  ok1(mkdir(nested.c_str(), 0700) == 0 &&
      rename(executable.c_str(), (nested + "/" + id).c_str()) == 0 &&
      rename(descriptor.c_str(), (nested + "/" + id + ".tool").c_str()) == 0);
  chmod(directory, 0777);
  ok1(!HasTool(SystemTools::LoadInstalled({}, Path{nested.c_str()}), id,
               SystemTools::Origin::USER));
  chmod(directory, 0775);
  ok1(HasTool(SystemTools::LoadInstalled({}, Path{nested.c_str()}), id,
               SystemTools::Origin::USER));
  if (owner == 0) {
    const gid_t foreign_group = getegid() == 65534 ? 65533 : 65534;
    ok1(chown(directory, owner, foreign_group) == 0);
    ok1(!HasTool(SystemTools::LoadInstalled({}, Path{nested.c_str()}), id,
                 SystemTools::Origin::USER));
    ok1(chown(directory, owner, getegid()) == 0);
  }
  chmod(directory, 01777);
  ok1(HasTool(SystemTools::LoadInstalled({}, Path{nested.c_str()}), id,
              SystemTools::Origin::USER));
  chmod(directory, 0700);
  ok1(HasTool(SystemTools::LoadInstalled({}, Path{nested.c_str()}), id,
              SystemTools::Origin::USER));
  const std::string link = std::string{directory} + "/link";
  ok1(symlink(".", link.c_str()) == 0);
  const std::string linked = link + "/tools";
  ok1(!HasTool(SystemTools::LoadInstalled({}, Path{linked.c_str()}), id,
               SystemTools::Origin::USER));
  ok1(unlink(link.c_str()) == 0 &&
      rename((nested + "/" + id).c_str(), executable.c_str()) == 0 &&
      rename((nested + "/" + id + ".tool").c_str(), descriptor.c_str()) == 0 &&
      rmdir(nested.c_str()) == 0);

  chmod(directory, 0775);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(HasTool(tools, id, SystemTools::Origin::USER));

  ok1(chmod(directory, 0775) == 0 &&
      chmod(executable.c_str(), 0775) == 0 &&
      chmod(descriptor.c_str(), 0664) == 0);
  ok1(HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
              SystemTools::Origin::USER));
  if (owner == 0) {
    const gid_t foreign_group = getegid() == 65534 ? 65533 : 65534;
    for (const auto &path : {std::string{directory}, executable, descriptor}) {
      ok1(chown(path.c_str(), owner, foreign_group) == 0);
      ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
                   SystemTools::Origin::USER));
      ok1(chown(path.c_str(), owner, getegid()) == 0);
    }
  }
  chmod(descriptor.c_str(), 0600);

  chmod(directory, 0777);
  chmod(executable.c_str(), 0755);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(!HasTool(tools, id, SystemTools::Origin::USER));

  chmod(directory, 0755);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(HasTool(tools, id, SystemTools::Origin::USER));

  chmod(executable.c_str(), 0777);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(!HasTool(tools, id, SystemTools::Origin::USER));

  for (const mode_t mode : {04700, 02700}) {
    ok1(chmod(executable.c_str(), mode) == 0);
    ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
                 SystemTools::Origin::USER));
  }

  chmod(executable.c_str(), 0001);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(!HasTool(tools, id, SystemTools::Origin::USER));

  chmod(executable.c_str(), 0700);
  chmod(descriptor.c_str(), 0666);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(!HasTool(tools, id, SystemTools::Origin::USER));

  chmod(descriptor.c_str(), 0600);
  chmod(directory, 0666);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(!HasTool(tools, id, SystemTools::Origin::USER));

  chmod(directory, 0755);
  // A long line must not hide a duplicate field after a valid prefix.
  const std::string oversized = std::string{descriptor_contents} +
    "future-field=" + std::string(600000, 'x') + "\nmode=exclusive\n";
  ok1(WriteTestFile(descriptor, oversized.c_str(), 0600));
  ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
               SystemTools::Origin::USER));
  ok1(WriteTestFile(descriptor, descriptor_contents, 0600));

  const std::string shell_script = executable + ".sh";
  ok1(rename(executable.c_str(), shell_script.c_str()) == 0);
  tools = SystemTools::LoadInstalled({}, Path{directory});
  ok1(std::any_of(tools.begin(), tools.end(), [&](const auto &tool) {
    return tool.id == id && tool.origin == SystemTools::Origin::USER &&
      tool.executable == Path{shell_script.c_str()};
  }));

  chmod(shell_script.c_str(), 0777);
  ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
               SystemTools::Origin::USER));
  chmod(shell_script.c_str(), 0700);

  ok1(WriteTestFile(executable, "#!/bin/sh\nexit 0\n", 0700));
  ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
               SystemTools::Origin::USER));
  // An unsafe exact-name entry must not enable the alternate script.
  chmod(executable.c_str(), 0000);
  ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
               SystemTools::Origin::USER));
  unlink(executable.c_str());
  ok1(symlink(shell_script.c_str(), executable.c_str()) == 0);
  ok1(!HasTool(SystemTools::LoadInstalled({}, Path{directory}), id,
               SystemTools::Origin::USER));
  unlink(executable.c_str());
  ok1(rename(shell_script.c_str(), executable.c_str()) == 0);

  const auto system_tools = SystemTools::LoadInstalled();
  if (!system_tools.empty()) {
    const auto duplicate = std::string{directory} + "/" +
      system_tools.front().id;
    ok1(WriteTestFile(duplicate, "#!/bin/sh\nexit 0\n", 0700) &&
        WriteTestFile(duplicate + ".tool", descriptor_contents, 0600));
    const auto merged = SystemTools::LoadInstalled({}, Path{directory});
    ok1(!HasTool(merged, system_tools.front().id, SystemTools::Origin::SYSTEM));
    ok1(HasTool(merged, system_tools.front().id, SystemTools::Origin::USER));
    ok1(HasTool(merged, id, SystemTools::Origin::USER));
    chmod(duplicate.c_str(), 0777);
    const auto unsafe = SystemTools::LoadInstalled({}, Path{directory});
    ok1(HasTool(unsafe, system_tools.front().id, SystemTools::Origin::SYSTEM));
    ok1(!HasTool(unsafe, system_tools.front().id, SystemTools::Origin::USER));
    unlink(duplicate.c_str());
    unlink((duplicate + ".tool").c_str());
  } else
    skip(6, 1, "No packaged tool installed");

  for (unsigned i = 0; i < 64; ++i) {
    const std::string extra = std::string{directory} + "/extra-" +
      std::to_string(i);
    if (!WriteTestFile(extra, "#!/bin/sh\nexit 0\n", 0700) ||
        !WriteTestFile(extra + ".tool", descriptor_contents, 0600))
      std::abort();
  }

  // Reject an overflowing personal catalogue without losing system tools.
  ok1(SystemTools::LoadInstalled({}, Path{directory}).size() ==
      system_tools.size());
  unlink((std::string{directory} + "/extra-63.tool").c_str());
  ok1(SystemTools::LoadInstalled({}, Path{directory}).size() ==
      (system_tools.empty() ? 64 : system_tools.size()));
  if (system_tools.size() == 1) {
    const std::string extra = std::string{directory} + "/extra-62";
    const std::string override = std::string{directory} + "/" +
      system_tools.front().id;
    ok1(rename(extra.c_str(), override.c_str()) == 0 &&
        rename((extra + ".tool").c_str(), (override + ".tool").c_str()) == 0);
    const auto full = SystemTools::LoadInstalled({}, Path{directory});
    ok1(full.size() == 64 &&
        HasTool(full, system_tools.front().id, SystemTools::Origin::USER) &&
        !HasTool(full, system_tools.front().id, SystemTools::Origin::SYSTEM));
    unlink(override.c_str());
    unlink((override + ".tool").c_str());
  } else
    skip(2, 1, "Requires one packaged tool");

  for (unsigned i = 0; i < 64; ++i) {
    const std::string extra = std::string{directory} + "/extra-" +
      std::to_string(i);
    unlink(extra.c_str());
    unlink((extra + ".tool").c_str());
  }

  unlink(descriptor.c_str());
  unlink(executable.c_str());
  rmdir(directory);
}

#endif

int
main()
{
#ifdef HAVE_POSIX
  plan_tests(84 + (geteuid() == 0 ? 19 : 0));
#else
  plan_tests(35);
#endif

  ok1(SystemTools::IsValidId("sensor-calibration"));
  ok1(SystemTools::IsValidId("vendor.tool_2"));
  ok1(!SystemTools::IsValidId(""));
  ok1(!SystemTools::IsValidId(".hidden"));
  ok1(!SystemTools::IsValidId("has/slash"));
  ok1(!SystemTools::IsValidId("has space"));

  char valid[] =
    "# Installed by the platform package\n"
    " name = Sensor calibration \n"
    "description=Calibrate the pressure sensors\n"
    "instructions=Keep the device still.\n"
    "mode=job\n"
    "input=none\n"
    "confirmation=required\n"
    "future-field=ignored\n";
  MemoryReader valid_memory{std::as_bytes(std::span{
    valid, sizeof(valid) - 1})};
  BufferedLineReader valid_reader{valid_memory};
  const auto descriptor = SystemTools::ParseDescriptor(valid_reader);
  ok1(descriptor.name == "Sensor calibration");
  ok1(descriptor.description == "Calibrate the pressure sensors");
  ok1(descriptor.instructions == "Keep the device still.");
  ok1(descriptor.mode == SystemTools::Mode::JOB);
  ok1(descriptor.confirmation == SystemTools::Confirmation::REQUIRED);

  char localized[] =
    "name[de]=Sensorkalibrierung\n"
    "description[de]=Sensoren kalibrieren\n"
    "instructions[de]=Nicht bewegen\n"
    "name=Sensor calibration\n"
    "description=Calibrate sensors\n"
    "instructions=Keep still\n"
    "mode=exclusive\n";
  MemoryReader localized_memory{std::as_bytes(std::span{
    localized, sizeof(localized) - 1})};
  BufferedLineReader localized_reader{localized_memory};
  const auto translated =
    SystemTools::ParseDescriptor(localized_reader, "de");
  ok1(translated.name == "Sensorkalibrierung");
  ok1(translated.description == "Sensoren kalibrieren");
  ok1(translated.instructions == "Nicht bewegen");
  ok1(translated.mode == SystemTools::Mode::EXCLUSIVE);

  char fallback[] =
    "name[de]=Sensorkalibrierung\n"
    "description[de]=Sensoren kalibrieren\n"
    "instructions[de]=Nicht bewegen\n"
    "name=Sensor calibration\n"
    "description=Calibrate sensors\n"
    "instructions=Keep still\n"
    "mode=job\n";
  MemoryReader fallback_memory{std::as_bytes(std::span{
    fallback, sizeof(fallback) - 1})};
  BufferedLineReader fallback_reader{fallback_memory};
  const auto untranslated =
    SystemTools::ParseDescriptor(fallback_reader, "fr");
  ok1(untranslated.name == "Sensor calibration");
  ok1(untranslated.description == "Calibrate sensors");
  ok1(untranslated.instructions == "Keep still");
  ok1(untranslated.confirmation == SystemTools::Confirmation::REQUIRED);

  char no_confirmation[] =
    "name=System information\n"
    "description=Show system information\n"
    "instructions=This action does not change settings.\n"
    "mode=job\n"
    "confirmation=none\n";
  MemoryReader no_confirmation_memory{std::as_bytes(std::span{
    no_confirmation, sizeof(no_confirmation) - 1})};
  BufferedLineReader no_confirmation_reader{no_confirmation_memory};
  ok1(SystemTools::ParseDescriptor(no_confirmation_reader).confirmation ==
      SystemTools::Confirmation::NONE);

  char destination_directory[] =
    "name=Back up system settings\n"
    "description=Create a settings archive\n"
    "instructions=Select a destination.\n"
    "mode=job\n"
    "input=destination-directory\n";
  ok1(Rejects(destination_directory));

  char existing_file[] =
    "name=Restore system settings\n"
    "description=Restore a settings archive\n"
    "instructions=Select an archive.\n"
    "mode=exclusive\n"
    "input=existing-file\n";
  ok1(Rejects(existing_file));

  char incomplete[] = "name=Missing fields\nmode=job\n";
  ok1(Rejects(incomplete));

  char malformed[] =
    "name=Malformed\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "this is not a field\n"
    "mode=job\n";
  ok1(Rejects(malformed));

  char duplicate[] =
    "name=First\n"
    "name=Second\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(duplicate));

  char bad_mode[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=shell\n";
  ok1(Rejects(bad_mode));

  char bad_input[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "input=command-line\n";
  ok1(Rejects(bad_input));

  char duplicate_input[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "input=none\n"
    "input=existing-file\n";
  ok1(Rejects(duplicate_input));

  char bad_confirmation[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "confirmation=optional\n";
  ok1(Rejects(bad_confirmation));

  char duplicate_confirmation[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "confirmation=required\n"
    "confirmation=none\n";
  ok1(Rejects(duplicate_confirmation));

  char empty_text[] =
    "name=\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(empty_text));

  char invalid_utf8[] =
    "name=Bad \xff\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(invalid_utf8));

  std::string maximum_name =
    "name=" + std::string(80, 'x') + "\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  MemoryReader maximum_name_memory{std::as_bytes(std::span{
    maximum_name.data(), maximum_name.size()})};
  BufferedLineReader maximum_name_reader{maximum_name_memory};
  ok1(SystemTools::ParseDescriptor(maximum_name_reader).name.size() == 80);

  std::string excessive_name =
    "name=" + std::string(81, 'x') + "\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(excessive_name.data()));

  char omitted_input[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  MemoryReader omitted_input_memory{std::as_bytes(std::span{
    omitted_input, sizeof(omitted_input) - 1})};
  BufferedLineReader omitted_input_reader{omitted_input_memory};
  ok1(SystemTools::ParseDescriptor(omitted_input_reader).mode ==
      SystemTools::Mode::JOB);

#ifdef HAVE_POSIX
  TestUserCatalogue();
#endif

  return exit_status();
}
