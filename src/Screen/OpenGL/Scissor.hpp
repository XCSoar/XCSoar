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

#ifndef XCSOAR_SCREEN_OPENGL_SCISSOR_HPP
#define XCSOAR_SCREEN_OPENGL_SCISSOR_HPP

#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/Canvas.hpp"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "Rotate.hpp"

class GLCanvasScissor : public GLEnable<GL_SCISSOR_TEST> {
public:
  explicit GLCanvasScissor(const Canvas &canvas) {
    Scissor(PixelRect(0, 0, canvas.GetWidth(), canvas.GetHeight()));
  }

  explicit GLCanvasScissor(PixelRect rc) {
    Scissor(rc);
  }

private:
  void Scissor(PixelRect rc) {
    OpenGL::ToViewport(rc);
    ::glScissor(rc.left, rc.top, rc.GetWidth(), rc.GetHeight());
  }
};

#else

class GLCanvasScissor : public GLScissor {
public:
  GLCanvasScissor(const Canvas &canvas)
    :GLScissor(OpenGL::translate.x,
               OpenGL::viewport_size.y - OpenGL::translate.y - canvas.GetHeight(),
               canvas.GetWidth(), canvas.GetHeight()) {}

  explicit GLCanvasScissor(PixelRect rc)
    :GLScissor(OpenGL::translate.x + rc.left,
               OpenGL::viewport_size.y - OpenGL::translate.y - rc.bottom,
               rc.GetWidth(), rc.GetHeight()) {}
};

#endif

#endif
