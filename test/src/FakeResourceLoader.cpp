// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

namespace ResourceLoader {

Data
Load([[maybe_unused]] const char *name,
     [[maybe_unused]] const char *type)
{
  return {};
}

#ifndef ANDROID
Data
Load([[maybe_unused]] ResourceId id)
{
  return {};
}
#endif

#ifdef _WIN32
HBITMAP
LoadBitmap2([[maybe_unused]] ResourceId id)
{
  return nullptr;
}
#endif

} // namespace ResourceLoader
