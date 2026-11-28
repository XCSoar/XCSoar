// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceFile.hpp"
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

#define DEVICE_FILE "device.xcd"

namespace DeviceFile {

namespace Keys {
constexpr std::string_view StartupMode = "StartupMode";
constexpr std::string_view AntiAliasing = "AntiAliasing";
} // namespace Keys

/**
 * Counts the screen can actually be asked for.  Keep in step with
 * ANTIALIASING_SAMPLE_COUNTS in UISettings.hpp; this file does not
 * include that header.
 */
static bool
IsStoredAntialiasing(unsigned samples) noexcept
{
  switch (samples) {
  case 0:
  case 2:
  case 4:
  case 8:
  case 16:
    return true;
  default:
    return false;
  }
}

/** the in-memory copy of device.xcd */
static ProfileMap state;

[[gnu::pure]]
static AllocatedPath
GetPath() noexcept
{
  return LocalPath(DEVICE_FILE);
}

void
Load() noexcept
{
  state.Clear();

  const auto path = GetPath();
  if (!File::Exists(path)) {
    /* no device file yet; the first Save() call will create it */
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
    LogError(std::current_exception(), "Failed to load " DEVICE_FILE);
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
    LogError(std::current_exception(), "Failed to save " DEVICE_FILE);
  }
}

StartupMode
GetStartupMode() noexcept
{
  int value;
  if (!state.Get(Keys::StartupMode, value) ||
      value < int(StartupMode::ASK) ||
      value > int(StartupMode::SIMULATOR))
    /* missing, or a value that does not fit the enum */
    return StartupMode::ASK;

  return StartupMode(value);
}

void
SetStartupMode(StartupMode mode) noexcept
{
  state.SetEnum(Keys::StartupMode, mode);
  Save();
}

unsigned
GetAntialiasing() noexcept
{
  unsigned value;
  if (!state.Get(Keys::AntiAliasing, value) ||
      !IsStoredAntialiasing(value))
    return 0;

  return value;
}

void
SetAntialiasing(unsigned samples) noexcept
{
  if (!IsStoredAntialiasing(samples))
    samples = 0;

  state.Set(Keys::AntiAliasing, samples);
  Save();
}

} // namespace DeviceFile
