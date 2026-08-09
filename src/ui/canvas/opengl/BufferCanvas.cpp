// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/BufferCanvas.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "Globals.hpp"
#include "Texture.hpp"
#include "FrameBuffer.hpp"
#include "RenderBuffer.hpp"
#include "Init.hpp"
#include "Shaders.hpp"
#include "Program.hpp"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "DisplayOrientation.hpp"
#endif

#include <cassert>

void
BufferCanvas::Create(PixelSize new_size) noexcept
{
  assert(!active);

  Destroy();
  texture = new GLTexture(INTERNAL_FORMAT, new_size, FORMAT, TYPE, true);
  frame_buffer = new GLFrameBuffer();

  if (OpenGL::render_buffer_stencil != GL_NONE) {
    stencil_buffer = new GLRenderBuffer();
    stencil_buffer->Bind();
    PixelSize size = texture->GetAllocatedSize();
    stencil_buffer->Storage(OpenGL::render_buffer_stencil,
                            size.width, size.height);
    stencil_buffer->Unbind();
  }

  Canvas::Create(new_size);
}

void
BufferCanvas::Destroy() noexcept
{
  assert(!active);

  if (IsDefined()) {
    delete stencil_buffer;
    stencil_buffer = nullptr;

    delete frame_buffer;
    frame_buffer = nullptr;

    delete texture;
    texture = nullptr;
  }
}

void
BufferCanvas::Resize(PixelSize new_size) noexcept
{
  assert(IsDefined());

  if (new_size == GetSize())
    return;

  texture->ResizeDiscard(INTERNAL_FORMAT, new_size, FORMAT, TYPE);

  if (stencil_buffer != nullptr) {
    /* the stencil buffer must be detached before we resize it */
    frame_buffer->Bind();
    if (OpenGL::render_buffer_stencil == OpenGL::render_buffer_depth_stencil)
      stencil_buffer->DetachFramebuffer(FBO::DEPTH_ATTACHMENT);
    stencil_buffer->DetachFramebuffer(FBO::STENCIL_ATTACHMENT);
    frame_buffer->Unbind();

    stencil_buffer->Bind();
    PixelSize size = texture->GetAllocatedSize();
    stencil_buffer->Storage(OpenGL::render_buffer_stencil,
                            size.width, size.height);
    stencil_buffer->Unbind();
  }

  Canvas::Create(new_size);
}

void
BufferCanvas::Activate() noexcept
{
  assert(IsDefined());
  assert(!active);
  assert(frame_buffer != nullptr);

  /* activate the frame buffer */
  frame_buffer->Bind();
  texture->AttachFramebuffer(FBO::COLOR_ATTACHMENT0);

  if (stencil_buffer != nullptr) {
    if (OpenGL::render_buffer_stencil == OpenGL::render_buffer_depth_stencil)
      /* we don't need a depth buffer, but we must attach it to the
         FBO if the stencil Renderbuffer has one */
      stencil_buffer->AttachFramebuffer(FBO::DEPTH_ATTACHMENT);

    stencil_buffer->AttachFramebuffer(FBO::STENCIL_ATTACHMENT);
  }

  /* save the old viewport */

  glGetIntegerv(GL_VIEWPORT, old_viewport);

  old_projection_matrix = OpenGL::projection_matrix;
  OpenGL::projection_matrix = glm::mat4(1);

  old_translate = OpenGL::translate;
  old_size = OpenGL::viewport_size;

  /* Parent paint may have enabled a screen-space scissor (e.g.
     VScrollPanel).  That box is wrong for this FBO's viewport and
     would clip strip fills to a band that shifts with layout. */
  old_scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);
  if (old_scissor_enabled)
    glDisable(GL_SCISSOR_TEST);

#ifdef SOFTWARE_ROTATE_DISPLAY
  old_orientation = OpenGL::display_orientation;
  OpenGL::display_orientation = DisplayOrientation::DEFAULT;
#endif

  /* configure a new viewport */
  OpenGL::SetupViewport({GetWidth(), GetHeight()});
  OpenGL::translate = {0, 0};

  OpenGL::UpdateShaderTranslate();

#ifndef NDEBUG
  active = true;
#endif
}

void
BufferCanvas::Deactivate() noexcept
{
  assert(IsDefined());
  assert(active);
  assert(frame_buffer != nullptr);

  assert(OpenGL::translate.x == 0);
  assert(OpenGL::translate.y == 0);

  frame_buffer->Unbind();

  /* restore the old viewport */

  assert(OpenGL::translate == PixelPoint(0, 0));

  glViewport(old_viewport[0], old_viewport[1],
             old_viewport[2], old_viewport[3]);

  OpenGL::projection_matrix = old_projection_matrix;
  OpenGL::UpdateShaderProjectionMatrix();

  OpenGL::translate = old_translate;
  OpenGL::viewport_size = old_size;

  OpenGL::UpdateShaderTranslate();

  if (old_scissor_enabled)
    glEnable(GL_SCISSOR_TEST);

#ifdef SOFTWARE_ROTATE_DISPLAY
  OpenGL::display_orientation = old_orientation;
#endif

#ifndef NDEBUG
  active = false;
#endif
}

void
BufferCanvas::Begin() noexcept
{
  Activate();
}

void
BufferCanvas::Begin(Canvas &other) noexcept
{
  assert(IsDefined());

  Resize(other.GetSize());
  Activate();
}

void
BufferCanvas::End() noexcept
{
  Deactivate();
}

void
BufferCanvas::Commit(Canvas &other) noexcept
{
  assert(IsDefined());
  assert(active);
  assert(GetWidth() == other.GetWidth());
  assert(GetHeight() == other.GetHeight());

  End();
  CopyTo(other);
}

void
BufferCanvas::CopyTo(Canvas &other) noexcept
{
  CopyTo(other, other.GetRect(), GetRect());
}

void
BufferCanvas::CopyTo([[maybe_unused]] Canvas &dest, PixelRect dest_rc,
                     PixelRect src_rc) noexcept
{
  assert(IsDefined());
  assert(frame_buffer != nullptr);
  assert(!active);

  /* FBO-backed buffers use a flipped texture.  Full-buffer CopyTo is
     fine (the whole image is inverted as a unit), but a partial source
     rectangle would otherwise show the strip upside-down and invert
     scroll direction.  Remap canvas-space Y into the flipped texel
     space that #GLTexture::Draw expects. */
  if (texture->IsFlipped()) {
    const int buffer_h = static_cast<int>(GetHeight());
    const int src_h = static_cast<int>(src_rc.GetHeight());
    const int new_top = buffer_h - src_rc.bottom;
    src_rc.top = new_top;
    src_rc.bottom = new_top + src_h;
  }

  OpenGL::texture_shader->Use();

  texture->Bind();
  texture->Draw(dest_rc, src_rc);
}
