// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Init.hpp"
#include "lib/fmt/RuntimeError.hxx"

#if defined(__clang__) && defined(__arm__)
/* work around warning: 'register' storage class specifier is
   deprecated */
#define register
#endif

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdexcept>

namespace FreeType {

#ifdef KOBO
bool mono = true;
#endif

static FT_Library ft_library;

void
Initialise()
{
  FT_Error error = FT_Init_FreeType(&ft_library);
  if (error)
    throw std::runtime_error{"FT_Init_FreeType() failed"};
}

void
Deinitialise()
{
  FT_Done_FreeType(ft_library);
}

FT_Face
Load(const char *path)
{
  FT_Face face;
  FT_Error error = FT_New_Face(ft_library, path, 0, &face);
  if (error)
    throw FmtRuntimeError("Failed to load font {}: {}",
                          path, FT_Error_String(error));

  return face;
}

} // namespace FreeType
