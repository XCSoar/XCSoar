/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ConfigChooser.hpp"
#include "ui/canvas/egl/GBM.hpp"
#include "ui/opengl/Features.hpp"
#include "util/RuntimeError.hxx"

#include <array>

namespace EGL {

/**
 * Returns the requested renderable type for EGL_RENDERABLE_TYPE.
 */
static constexpr EGLint
GetRenderableType()
{
  return HaveGLES()
    ? (HaveGLES2() ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT)
    : EGL_OPENGL_BIT;
}

#ifdef MESA_KMS

/**
 * Find an EGLConfig with the specified attribute value.
 *
 * @return an index into the #configs parameter or -1 if no matching
 * EGLConfig was found
 */
static EGLint
FindConfigWithAttribute(EGLDisplay display,
                        const EGLConfig *configs, EGLint num_configs,
                        EGLint attribute, EGLint expected_value) noexcept
{
  for (EGLint i = 0; i < num_configs; ++i) {
    EGLint value;
    if (eglGetConfigAttrib(display, configs[i], attribute, &value) &&
        value == expected_value)
      return i;
  }

  return -1;
}

#endif /* MESA_KMS */

EGLConfig
ChooseConfig(EGLDisplay display)
{
  static constexpr EGLint attributes[] = {
    EGL_STENCIL_SIZE, 1,
#ifdef MESA_KMS
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
#ifndef RASPBERRY_PI /* the Raspberry Pi 4 doesn't have an alpha channel */
    EGL_ALPHA_SIZE, 1,
#endif
#endif
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, GetRenderableType(),
    EGL_NONE
  };

  std::array<EGLConfig, 64> configs;
  EGLint num_configs;
  if (!eglChooseConfig(display, attributes, configs.data(), configs.size(),
                       &num_configs))
    throw FormatRuntimeError("eglChooseConfig() failed: %#x", eglGetError());

  if (num_configs == 0)
    throw std::runtime_error("eglChooseConfig() failed");

#ifdef MESA_KMS
  /* On some GBM targets, such as the Raspberry Pi 4,
     eglChooseConfig() gives us an EGLConfig which will later fail
     eglCreateWindowSurface() with EGL_BAD_MATCH.  Only the EGLConfig
     which has the matching EGL_NATIVE_VISUAL_ID will work. */
  EGLint i = FindConfigWithAttribute(display, configs.data(), num_configs,
                                     EGL_NATIVE_VISUAL_ID,
                                     XCSOAR_GBM_FORMAT);
  if (i < 0)
    i = FindConfigWithAttribute(display, configs.data(), num_configs,
                                EGL_NATIVE_VISUAL_ID,
                                XCSOAR_GBM_FORMAT_FALLBACK);
  return i >= 0 ? configs[i] : configs[0];
#else
  return configs[0];
#endif
}

} // namespace EGL
