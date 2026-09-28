// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "system/Path.hpp"

void
Profile::Save() noexcept
{
}

void
Profile::SetFiles([[maybe_unused]] Path override_path) noexcept
{
}

const char *
Profile::Get([[maybe_unused]] std::string_view key,
             [[maybe_unused]] const char *default_value) noexcept
{
  return NULL;
}

bool
Profile::Get([[maybe_unused]] std::string_view key,
             std::span<char> value) noexcept
{
  value[0] = '\0';
  return false;
}

void
Profile::Set([[maybe_unused]] std::string_view key,
             [[maybe_unused]] const char *value) noexcept
{
}

AllocatedPath
Profile::GetPath([[maybe_unused]] std::string_view key) noexcept
{
  return nullptr;
}

bool
Profile::SetPath([[maybe_unused]] std::string_view key,
                 [[maybe_unused]] Path value) noexcept
{
  return false;
}

std::vector<AllocatedPath>
Profile::GetMultiplePaths([[maybe_unused]] std::string_view key,
                          [[maybe_unused]] const char *patterns)
{
  return std::vector<AllocatedPath>();
}

bool
Profile::SetMultiplePaths([[maybe_unused]] std::string_view key,
                          [[maybe_unused]] std::span<const Path> values) noexcept
{
  return false;
}

bool
Profile::GetPathIsEqual([[maybe_unused]] std::string_view key,
                        [[maybe_unused]] Path value) noexcept
{
  return false;
}
