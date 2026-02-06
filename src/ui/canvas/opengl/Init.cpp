// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Init.hpp"
#include "Debug.hpp"
#include "Extension.hpp"
#include "Globals.hpp"
#include "Function.hpp"
#include "Dynamic.hpp"
#include "FBO.hpp"
#include "ui/canvas/custom/Cache.hpp"
#include "ui/opengl/Features.hpp"
#include "Math/Point2D.hpp"
#include "Asset.hpp"
#include "DisplayOrientation.hpp"
#include "LogFile.hpp"

#ifndef HAVE_GLES2
#ifdef _WIN32
#include <glad/glad.h>
#endif
#ifdef ENABLE_SDL
#include <SDL_video.h>
#endif
#endif

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
#ifdef _WIN32
  thread = GetCurrentThreadId();
#else
  thread = pthread_self();
#endif
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
#ifdef HAVE_GLES2
  return OpenGL::IsExtensionSupported("GL_OES_texture_npot");
#else
  return true; /* Desktop OpenGL 2.0+ always supports NPOT textures */
#endif
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
#ifdef HAVE_GLES2
  if (OpenGL::IsExtensionSupported("GL_OES_packed_depth_stencil"))
    return GL_DEPTH24_STENCIL8_OES;

  /* not supported */
  return GL_NONE;
#else
  return GL_DEPTH24_STENCIL8; /* DesktopGL has packed depth+stencil via GL_DEPTH24_STENCIL8 */
#endif

}

/**
 * Check which stencil internalType is available for a Renderbuffer.
 * Returns GL_NONE if the Renderbuffer does not support it.
 */
[[gnu::pure]]
static GLenum
CheckStencil() noexcept
{
#ifdef HAVE_GLES2
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
#else
  return GL_STENCIL_INDEX8; /* Desktop OpenGL supports 8-bit stencil */
#endif
}

void
OpenGL::SetupContext()
{
#ifdef _WIN32
#ifndef HAVE_GLES2
  // Load OpenGL functions via glad (required on Windows where opengl32 only exports GL 1.1)
#ifdef ENABLE_SDL
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    LogFormat("Failed to initialize OpenGL loader (glad)");
    return;
  }
#else
  if (!gladLoadGL()) {
    LogFormat("Failed to initialize OpenGL loader (glad)");
    return;
  }
#endif
  LogFormat("OpenGL %d.%d loaded via glad", GLVersion.major, GLVersion.minor);
#endif
#endif

  if (auto s = (const char *)glGetString(GL_VENDOR))
    LogFormat("GL vendor: %s", s);

  if (auto s = (const char *)glGetString(GL_VERSION))
    LogFormat("GL version: %s", s);

  if (auto s = (const char *)glGetString(GL_RENDERER)) {
    LogFormat("GL renderer: %s", s);

    if (strstr(s, "Mali400") == s) {
      /* Allwinner A20 SoC has a Mali-400 GPU. Limit the map scale to
         250 km. */
      max_map_scale = 251*1000*2;
    } else if (strstr(s, "PowerVR Rogue GE8300") != nullptr) {
      /* PowerVR Rogue GE8300 (MediaTek MT8166) crashes in the driver
         when rendering large topography polygons at extreme zoom-out.
         Limit the maximum map scale to avoid triggering the bug.
         See https://github.com/XCSoar/XCSoar/issues/1235 */
      max_map_scale = 300000;
    }
  }

  if (auto s = (const char *)glGetString(GL_EXTENSIONS))
    LogFormat("GL extensions: %s", s);

  texture_non_power_of_two = SupportsNonPowerOfTwoTextures();

#ifdef ANDROID
  if (native_view != nullptr)
    native_view->SetTexturePowerOfTwo(Java::GetEnv(), texture_non_power_of_two);
#endif

#ifdef HAVE_GLES2
  mapbuffer = IsExtensionSupported("GL_OES_mapbuffer");
#else
  mapbuffer = true; /* Desktop OpenGL always has glMapBuffer */
#endif

#ifdef HAVE_DYNAMIC_MAPBUFFER
#ifdef HAVE_GLES2
  if (mapbuffer) {
    GLExt::map_buffer = (PFNGLMAPBUFFEROESPROC)
      GetProcAddress("glMapBufferOES");
    GLExt::unmap_buffer = (PFNGLUNMAPBUFFEROESPROC)
      GetProcAddress("glUnmapBufferOES");
    if (GLExt::map_buffer == nullptr || GLExt::unmap_buffer == nullptr)
      mapbuffer = false;
  }
#else
  /* Desktop OpenGL uses standard glMapBuffer/glUnmapBuffer */
  if (mapbuffer) {
    GLExt::map_buffer = (PFNGLMAPBUFFERPROC)
      GetProcAddress("glMapBuffer");
    GLExt::unmap_buffer = (PFNGLUNMAPBUFFERPROC)
      GetProcAddress("glUnmapBuffer");
    if (GLExt::map_buffer == nullptr || GLExt::unmap_buffer == nullptr)
      mapbuffer = false;
  }
#endif
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

#ifdef GL_EXT_discard_framebuffer
  if (IsExtensionSupported("GL_EXT_discard_framebuffer")) {
    GLExt::discard_framebuffer = (PFNGLDISCARDFRAMEBUFFEREXTPROC)
      GetProcAddress("glDiscardFramebufferEXT");
  } else {
    GLExt::discard_framebuffer = nullptr;
  }
#endif

  render_buffer_depth_stencil = CheckDepthStencil();

  render_buffer_stencil = CheckStencil();
  if (!render_buffer_stencil)
    /* fall back to a packed depth+stencil format */
    render_buffer_stencil = render_buffer_depth_stencil;

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_DITHER);

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

  TextCache::Flush();
}
