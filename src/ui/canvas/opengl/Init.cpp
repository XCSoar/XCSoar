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

#include "Init.hpp"
#include "Debug.hpp"
#include "Extension.hpp"
#include "Globals.hpp"
#include "Shapes.hpp"
#include "Function.hpp"
#include "Dynamic.hpp"
#include "FBO.hpp"
#include "ui/canvas/custom/Cache.hpp"
#include "ui/opengl/Features.hpp"
#include "Math/Point2D.hpp"
#include "Asset.hpp"
#include "DisplayOrientation.hpp"
#include "LogFile.hpp"

#ifdef USE_EGL
#include "ui/egl/System.hpp"
#endif

#include "Shaders.hpp"
#include "Math/Angle.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

#include <cassert>
#include <string.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

void
OpenGL::Initialise()
{
#ifndef NDEBUG
  thread = pthread_self();
#endif
}

/**
 * Does the current OpenGL context support textures with dimensions
 * other than power-of-two?
 */
[[gnu::pure]]
static bool
SupportsNonPowerOfTwoTextures() noexcept
{
  return OpenGL::IsExtensionSupported("GL_OES_texture_npot");
}

/**
 * Check which depth+stencil internalType is available for a
 * Renderbuffer.  Returns GL_NONE if the Renderbuffer does not support
 * it.
 */
[[gnu::pure]]
static GLenum
CheckDepthStencil() noexcept
{
  if (OpenGL::IsExtensionSupported("GL_OES_packed_depth_stencil"))
    return GL_DEPTH24_STENCIL8_OES;

  /* not supported */
  return GL_NONE;
}

/**
 * Check which stencil internalType is available for a Renderbuffer.
 * Returns GL_NONE if the Renderbuffer does not support it.
 */
[[gnu::pure]]
static GLenum
CheckStencil() noexcept
{
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
  if (OpenGL::IsExtensionSupported("GL_OES_stencil1"))
    return GL_STENCIL_INDEX1_OES;

  if (OpenGL::IsExtensionSupported("GL_OES_stencil4"))
    return GL_STENCIL_INDEX4_OES;
#endif

  if (OpenGL::IsExtensionSupported("GL_OES_stencil8")) {
    return GL_STENCIL_INDEX8;
  }

  /* not supported */
  return GL_NONE;
}

void
OpenGL::SetupContext()
{
  if (auto s = (const char *)glGetString(GL_VENDOR))
    LogFormat("GL vendor: %s", s);

  if (auto s = (const char *)glGetString(GL_VERSION))
    LogFormat("GL version: %s", s);

  if (auto s = (const char *)glGetString(GL_RENDERER))
    LogFormat("GL renderer: %s", s);

  if (auto s = (const char *)glGetString(GL_EXTENSIONS))
    LogFormat("GL extensions: %s", s);

  texture_non_power_of_two = SupportsNonPowerOfTwoTextures();

#ifdef ANDROID
  native_view->SetTexturePowerOfTwo(Java::GetEnv(), texture_non_power_of_two);
#endif

  mapbuffer = IsExtensionSupported("GL_OES_mapbuffer");

#ifdef HAVE_DYNAMIC_MAPBUFFER
  if (mapbuffer) {
    GLExt::map_buffer = (PFNGLMAPBUFFEROESPROC)
      GetProcAddress("glMapBufferOES");
    GLExt::unmap_buffer = (PFNGLUNMAPBUFFEROESPROC)
      GetProcAddress("glUnmapBufferOES");
    if (GLExt::map_buffer == nullptr || GLExt::unmap_buffer == nullptr)
      mapbuffer = false;
  }
#endif

#ifdef HAVE_DYNAMIC_MULTI_DRAW_ARRAYS
  if (IsExtensionSupported("GL_EXT_multi_draw_arrays")) {
    GLExt::multi_draw_arrays = (PFNGLMULTIDRAWARRAYSEXTPROC)
      GetProcAddress("glMultiDrawArraysEXT");
    GLExt::multi_draw_elements = (PFNGLMULTIDRAWELEMENTSEXTPROC)
      GetProcAddress("glMultiDrawElementsEXT");
  } else {
    GLExt::multi_draw_arrays = nullptr;
    GLExt::multi_draw_elements = nullptr;
  }
#endif

  render_buffer_depth_stencil = CheckDepthStencil();

  render_buffer_stencil = CheckStencil();
  if (!render_buffer_stencil)
    /* fall back to a packed depth+stencil format */
    render_buffer_stencil = render_buffer_depth_stencil;

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);

  InitShapes();

  InitShaders();
}

#ifdef SOFTWARE_ROTATE_DISPLAY

/**
 * Determine the projection rotation angle (in degrees) of the
 * specified orientation.
 */
[[gnu::const]]
static GLfloat
OrientationToRotation(DisplayOrientation orientation) noexcept
{
  switch (orientation) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::LANDSCAPE:
    return 0;

  case DisplayOrientation::PORTRAIT:
    return 90;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    return 180;

  case DisplayOrientation::REVERSE_PORTRAIT:
    return 270;
  }

  assert(false);
  gcc_unreachable();
}

/**
 * Swap x and y if the given orientation specifies it.
 */
static void
OrientationSwap(UnsignedPoint2D &p, DisplayOrientation orientation) noexcept
{
  if (AreAxesSwapped(orientation))
    std::swap(p.x, p.y);
}

#endif /* SOFTWARE_ROTATE_DISPLAY */

UnsignedPoint2D
OpenGL::SetupViewport(UnsignedPoint2D size) noexcept
{
  window_size = size;

  glViewport(0, 0, size.x, size.y);

#ifdef SOFTWARE_ROTATE_DISPLAY
  OrientationSwap(size, display_orientation);
#endif

  projection_matrix = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);

#ifdef SOFTWARE_ROTATE_DISPLAY
  glm::mat4 rot_matrix = glm::rotate(
    glm::mat4(1),
    (GLfloat)Angle::Degrees(OrientationToRotation(display_orientation)).Radians(),
    glm::vec3(0, 0, 1));
  projection_matrix = rot_matrix * projection_matrix;
#endif

  viewport_size = size;

  UpdateShaderProjectionMatrix();

  return size;
}

void
OpenGL::Deinitialise() noexcept
{
  DeinitShaders();

  DeinitShapes();

  TextCache::Flush();
}
