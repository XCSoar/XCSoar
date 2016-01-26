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

#include "Screen/OpenGL/Init.hpp"
#include "Screen/OpenGL/Debug.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Extension.hpp"
#include "Screen/OpenGL/Features.hpp"
#include "Screen/OpenGL/Shapes.hpp"
#include "Function.hpp"
#include "Dynamic.hpp"
#include "FBO.hpp"
#include "Screen/Custom/Cache.hpp"
#include "Math/Point2D.hpp"
#include "Asset.hpp"
#include "DisplayOrientation.hpp"

#ifdef USE_EGL
#include "Screen/EGL/System.hpp"
#endif

#ifdef USE_GLSL
#include "Shaders.hpp"
#endif

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef USE_GLSL
#include <glm/gtc/matrix_transform.hpp>
#endif

#include <algorithm>

#include <assert.h>
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
 * Does the current GLES context support textures with dimensions
 * other than power-of-two?
 */
gcc_pure
static bool
SupportsNonPowerOfTwoTexturesGLES()
{
  /* the Dell Streak Mini announces this extension */
  if (OpenGL::IsExtensionSupported("GL_APPLE_texture_2D_limited_npot"))
    return true;

  /* this extension is announced by all modern Android 2.2 handsets,
     however the HTC Desire HD (Adreno 205 GPU) is unable to create
     such textures - not a reliable indicator, it seems */
  if (OpenGL::IsExtensionSupported("GL_OES_texture_npot")) {
    /* flush previous errors */
    while (glGetError() != GL_NO_ERROR) {}

    /* attempt to create an odd texture */
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 11, 11, 0,
                 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr);
    glDeleteTextures(1, &id);

    /* see if there is a complaint */
    return glGetError() == GL_NO_ERROR;
  }

  return false;
}

#ifdef HAVE_OES_DRAW_TEXTURE

gcc_pure
static bool
CheckOESDrawTexture()
{
  return OpenGL::IsExtensionSupported("GL_OES_draw_texture");
}

#endif

#ifdef ANDROID
/**
 * Is it safe to use VBO?
 */
gcc_pure
static bool
EnableVBO()
{
  /* disable VBO on Android with OpenGL/ES 1.0 (that's Android 1.6 and
     the Android emulator) - on those versions, glDeleteBuffers()
     crashes instantly, see
     http://code.google.com/p/android/issues/detail?id=4273 */
  const char *version = (const char *)glGetString(GL_VERSION);
  return version != nullptr && strstr(version, "ES-CM 1.0") == nullptr;
}
#endif

/**
 * Does the current OpenGL context support textures with dimensions
 * other than power-of-two?
 */
gcc_pure
static bool
SupportsNonPowerOfTwoTextures()
{
  return OpenGL::IsExtensionSupported("GL_ARB_texture_non_power_of_two") ||
    (HaveGLES() && SupportsNonPowerOfTwoTexturesGLES());
}

gcc_pure
static bool
CheckFBO()
{
  return OpenGL::IsExtensionSupported(HaveGLES()
                                      ? "GL_OES_framebuffer_object"
                                      : "GL_EXT_framebuffer_object");
}

/**
 * Check which depth+stencil internalType is available for a
 * Renderbuffer.  Returns GL_NONE if the Renderbuffer does not support
 * it.
 */
gcc_pure
static GLenum
CheckDepthStencil()
{
#ifdef HAVE_GLES
  if (OpenGL::IsExtensionSupported("GL_OES_packed_depth_stencil"))
    return GL_DEPTH24_STENCIL8_OES;

  /* not supported */
#ifdef HAVE_GLES2
  return GL_NONE;
#else
  return GL_NONE_OES;
#endif

#else

  if (OpenGL::IsExtensionSupported("GL_EXT_packed_depth_stencil"))
    return FBO::DEPTH_STENCIL;

  /* not supported */
  return GL_NONE;
#endif
}

/**
 * Check which stencil internalType is available for a Renderbuffer.
 * Returns GL_NONE if the Renderbuffer does not support it.
 */
gcc_pure
static GLenum
CheckStencil()
{
#ifdef HAVE_GLES
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
  if (OpenGL::IsExtensionSupported("GL_OES_stencil1"))
    return GL_STENCIL_INDEX1_OES;

  if (OpenGL::IsExtensionSupported("GL_OES_stencil4"))
    return GL_STENCIL_INDEX4_OES;
#endif

  if (OpenGL::IsExtensionSupported("GL_OES_stencil8")) {
#ifdef HAVE_GLES2
    return GL_STENCIL_INDEX8;
#else
    return GL_STENCIL_INDEX8_OES;
#endif
  }

  /* not supported */
#ifdef HAVE_GLES2
  return GL_NONE;
#else
  return GL_NONE_OES;
#endif

#else

#if 0
  /* this one works with Nvidia GF114 on Linux, but
     https://www.opengl.org/wiki/Image_Format strongly recommends not
     using it */
#ifdef GL_STENCIL_INDEX8
  return GL_STENCIL_INDEX8;
#else
  return GL_STENCIL_INDEX8_EXT;
#endif
#endif

  /* not supported */
  return GL_NONE;
#endif
}

void
OpenGL::SetupContext()
{
  texture_non_power_of_two = SupportsNonPowerOfTwoTextures();

#ifdef HAVE_OES_DRAW_TEXTURE
  oes_draw_texture = CheckOESDrawTexture();
#endif

#ifdef ANDROID
  native_view->SetTexturePowerOfTwo(texture_non_power_of_two);

  vertex_buffer_object = EnableVBO();
#endif

#ifdef HAVE_OES_MAPBUFFER
  mapbuffer = IsExtensionSupported("GL_OES_mapbuffer");
#endif

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

  frame_buffer_object = CheckFBO() && FBO::Initialise();
  if (frame_buffer_object) {
    render_buffer_depth_stencil = CheckDepthStencil();

    render_buffer_stencil = CheckStencil();
    if (!render_buffer_stencil)
      /* fall back to a packed depth+stencil format */
      render_buffer_stencil = render_buffer_depth_stencil;
  }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
#ifndef HAVE_GLES2
  glDisable(GL_LIGHTING);
#endif

#ifndef USE_GLSL
  glEnableClientState(GL_VERTEX_ARRAY);
#endif

  InitShapes();

#ifdef USE_GLSL
  InitShaders();
#endif
}

#ifdef SOFTWARE_ROTATE_DISPLAY

/**
 * Determine the projection rotation angle (in degrees) of the
 * specified orientation.
 */
gcc_const
static GLfloat
OrientationToRotation(DisplayOrientation orientation)
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
OrientationSwap(UnsignedPoint2D &p, DisplayOrientation orientation)
{
  if (AreAxesSwapped(orientation))
    std::swap(p.x, p.y);
}

#endif /* SOFTWARE_ROTATE_DISPLAY */

UnsignedPoint2D
OpenGL::SetupViewport(UnsignedPoint2D size)
{
  window_size = size;

  glViewport(0, 0, size.x, size.y);

#ifdef USE_GLSL
#ifdef SOFTWARE_ROTATE_DISPLAY
  projection_matrix = glm::rotate(glm::mat4(),
                                  OrientationToRotation(display_orientation),
                                  glm::vec3(0, 0, 1));
  OrientationSwap(size, display_orientation);
#endif
  projection_matrix = glm::ortho<float>(0, size.x, size.y, 0, -1, 1);
  UpdateShaderProjectionMatrix();
#else
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
#ifdef SOFTWARE_ROTATE_DISPLAY
  glRotatef(OrientationToRotation(display_orientation), 0, 0, 1);
  OrientationSwap(size, display_orientation);
#endif
#ifdef HAVE_GLES
  glOrthox(0, size.x << 16, size.y << 16, 0, -(1<<16), 1<<16);
#else
  glOrtho(0, size.x, size.y, 0, -1, 1);
#endif

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#endif

  viewport_size = size;

  return size;
}

void
OpenGL::Deinitialise()
{
#ifdef USE_GLSL
  DeinitShaders();
#endif

  DeinitShapes();

  TextCache::Flush();
}
