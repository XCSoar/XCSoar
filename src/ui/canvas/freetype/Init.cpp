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

#ifdef _WIN32
#include "system/OpenPathFile.hpp"
#include "system/Path.hpp"
#include "util/ScopeExit.hxx"

#include <vector>
#endif

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

#ifdef _WIN32

/**
 * Free the heap buffer attached to a memory face (see FreeType::Load).
 */
static void
FreeFaceFileBuffer(void *object)
{
  auto *face = (FT_Face)object;
  delete (std::vector<unsigned char> *)face->generic.data;
  face->generic.data = nullptr;
}

#endif

FT_Face
Load(const char *path)
{
  if (path == nullptr || *path == '\0')
    throw std::runtime_error("Invalid font path");

  FT_Face face;

#ifdef _WIN32
  /* FT_New_Face() uses CRT open() (ANSI); load via _wfopen into a
     memory face so UTF-8 font paths work. */
  FILE *file = OpenPathFile(Path(path), "rb");
  if (file == nullptr)
    throw FmtRuntimeError("Failed to open font {}", path);

  AtScopeExit(file) { fclose(file); };

  if (fseek(file, 0, SEEK_END) != 0)
    throw FmtRuntimeError("Failed to seek font {}", path);

  const long size = ftell(file);
  if (size <= 0)
    throw FmtRuntimeError("Failed to size font {}", path);

  if (fseek(file, 0, SEEK_SET) != 0)
    throw FmtRuntimeError("Failed to rewind font {}", path);

  auto *buffer = new std::vector<unsigned char>((std::size_t)size);
  if (fread(buffer->data(), 1, buffer->size(), file) != buffer->size()) {
    delete buffer;
    throw FmtRuntimeError("Failed to read font {}", path);
  }

  FT_Error error = FT_New_Memory_Face(ft_library,
                                      buffer->data(),
                                      (FT_Long)buffer->size(),
                                      0, &face);
  if (error) {
    delete buffer;
    throw FmtRuntimeError("Failed to load font {}: {}",
                          path, FT_Error_String(error));
  }

  face->generic.data = buffer;
  face->generic.finalizer = FreeFaceFileBuffer;
#else
  FT_Error error = FT_New_Face(ft_library, path, 0, &face);
  if (error)
    throw FmtRuntimeError("Failed to load font {}: {}",
                          path, FT_Error_String(error));
#endif

  return face;
}

} // namespace FreeType
