/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/OpenGL/Cache.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Extension.hpp"
#include "Screen/OpenGL/Features.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
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
    (have_gles() && SupportsNonPowerOfTwoTexturesGLES());
}

void
OpenGL::SetupContext()
{
  texture_non_power_of_two = SupportsNonPowerOfTwoTextures();

#ifdef ANDROID
  native_view->SetTexturePowerOfTwo(texture_non_power_of_two);

  vertex_buffer_object = EnableVBO();
#endif

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
  glDisable(GL_LIGHTING);

  glEnableClientState(GL_VERTEX_ARRAY);
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
  TextCache::flush();
}
