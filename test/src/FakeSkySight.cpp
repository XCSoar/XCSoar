// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Weather/SkySight/SkySightClient.hpp"

std::shared_ptr<SkySightClient>
DataGlobals::GetSkySight() noexcept
{
  return {};
}

void
SkySightClient::Render()
{
}

#endif