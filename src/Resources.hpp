// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ResourceId.hpp"

#if defined(USE_GDI) || defined(ANDROID)

#define MAKE_RESOURCE(name, id) \
  static constexpr ResourceId name(id);

#else

#include <cstddef>

#define MAKE_RESOURCE(name, id) \
  extern "C" std::byte resource_ ## id[]; \
  extern "C" const size_t resource_ ## id ## _size; \
  static constexpr ResourceId name(resource_ ##id, &resource_ ## id ## _size);

#endif

#include "MakeResource.hpp"

#undef MAKE_RESOURCE
