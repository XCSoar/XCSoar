// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfiguredFile.hpp"
#include "FileReader.hxx"
#include "Profile/Profile.hpp"
#include "system/Path.hpp"

std::unique_ptr<FileReader>
OpenConfiguredFile(std::string_view profile_key)
{
  const auto path = Profile::GetPath(profile_key);
  if (path == nullptr)
    return nullptr;

  return std::make_unique<FileReader>(path);
}
