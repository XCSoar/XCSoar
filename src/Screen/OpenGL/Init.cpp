/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "FBO.hpp"
#include "Screen/Custom/Cache.hpp"
#include "Asset.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#if defined(HAVE_DYNAMIC_EGL)
#include "EGL.hpp"
#endif

#include <string.h>

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
                 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    glDeleteTextures(1, &id);

    /* see if there is a complaint */
    return glGetError() == GL_NO_ERROR;
  }

  return false;
}

#ifdef HAVE_OES_DRAW_TEXTURE

gcc_pure
static bool
IsVendor(const char *_vendor)
{
  const char *vendor = (const char *)glGetString(GL_VENDOR);
  return vendor != nullptr && strcmp(vendor, _vendor) == 0;
}

gcc_pure
static bool
IsRenderer(const char *_renderer)
{
  const char *renderer = (const char *)glGetString(GL_RENDERER);
  return renderer != nullptr && strcmp(renderer, _renderer) == 0;
}

gcc_pure
static bool
IsVivanteGC600()
{
  /* found on StreetMate GTA-50-3D */
  return IsVendor("Vivante Corporation") &&
    IsRenderer("GC600 Graphics Engine");
}

gcc_pure
static bool
IsVivanteGC800()
{
  /* note: this is a Vivante GPU, but its driver declares Marvell as
     its vendor (found on Samsung GT-S5690) */
  return IsVendor("Marvell Technology Group Ltd") &&
    IsRenderer("GC800 Graphics Engine");
}

gcc_pure
static bool
IsVivanteGC1000()
{
  return IsVendor("Vivante Corporation") &&
    IsRenderer("GC1000 Graphics Engine");
}

/**
 * Is this OpenGL driver blacklisted for OES_draw_texture?
 *
 * This is a workaround to disable OES_draw_texture on certain Vivante
 * GPUs because they are known to be buggy: when combined with
 * GL_COLOR_LOGIC_OP, the left quarter of the texture is not drawn
 */
gcc_pure
static bool
IsBlacklistedOESDrawTexture()
{
  return IsAndroid() && (IsVivanteGC600() || IsVivanteGC800() ||
                         IsVivanteGC1000());
}

gcc_pure
static bool
CheckOESDrawTexture()
{
  return OpenGL::IsExtensionSupported("GL_OES_draw_texture") &&
    !IsBlacklistedOESDrawTexture();
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
  return version != NULL && strstr(version, "ES-CM 1.0") == NULL;
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
  return GL_NONE_OES;

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
  if (OpenGL::IsExtensionSupported("GL_OES_stencil1"))
    return GL_STENCIL_INDEX1_OES;

  if (OpenGL::IsExtensionSupported("GL_OES_stencil4"))
    return GL_STENCIL_INDEX4_OES;

  if (OpenGL::IsExtensionSupported("GL_OES_stencil8"))
    return GL_STENCIL_INDEX8_OES;

  /* not supported */
  return GL_NONE_OES;

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
#if defined(HAVE_DYNAMIC_EGL)
  egl = EGLInit();
#endif

  texture_non_power_of_two = SupportsNonPowerOfTwoTextures();

#ifdef HAVE_OES_DRAW_TEXTURE
  oes_draw_texture = CheckOESDrawTexture();
#endif

#ifdef ANDROID
  native_view->SetTexturePowerOfTwo(texture_non_power_of_two);

  vertex_buffer_object = EnableVBO();
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
  glDisable(GL_LIGHTING);

  glEnableClientState(GL_VERTEX_ARRAY);

  InitShapes();
}

void
OpenGL::SetupViewport(unsigned width, unsigned height)
{
  screen_width = width;
  screen_height = height;

  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
#ifdef HAVE_GLES
  glOrthox(0, width << 16, height << 16, 0, -(1<<16), 1<<16);
#else
  glOrtho(0, width, height, 0, -1, 1);
#endif

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void
OpenGL::Deinitialise()
{
  DeinitShapes();

  TextCache::Flush();
}
