/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Extension.hpp"
#include "Features.hpp"
#include "System.hpp"

#include <string.h>

bool
OpenGL::IsExtensionSupported(const char *extension)
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
