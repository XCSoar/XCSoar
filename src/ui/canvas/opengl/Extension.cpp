// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Extension.hpp"
#include "ui/opengl/Features.hpp"
#include "ui/opengl/System.hpp"

#include <string.h>

bool
OpenGL::IsExtensionSupported(const char *extension) noexcept
{
  /* this code is copied from
     http://www.opengl.org/resources/features/OGLextensions/ */

  const GLubyte *const extensions = glGetString(GL_EXTENSIONS);
#ifdef ANDROID
  /* some broken Android drivers are insane and return nullptr under
     certain conditions; under these conditions, the driver doesn't
     work at all; the following check works around the crash */
  if (extensions == nullptr)
    return false;
#endif

  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  const GLubyte *start = extensions;
  for (;;) {
    const GLubyte *where = (const GLubyte *) strstr((const char *) start, extension);
    if (!where)
      break;

    const GLubyte *terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ')
      if (*terminator == ' ' || *terminator == '\0')
        return true;

    start = terminator;
  }
  return false;
}
