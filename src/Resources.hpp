// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ResourceId.hpp"

#ifdef USE_WIN32_RESOURCES

#define MAKE_RESOURCE(name, file, id) \
  static constexpr ResourceId name(id);

#elif defined(ANDROID)

#define MAKE_RESOURCE(name, file, id) \
  static constexpr ResourceId name{#file};

#else

#include <cstddef>

#define MAKE_RESOURCE(name, file, id) \
  extern "C" std::byte resource_ ## name[]; \
  extern "C" const size_t resource_ ## name ## _size; \
  static constexpr ResourceId name(resource_ ## name, &resource_ ## name ## _size);

#endif

#include "MakeResource.hpp"

#undef MAKE_RESOURCE
