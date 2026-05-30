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
#include "ui/opengl/Features.hpp"
#include "LogFile.hpp"

#ifdef SOFTWARE_ROTATE_DISPLAY
#include "DisplayOrientation.hpp"
#endif

#include <glm/gtc/matrix_transform.hpp>

#include <cassert>

static void
SetupOffscreenViewport(PixelSize render_size,
                       PixelSize logical_size,
                       unsigned pixel_scale) noexcept
{
  glViewport(0, 0, int(render_size.width), int(render_size.height));

  OpenGL::projection_matrix = glm::ortho<float>(
    0.f, float(logical_size.width),
    float(logical_size.height), 0.f, -1.f, 1.f);
  OpenGL::viewport_size = {logical_size.width, logical_size.height};
  OpenGL::viewport_pixel_scale = pixel_scale;
  OpenGL::UpdateShaderProjectionMatrix();
}

void
BufferCanvas::CreateRenderTarget(PixelSize render_size) noexcept
{
  Destroy();
  texture = new GLTexture(INTERNAL_FORMAT, render_size, FORMAT, TYPE, true);
  frame_buffer = new GLFrameBuffer();

  if (OpenGL::render_buffer_stencil != GL_NONE) {
    stencil_buffer = new GLRenderBuffer();
    stencil_buffer->Bind();
    PixelSize size = texture->GetAllocatedSize();
    stencil_buffer->Storage(OpenGL::render_buffer_stencil,
                            size.width, size.height);
    stencil_buffer->Unbind();
  }
}

void
BufferCanvas::EnsureRenderTarget(PixelSize render_size) noexcept
{
  if (!IsDefined()) {
    CreateRenderTarget(render_size);
    return;
  }

  if (texture->GetSize() == render_size)
    return;

  texture->ResizeDiscard(INTERNAL_FORMAT, render_size, FORMAT, TYPE);

  if (stencil_buffer != nullptr) {
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
}

void
BufferCanvas::Create(PixelSize new_size) noexcept
{
  assert(!active);

  CreateRenderTarget(new_size);
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

  if (new_size == GetSize() && texture->GetSize() == new_size)
    return;

  EnsureRenderTarget(new_size);
  Canvas::Create(new_size);
}

void
BufferCanvas::Begin(Canvas &other) noexcept
{
  assert(!active);

  const PixelSize logical_size = other.GetSize();
  const unsigned scale = SupersampleScale();
  const PixelSize render_size{
    logical_size.width * scale,
    logical_size.height * scale,
  };

  EnsureRenderTarget(render_size);
  Canvas::Create(logical_size);

#if OPENGL_BUFFER_SUPERSAMPLE > 1
  static bool logged_supersample = false;
  if (!logged_supersample && scale > 1) {
    LogFormat("OpenGL buffer supersampling: %ux", scale);
    logged_supersample = true;
  }
#endif

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
  old_translate = OpenGL::translate;
  old_size = OpenGL::viewport_size;
  old_viewport_pixel_scale = OpenGL::viewport_pixel_scale;

#ifdef SOFTWARE_ROTATE_DISPLAY
  old_orientation = OpenGL::display_orientation;
  OpenGL::display_orientation = DisplayOrientation::DEFAULT;
#endif

  /* logical coordinates, supersampled pixel grid */
  SetupOffscreenViewport(render_size, logical_size, scale);
  OpenGL::translate = {0, 0};

  OpenGL::UpdateShaderTranslate();

#ifndef NDEBUG
  active = true;
#endif
}

void
BufferCanvas::Commit(Canvas &other) noexcept
{
  assert(IsDefined());
  assert(active);
  assert(GetWidth() == other.GetWidth());
  assert(GetHeight() == other.GetHeight());
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
  OpenGL::viewport_pixel_scale = old_viewport_pixel_scale;

  OpenGL::UpdateShaderTranslate();

#ifdef SOFTWARE_ROTATE_DISPLAY
  OpenGL::display_orientation = old_orientation;
#endif

  /* copy frame buffer to screen */
  CopyTo(other);

#ifndef NDEBUG
  active = false;
#endif
}

void
BufferCanvas::CopyTo(Canvas &other) noexcept
{
  assert(IsDefined());
  assert(frame_buffer != nullptr);

  OpenGL::texture_shader->Use();

  texture->Bind();
  texture->Draw(other.GetRect(), PixelRect(texture->GetSize()));
}
