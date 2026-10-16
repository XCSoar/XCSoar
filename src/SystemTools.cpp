// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemTools.hpp"
#include "LogFile.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/BufferedLineReader.hpp"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"
#include "io/LineReader.hpp"
#include "system/FileUtil.hpp"
#include "util/CharUtil.hxx"
#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"
#include "util/UTF8.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <utility>

#ifdef HAVE_POSIX
#include "io/UniqueFileDescriptor.hxx"

#include <cerrno>
#include <fcntl.h>
#include <limits.h>
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
static constexpr std::size_t MAX_EXCLUSIVE_RESULT_SIZE = 8192;
static constexpr const char *EXCLUSIVE_HANDOFF_ENV =
  "XCSOAR_EXCLUSIVE_HANDOFF_DIR";

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
HasUnsafeWritePermissions(const struct stat &st,
                          bool trust_group = false) noexcept
{
  return (st.st_mode & S_IWOTH) != 0 ||
    ((st.st_mode & S_IWGRP) != 0 &&
     (!trust_group || st.st_gid != getegid()));
}

static bool
IsTrustedDirectory(Path path, uid_t owner, bool trust_group = false) noexcept
{
  struct stat st;
  return lstat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) &&
    st.st_uid == owner && !HasUnsafeWritePermissions(st, trust_group);
}

static bool
IsTrustedFile(Path path, uid_t owner, bool executable,
              bool trust_group = false) noexcept
{
  struct stat st;
  return lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
    st.st_uid == owner && !HasUnsafeWritePermissions(st, trust_group) &&
    (!executable || ((st.st_mode & (S_ISUID | S_ISGID)) == 0 &&
                     (st.st_mode & S_IXUSR) != 0 &&
                     access(path.c_str(), X_OK) == 0));
}

static bool
IsTrustedDirectoryTree(Path directory, uid_t owner,
                       bool user_directory = false,
                       bool trust_group = false) noexcept
{
  AllocatedPath current{directory};
  if (!directory.IsAbsolute()) {
    if (!user_directory)
      return false;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr)
      return false;
    current = AllocatedPath::Build(Path{cwd}, directory);
  }

  bool first = true;
  while (true) {
    struct stat st;
    if (lstat(current.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) ||
        (st.st_uid != 0 && st.st_uid != owner) ||
        (HasUnsafeWritePermissions(st, trust_group) &&
         !(user_directory && !first && (st.st_mode & S_ISVTX) != 0)))
      return false;

    if (current == Path{"/"})
      return true;

    // Path::GetParent() returns "." for an immediate child of root.
    auto parent = current.GetParent();
    current = parent == Path{"."} ? AllocatedPath{Path{"/"}}
                                  : std::move(parent);
    first = false;
  }
}

static AllocatedPath
GetExclusiveHandoffDirectory() noexcept
{
  const char *const value = std::getenv(EXCLUSIVE_HANDOFF_ENV);
  if (value == nullptr || *value == '\0')
    return nullptr;

  const Path path{value};
  if (!path.IsAbsolute())
    return nullptr;

  struct stat st;
  if (lstat(path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) ||
      st.st_uid != geteuid() ||
      (st.st_mode & (S_IWGRP | S_IWOTH)) != 0 ||
      access(path.c_str(), W_OK | X_OK) != 0 ||
      !IsTrustedDirectoryTree(path, geteuid(), true))
    return nullptr;

  return AllocatedPath{path};
}

class DescriptorVisitor final : public File::Visitor {
  std::vector<InstalledTool> &tools;
  const Path directory;
  const std::string_view language;
  const uid_t owner;
  const Origin origin;

public:
  DescriptorVisitor(std::vector<InstalledTool> &_tools,
                    Path _directory,
                    std::string_view _language, uid_t _owner,
                    Origin _origin) noexcept
    :tools(_tools), directory(_directory), language(_language),
     owner(_owner), origin(_origin) {}

  void Visit(Path path, Path filename) override {
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
    if (origin == Origin::USER) {
      auto shell_script = AllocatedPath::Build(directory, (id + ".sh").c_str());
      struct stat st;
      const bool have_exact = lstat(executable.c_str(), &st) == 0;
      const bool have_shell = lstat(shell_script.c_str(), &st) == 0;
      if (have_exact && have_shell) {
        LogFormat("Ignoring ambiguous user tool: %s", path.c_str());
        return;
      }
      if (!have_exact && have_shell)
        executable = std::move(shell_script);
    }

    if (!IsTrustedFile(path, owner, false, origin == Origin::USER) ||
        !IsTrustedFile(executable, owner, true, origin == Origin::USER)) {
      LogFormat("Ignoring untrusted tool: %s", path.c_str());
      return;
    }

    Descriptor descriptor;
    try {
      FileReader file{path};
      // Stay below the buffered reader's line limit to avoid false EOF.
      if (file.GetSize() > MAX_DESCRIPTOR_SIZE)
        throw std::runtime_error{"System-tool descriptor too large"};
      BufferedLineReader reader{file};
      descriptor = ParseDescriptor(reader, language);
    } catch (...) {
      LogFormat("Ignoring malformed tool: %s", path.c_str());
      LogError(std::current_exception(), "Failed to load tool");
      return;
    }

    // A partial catalogue could hide a duplicate in another directory.
    if (tools.size() >= MAX_INSTALLED_TOOLS)
      throw std::runtime_error{"Too many system tools"};

    tools.push_back({id, std::move(descriptor),
                     std::move(executable), origin});
  }
};

static std::vector<InstalledTool>
LoadDirectory(Path directory, std::string_view language, uid_t owner,
              Origin origin, bool validate_parents)
{
  std::vector<InstalledTool> result;

  if (!Directory::Exists(directory))
    return result;

  const bool trusted = validate_parents
    ? IsTrustedDirectoryTree(directory, owner)
    : IsTrustedDirectory(directory, owner, origin == Origin::USER);
  if (!trusted) {
    LogFormat("Ignoring untrusted tool directory after recheck: %s",
              directory.c_str());
    return result;
  }

  DescriptorVisitor visitor{result, directory, language, owner, origin};
  Directory::VisitSpecificFiles(directory, "*.tool", visitor);

  return result;
}

static std::vector<InstalledTool>
LoadUserDirectory(Path directory, std::string_view language)
{
  if (directory == nullptr)
    return {};

  struct stat st;
  if (lstat(directory.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
    return {};

  const uid_t effective_uid = geteuid();
  if (st.st_uid != effective_uid || (st.st_mode & S_IXUSR) == 0 ||
      HasUnsafeWritePermissions(st, true) ||
      !IsTrustedDirectoryTree(directory, effective_uid, true, true)) {
    LogFormat("Ignoring untrusted tool directory: %s "
              "(uid=%lu mode=%o euid=%lu)",
              directory.c_str(), static_cast<unsigned long>(st.st_uid),
              static_cast<unsigned>(st.st_mode & 07777),
              static_cast<unsigned long>(effective_uid));
    return {};
  }

  return LoadDirectory(directory, language, effective_uid,
                       Origin::USER, false);
}

#endif

std::vector<InstalledTool>
LoadInstalled(std::string_view language, Path user_directory) noexcept
try
{
  std::vector<InstalledTool> result;

#ifdef HAVE_POSIX
  std::vector<std::string> duplicate_ids;
  std::vector<AllocatedPath> loaded_directories;

  const auto merge_tools = [&](std::vector<InstalledTool> installed) {
    for (auto &tool : installed) {
      if (std::find(duplicate_ids.begin(), duplicate_ids.end(), tool.id) !=
          duplicate_ids.end())
        continue;

      const auto existing =
        std::find_if(result.begin(), result.end(), [&tool](const auto &other) {
          return other.id == tool.id;
        });
      if (existing != result.end()) {
        LogFormat("Ignoring duplicate tool ID: %s", tool.id.c_str());
        duplicate_ids.push_back(tool.id);
        result.erase(existing);
      } else
        result.push_back(std::move(tool));
    }
  };

  for (const Path directory : INSTALLATION_DIRECTORIES) {
    if (std::find(loaded_directories.begin(), loaded_directories.end(),
                  directory) != loaded_directories.end())
      continue;

    loaded_directories.emplace_back(directory);
    merge_tools(LoadDirectory(directory, language, 0,
                              Origin::SYSTEM, true));
  }

  if (result.size() > MAX_INSTALLED_TOOLS)
    throw std::runtime_error{"Too many system tools"};

  if (user_directory != nullptr &&
      std::find(loaded_directories.begin(), loaded_directories.end(),
                user_directory) == loaded_directories.end()) {
    try {
      auto personal = LoadUserDirectory(user_directory, language);
      const auto overridden = [&](const auto &tool) {
        return std::any_of(personal.begin(), personal.end(),
                           [&](const auto &other) {
                             return other.id == tool.id;
                           });
      };
      const std::size_t combined_size = result.size() + personal.size() -
        std::count_if(result.begin(), result.end(), overridden);
      if (combined_size > MAX_INSTALLED_TOOLS)
        throw std::runtime_error{"Too many system tools"};
      result.reserve(combined_size);
      std::erase_if(result, overridden);
      for (auto &tool : personal)
        result.push_back(std::move(tool));
    } catch (...) {
      LogError(std::current_exception(), "Failed to load personal tools");
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &a, const auto &b) {
              return StringCollate(a.descriptor.name.c_str(),
                                   b.descriptor.name.c_str()) < 0;
            });
#else
  (void)language;
  (void)user_directory;
#endif

  return result;
}
catch (...)
{
  LogError(std::current_exception(), "Failed to load system tools");
  return {};
}

bool
HasExclusiveHandoff() noexcept
{
#ifdef HAVE_POSIX
  return GetExclusiveHandoffDirectory() != nullptr;
#else
  return false;
#endif
}

bool
RequestExclusiveHandoff(std::string_view id) noexcept
{
#ifdef HAVE_POSIX
  if (!IsValidId(id))
    return false;

  const auto directory = GetExclusiveHandoffDirectory();
  if (directory == nullptr)
    return false;

  try {
    FileOutputStream file{
      AllocatedPath::Build(directory, "request")};
    BufferedOutputStream writer{file};
    writer.Write(id);
    writer.NewLine();
    writer.Flush();
    file.Commit();
    return true;
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to request exclusive system tool");
  }
#else
  (void)id;
#endif

  return false;
}

std::optional<ExclusiveResult>
ConsumeExclusiveResult() noexcept
{
#ifdef HAVE_POSIX
  const auto directory = GetExclusiveHandoffDirectory();
  if (directory == nullptr)
    return std::nullopt;

  const auto path = AllocatedPath::Build(directory, "result");
  UniqueFileDescriptor fd;
  if (!fd.Open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK))
    return std::nullopt;

  struct stat st;
  if (fstat(fd.Get(), &st) != 0 || !S_ISREG(st.st_mode) ||
      st.st_uid != geteuid() ||
      (st.st_mode & (S_IWGRP | S_IWOTH)) != 0 || st.st_size < 0 ||
      static_cast<std::size_t>(st.st_size) > MAX_EXCLUSIVE_RESULT_SIZE) {
    LogFormat("Ignoring invalid exclusive system-tool result: %s",
              path.c_str());
    File::Delete(path);
    return std::nullopt;
  }

  std::array<char, MAX_EXCLUSIVE_RESULT_SIZE + 1> buffer;
  ssize_t nbytes;
  do {
    nbytes = fd.Read(buffer.data(), buffer.size());
  } while (nbytes < 0 && errno == EINTR);
  if (nbytes < 0 || nbytes != st.st_size) {
    LogFormat("Failed to read exclusive system-tool result: %s",
              path.c_str());
    File::Delete(path);
    return std::nullopt;
  }
  File::Delete(path);
  std::string_view source{buffer.data(), static_cast<std::size_t>(nbytes)};
  const auto id_end = source.find('\n');
  if (id_end == std::string_view::npos)
    return std::nullopt;

  auto id = source.substr(0, id_end);
  source.remove_prefix(id_end + 1);
  const auto status_end = source.find('\n');
  if (status_end == std::string_view::npos)
    return std::nullopt;

  auto status = source.substr(0, status_end);
  source.remove_prefix(status_end + 1);
  if (!id.empty() && id.back() == '\r')
    id.remove_suffix(1);
  if (!status.empty() && status.back() == '\r')
    status.remove_suffix(1);

  if (!IsValidId(id) || (status != "success" && status != "failure") ||
      !ValidateUTF8(source))
    return std::nullopt;

  std::string message{source};
  std::erase_if(message, [](char ch) { return !IsTextByte(ch); });
  return ExclusiveResult{std::string{id}, status == "success",
                         std::move(message)};
#else
  return std::nullopt;
#endif
}

} // namespace SystemTools
