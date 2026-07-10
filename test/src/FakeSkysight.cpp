// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Weather/Skysight/Skysight.hpp"

std::shared_ptr<Skysight>
DataGlobals::GetSkysight() noexcept
{
  return {};
}

void
Skysight::Render()
{
}

#endif