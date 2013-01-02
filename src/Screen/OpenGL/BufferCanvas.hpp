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

#ifndef XCSOAR_SCREEN_OPENGL_BUFFER_CANVAS_HPP
#define XCSOAR_SCREEN_OPENGL_BUFFER_CANVAS_HPP

#include "Canvas.hpp"
#include "Util/DebugFlag.hpp"
#include "Screen/OpenGL/Surface.hpp"

class GLTexture;
class GLFrameBuffer;
class GLRenderBuffer;

/**
 * An off-screen #Canvas implementation.
 */
class BufferCanvas : public Canvas, private GLSurfaceListener {
  GLTexture *texture;

  GLFrameBuffer *frame_buffer;

  GLRenderBuffer *stencil_buffer;

  RasterPoint old_translate;
  PixelSize old_size;

  DebugFlag active;

public:
  BufferCanvas()
    :texture(NULL), frame_buffer(NULL), stencil_buffer(NULL) {}
  BufferCanvas(const Canvas &canvas,
               UPixelScalar _width, UPixelScalar _height);
  ~BufferCanvas() {
    Destroy();
  }

  bool IsDefined() const {
    return texture != NULL;
  }

  void Create(UPixelScalar _width, UPixelScalar _height);

  void Create(const Canvas &canvas,
              UPixelScalar _width, UPixelScalar _height) {
    assert(canvas.IsDefined());

    Create(_width, _height);
  }

  void Create(const Canvas &canvas) {
    Create(canvas, canvas.GetWidth(), canvas.GetHeight());
  }

  void Destroy();

  void Resize(UPixelScalar _width, UPixelScalar _height);

  /**
   * Similar to Resize(), but never shrinks the buffer.
   */
  void Grow(UPixelScalar _width, UPixelScalar _height);

  /**
   * Begin painting to the buffer and to the specified #Canvas.
   *
   * @param other an on-screen #Canvas
   */
  void Begin(Canvas &other);

  /**
   * Commit the data that was painted into this #BufferCanvas into
   * both the buffer and the other #Canvas.
   *
   * This method must be called before the next Begin().  There is no
   * rollback method, and painting to the buffer may be destructive
   * for the "other" #Canvas until Commit() is called.
   *
   * @param other an on-screen #Canvas
   */
  void Commit(Canvas &other);

  void CopyTo(Canvas &other);

#ifdef ENABLE_OPENGL
private:
  /* from GLSurfaceListener */
  virtual void SurfaceCreated() gcc_override;
  virtual void SurfaceDestroyed() gcc_override;
#endif
};

#endif
