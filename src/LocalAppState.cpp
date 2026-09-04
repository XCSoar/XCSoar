// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LocalAppState.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "Profile/Map.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "io/KeyValueFileReader.hpp"
#include "io/KeyValueFileWriter.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <string_view>

#define STATE_FILE "state.ini"

namespace LocalAppState {

namespace Keys {
constexpr std::string_view StartupMode = "StartupMode";
} // namespace Keys

/** the in-memory copy of the state file */
static ProfileMap state;

[[gnu::pure]]
static AllocatedPath
GetPath() noexcept
{
  return LocalPath(STATE_FILE);
}

void
Load() noexcept
{
  state.Clear();

  const auto path = GetPath();
  if (!File::Exists(path)) {
    /* no state file yet; the first Save() call will create it */
    state.SetModified(false);
    return;
  }

  try {
    FileLineReaderA reader(path);
    KeyValueFileReader kv_reader(reader);
    KeyValuePair pair;
    while (kv_reader.Read(pair))
      state.Set(pair.key, pair.value);
  } catch (...) {
    LogError(std::current_exception(), "Failed to load " STATE_FILE);
  }

  state.SetModified(false);
}

void
Save() noexcept
{
  if (!state.IsModified())
    return;

  const auto path = GetPath();

  try {
    if (const auto parent = path.GetParent(); parent != nullptr)
      Directory::CreateRecursive(parent);

    FileOutputStream file(path);
    BufferedOutputStream buffered(file);
    KeyValueFileWriter kv_writer(buffered);

    for (const auto &i : state)
      kv_writer.Write(i.first.c_str(), i.second.c_str());

    buffered.Flush();
    file.Commit();

    state.SetModified(false);
  } catch (...) {
    LogError(std::current_exception(), "Failed to save " STATE_FILE);
  }
}

StartupMode
GetStartupMode() noexcept
{
  StartupMode mode = StartupMode::ASK;
  state.GetEnum(Keys::StartupMode, mode);

  switch (mode) {
  case StartupMode::ASK:
  case StartupMode::FLY:
  case StartupMode::SIMULATOR:
    return mode;
  }

  /* invalid value in the state file */
  return StartupMode::ASK;
}

void
SetStartupMode(StartupMode mode) noexcept
{
  state.SetEnum(Keys::StartupMode, mode);
  Save();
}

} // namespace LocalAppState
