// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemTools.hpp"
#include "LogFile.hpp"
#include "io/BufferedLineReader.hpp"
#include "io/FileReader.hxx"
#include "io/LineReader.hpp"
#include "system/FileUtil.hpp"
#include "util/CharUtil.hxx"
#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"
#include "util/UTF8.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

#ifdef HAVE_POSIX
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace SystemTools {

#ifndef SYSTEM_TOOLS_DIRECTORY
#define SYSTEM_TOOLS_DIRECTORY "/usr/libexec/xcsoar/system-tools"
#endif

#ifndef LOCAL_SYSTEM_TOOLS_DIRECTORY
#define LOCAL_SYSTEM_TOOLS_DIRECTORY \
  "/usr/local/libexec/xcsoar/system-tools"
#endif

/**
 * Root-owned directories containing paired <id>.tool descriptors and <id>
 * executables.  Missing directories are normal.
 */
static constexpr Path INSTALLATION_DIRECTORIES[]{
  Path{SYSTEM_TOOLS_DIRECTORY},
  Path{LOCAL_SYSTEM_TOOLS_DIRECTORY},
};

static constexpr std::size_t MAX_ID_LENGTH = 64;
static constexpr std::size_t MAX_NAME_LENGTH = 80;
static constexpr std::size_t MAX_DESCRIPTION_LENGTH = 240;
static constexpr std::size_t MAX_INSTRUCTIONS_LENGTH = 1000;
static constexpr std::size_t MAX_INSTALLED_TOOLS = 64;
static constexpr std::size_t MAX_DESCRIPTOR_SIZE = 64 * 1024;

bool
IsValidId(std::string_view id) noexcept
{
  if (id.empty() || id.size() > MAX_ID_LENGTH ||
      !IsAlphaNumericASCII(id.front()))
    return false;

  for (const char ch : id)
    if (!IsAlphaNumericASCII(ch) && ch != '-' && ch != '_' && ch != '.')
      return false;

  return true;
}

static bool
HasControlCharacter(std::string_view value) noexcept
{
  for (const unsigned char ch : value)
    if (ch < 0x20 || ch == 0x7f)
      return true;

  return false;
}

static void
SetText(std::string &destination, bool &defined, std::string_view value,
        std::size_t max_length)
{
  if (defined)
    throw std::runtime_error{"Duplicate system-tool field"};

  if (value.empty() || !ValidateUTF8(value) || HasControlCharacter(value) ||
      TruncateStringUTF8(value, max_length) != value.size())
    throw std::runtime_error{"Invalid system-tool text"};

  destination = value;
  defined = true;
}

static bool
IsLocalizedKey(std::string_view key, std::string_view base,
               std::string_view language) noexcept
{
  return !language.empty() &&
    key.size() == base.size() + language.size() + 2 &&
    key.starts_with(base) && key[base.size()] == '[' && key.back() == ']' &&
    key.substr(base.size() + 1, language.size()) == language;
}

Descriptor
ParseDescriptor(NLineReader &reader, std::string_view language)
{
  Descriptor result;
  bool have_name = false;
  bool have_description = false;
  bool have_instructions = false;
  bool have_mode = false;
  bool have_input = false;
  bool have_confirmation = false;
  bool have_localized_name = false;
  bool have_localized_description = false;
  bool have_localized_instructions = false;
  std::string localized_name;
  std::string localized_description;
  std::string localized_instructions;

  for (char *line; (line = reader.ReadLine()) != nullptr;) {
    std::string_view source = Strip(std::string_view{line});
    if (source.empty() || source.front() == '#')
      continue;

    const auto separator = source.find('=');
    if (separator == std::string_view::npos)
      throw std::runtime_error{"Malformed system-tool field"};

    const auto key = Strip(source.substr(0, separator));
    const auto value = Strip(source.substr(separator + 1));
    if (key.empty())
      throw std::runtime_error{"Empty system-tool field name"};

    if (key == "name")
      SetText(result.name, have_name, value, MAX_NAME_LENGTH);
    else if (key == "description")
      SetText(result.description, have_description, value,
              MAX_DESCRIPTION_LENGTH);
    else if (key == "instructions")
      SetText(result.instructions, have_instructions, value,
              MAX_INSTRUCTIONS_LENGTH);
    else if (IsLocalizedKey(key, "name", language))
      SetText(localized_name, have_localized_name, value, MAX_NAME_LENGTH);
    else if (IsLocalizedKey(key, "description", language))
      SetText(localized_description, have_localized_description, value,
              MAX_DESCRIPTION_LENGTH);
    else if (IsLocalizedKey(key, "instructions", language))
      SetText(localized_instructions, have_localized_instructions, value,
              MAX_INSTRUCTIONS_LENGTH);
    else if (key == "mode") {
      if (have_mode)
        throw std::runtime_error{"Duplicate system-tool mode"};

      if (value == "job")
        result.mode = Mode::JOB;
      else if (value == "exclusive")
        result.mode = Mode::EXCLUSIVE;
      else
        throw std::runtime_error{"Invalid system-tool mode"};

      have_mode = true;
    } else if (key == "input") {
      if (have_input)
        throw std::runtime_error{"Duplicate system-tool input"};

      if (value != "none")
        throw std::runtime_error{"Invalid system-tool input"};

      have_input = true;
    } else if (key == "confirmation") {
      if (have_confirmation)
        throw std::runtime_error{"Duplicate system-tool confirmation"};

      if (value == "required")
        result.confirmation = Confirmation::REQUIRED;
      else if (value == "none")
        result.confirmation = Confirmation::NONE;
      else
        throw std::runtime_error{"Invalid system-tool confirmation"};

      have_confirmation = true;
    }
  }

  if (!have_name || !have_description || !have_instructions || !have_mode)
    throw std::runtime_error{"Incomplete system-tool descriptor"};

  if (have_localized_name)
    result.name = std::move(localized_name);
  if (have_localized_description)
    result.description = std::move(localized_description);
  if (have_localized_instructions)
    result.instructions = std::move(localized_instructions);

  return result;
}

#ifdef HAVE_POSIX

static bool
IsTrustedDirectory(Path path) noexcept
{
  struct stat st;
  return lstat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) &&
    st.st_uid == 0 && (st.st_mode & (S_IWGRP | S_IWOTH)) == 0;
}

static bool
IsTrustedFile(Path path, bool executable) noexcept
{
  struct stat st;
  return lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
    st.st_uid == 0 && (st.st_mode & (S_IWGRP | S_IWOTH)) == 0 &&
    (!executable || ((st.st_mode & (S_ISUID | S_ISGID)) == 0 &&
                     access(path.c_str(), X_OK) == 0));
}

static bool
IsTrustedInstallationDirectory(Path directory) noexcept
{
  if (!directory.IsAbsolute())
    return false;

  AllocatedPath current{directory};
  while (true) {
    if (!IsTrustedDirectory(current))
      return false;

    AllocatedPath parent = current.GetParent();
    if (parent == current)
      return true;

    current = std::move(parent);
  }
}

class DescriptorVisitor final : public File::Visitor {
  std::vector<InstalledTool> &tools;
  const Path directory;
  const std::string_view language;

public:
  DescriptorVisitor(std::vector<InstalledTool> &_tools,
                    Path _directory,
                    std::string_view _language) noexcept
    :tools(_tools), directory(_directory), language(_language) {}

  void Visit(Path path, Path filename) override {
    if (tools.size() >= MAX_INSTALLED_TOOLS)
      return;

    constexpr std::string_view suffix{".tool"};
    std::string_view basename{filename.c_str()};
    if (!basename.ends_with(suffix))
      return;

    basename.remove_suffix(suffix.size());
    if (!IsValidId(basename)) {
      LogFormat("Ignoring system tool with invalid ID: %s", path.c_str());
      return;
    }

    const std::string id{basename};
    AllocatedPath executable =
      AllocatedPath::Build(directory, id.c_str());
    if (!IsTrustedFile(path, false) ||
        !IsTrustedFile(executable, true)) {
      LogFormat("Ignoring untrusted system tool: %s", path.c_str());
      return;
    }

    try {
      FileReader file{path};
      // Stay below the buffered reader's line limit to avoid false EOF.
      if (file.GetSize() > MAX_DESCRIPTOR_SIZE)
        throw std::runtime_error{"System-tool descriptor too large"};
      BufferedLineReader reader{file};
      tools.push_back({id, ParseDescriptor(reader, language),
                       std::move(executable)});
    } catch (...) {
      LogFormat("Ignoring malformed system tool: %s", path.c_str());
      LogError(std::current_exception(), "Failed to load system tool");
    }
  }
};

static std::vector<InstalledTool>
LoadDirectory(Path directory, std::string_view language) noexcept
{
  std::vector<InstalledTool> result;

  if (!Directory::Exists(directory))
    return result;

  if (!IsTrustedInstallationDirectory(directory)) {
    LogFormat("Ignoring untrusted system-tool directory: %s",
              directory.c_str());
    return result;
  }

  try {
    DescriptorVisitor visitor{result, directory, language};
    Directory::VisitSpecificFiles(directory, "*.tool", visitor);
  } catch (...) {
    LogError(std::current_exception(), "Failed to scan system tools");
    result.clear();
  }

  return result;
}

#endif

std::vector<InstalledTool>
LoadInstalled(std::string_view language) noexcept
{
  std::vector<InstalledTool> result;

#ifdef HAVE_POSIX
  std::vector<std::string> duplicate_ids;
  Path previous_directory{nullptr};

  for (const Path directory : INSTALLATION_DIRECTORIES) {
    if (directory == previous_directory)
      continue;

    previous_directory = directory;
    auto installed = LoadDirectory(directory, language);
    for (auto &tool : installed) {
      if (std::find(duplicate_ids.begin(), duplicate_ids.end(), tool.id) !=
          duplicate_ids.end())
        continue;

      const auto existing =
        std::find_if(result.begin(), result.end(), [&tool](const auto &other) {
          return other.id == tool.id;
        });
      if (existing != result.end()) {
        LogFormat("Ignoring duplicate system tool ID: %s", tool.id.c_str());
        duplicate_ids.push_back(tool.id);
        result.erase(existing);
      } else if (result.size() < MAX_INSTALLED_TOOLS)
        result.push_back(std::move(tool));
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &a, const auto &b) {
              return StringCollate(a.descriptor.name.c_str(),
                                   b.descriptor.name.c_str()) < 0;
            });
#else
  (void)language;
#endif

  return result;
}

} // namespace SystemTools
