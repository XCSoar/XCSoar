// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Configured.hpp"
#include "RaspStore.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "LocalPath.hpp"

std::shared_ptr<RaspStore>
LoadConfiguredRasp() noexcept
{
  auto path = Profile::GetPath(ProfileKeys::RaspFile);
  if (path == nullptr)
    /* if no path is configured, attempt to load xcsoar-rasp.dat
       (XCSoar < 7.29) */
    path = LocalPath(RASP_FILENAME);

  auto rasp = std::make_shared<RaspStore>(std::move(path));
  rasp->ScanAll();
  return rasp;
}
