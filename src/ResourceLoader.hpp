// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

class ResourceId;

namespace ResourceLoader {

using Data = std::span<const std::byte>;

Data
Load(const char *name, const char *type);

#ifndef ANDROID
Data
Load(ResourceId id);
#endif

} // namespace ResourceLoader
