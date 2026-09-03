// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

#include "resource_data.h"
#include "util/StringAPI.hxx"

ResourceLoader::Data
ResourceLoader::Load(const char *name, [[maybe_unused]] const char *type)
{
  for (unsigned i = 0; named_resources[i].data.data() != nullptr; ++i)
    if (StringIsEqual(named_resources[i].name, name))
      return named_resources[i].data;

  return {};
}

#ifndef ANDROID

ResourceLoader::Data
ResourceLoader::Load(ResourceId id)
{
  return id;
}

#endif
