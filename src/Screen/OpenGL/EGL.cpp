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

#include "EGL.hpp"

#include <EGL/egl.h>

#include <assert.h>
#include <dlfcn.h>

typedef EGLDisplay (*T_eglGetDisplay)(EGLNativeDisplayType display_id);
typedef EGLSurface (*T_eglGetCurrentSurface)(EGLint readdraw);
typedef EGLBoolean (*T_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);

static T_eglGetDisplay _eglGetDisplay;
static T_eglGetCurrentSurface _eglGetCurrentSurface;
static T_eglSwapBuffers _eglSwapBuffers;

static EGLDisplay display;
static EGLSurface surface;

bool
EGLInit()
{
  void *egl = dlopen("libEGL.so", RTLD_NOW);
  if (egl == NULL)
    return false;

  _eglGetDisplay = (T_eglGetDisplay)dlsym(egl, "eglGetDisplay");
  _eglGetCurrentSurface = (T_eglGetCurrentSurface)
    dlsym(egl, "eglGetCurrentSurface");
  _eglSwapBuffers = (T_eglSwapBuffers)dlsym(egl, "eglSwapBuffers");

  if (_eglGetDisplay == NULL || _eglGetCurrentSurface == NULL ||
      _eglSwapBuffers == NULL)
    return false;

  display = _eglGetDisplay(EGL_DEFAULT_DISPLAY);
  surface = _eglGetCurrentSurface(EGL_DRAW);
  if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE)
    return false;

  return true;
}

void
EGLSwapBuffers()
{
  assert(_eglSwapBuffers != NULL);

  _eglSwapBuffers(display, surface);
}
