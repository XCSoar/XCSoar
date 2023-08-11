// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
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
             std::span<TCHAR> value) noexcept
{
  value[0] = _T('\0');
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
