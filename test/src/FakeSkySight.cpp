// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Weather/SkySight/SkySightManager.hpp"

std::shared_ptr<SkySightManager>
DataGlobals::GetSkySight() noexcept
{
  return {};
}

void
SkySightManager::Render()
{
}

#endif
