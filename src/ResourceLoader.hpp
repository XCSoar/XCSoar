// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

#ifdef _WIN32
#include <windef.h>
#endif

#include <tchar.h>

class ResourceId;

namespace ResourceLoader {

#ifdef _WIN32
void
Init(HINSTANCE hInstance);
#endif

using Data = std::span<const std::byte>;

Data
Load(const TCHAR *name, const TCHAR *type);

#ifndef ANDROID
Data
Load(ResourceId id);
#endif

#ifdef _WIN32
HBITMAP
LoadBitmap2(ResourceId id);
#endif

} // namespace ResourceLoader
