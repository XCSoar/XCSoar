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

#include "LogFile.hpp"

#include <cassert>

/**
 * How many MSAA samples should a new buffer render with?  The window
 * surface and the framebuffer objects each use as many as they can,
 * so this is not necessarily OpenGL::antialiasing_samples; a surface
 * which fell back to fewer samples must not limit the buffers.
 *
 * The chosen count is snapped down to the next lower level in
 * OpenGL::ANTIALIASING_SAMPLE_COUNTS that #available_fbo_antialiasing_samples
 * allows, rather than using every sample #fbo_antialiasing_samples
 * happens to allow, so the actual result matches what the
 * anti-aliasing selection dialog promises.
 */
[[gnu::pure]]
static GLsizei
ChooseSamples() noexcept
{
#ifdef HAVE_MULTISAMPLE_FBO
  return GLsizei(OpenGL::SelectAntialiasingSamples(
      OpenGL::requested_antialiasing_samples,
      OpenGL::available_fbo_antialiasing_samples));
#else
  return 0;
#endif
}

GLFrameBuffer &
BufferCanvas::GetDrawFrameBuffer() const noexcept
{
  assert(frame_buffer != nullptr);

  return msaa_frame_buffer != nullptr ? *msaa_frame_buffer : *frame_buffer;
}

void
BufferCanvas::CreateAttachments() noexcept
{
  assert(texture != nullptr);

  const PixelSize size = texture->GetAllocatedSize();

#ifdef HAVE_MULTISAMPLE_FBO
  if (samples > 0 &&
      OpenGL::fbo_antialiasing_mode == OpenGL::FboAntialiasingMode::BLIT) {
    /* the explicit resolve draws into a multisampled colour
       renderbuffer in a second framebuffer, and blits that into the
       texture when painting finishes */
    msaa_frame_buffer = new GLFrameBuffer();

    color_buffer = new GLRenderBuffer();
    color_buffer->Bind();
    GLRenderBuffer::StorageMultisample(samples, FBO::RGB8,
                                       size.width, size.height);
    GLRenderBuffer::Unbind();
  }
#endif

  if (OpenGL::render_buffer_stencil != GL_NONE) {
    stencil_buffer = new GLRenderBuffer();
    stencil_buffer->Bind();

#ifdef HAVE_MULTISAMPLE_FBO
    if (samples > 0)
      /* every attachment of a multisampled framebuffer must have the
         same sample count, or the framebuffer is incomplete */
      GLRenderBuffer::StorageMultisample(samples,
                                         OpenGL::render_buffer_stencil,
                                         size.width, size.height);
    else
#endif
      GLRenderBuffer::Storage(OpenGL::render_buffer_stencil,
                              size.width, size.height);

    GLRenderBuffer::Unbind();
  }
}

void
BufferCanvas::DestroyAttachments() noexcept
{
  if (stencil_buffer != nullptr || color_buffer != nullptr) {
    /* detach before deleting: a framebuffer that is not currently
       bound would keep a dangling attachment */
    GetDrawFrameBuffer().Bind();

    if (color_buffer != nullptr)
      GLRenderBuffer::DetachFramebuffer(FBO::COLOR_ATTACHMENT0);

    if (stencil_buffer != nullptr) {
      if (OpenGL::render_buffer_stencil ==
          OpenGL::render_buffer_depth_stencil)
        GLRenderBuffer::DetachFramebuffer(FBO::DEPTH_ATTACHMENT);

      GLRenderBuffer::DetachFramebuffer(FBO::STENCIL_ATTACHMENT);
    }

    GLFrameBuffer::Unbind();
  }

  delete stencil_buffer;
  stencil_buffer = nullptr;

  delete color_buffer;
  color_buffer = nullptr;

  delete msaa_frame_buffer;
  msaa_frame_buffer = nullptr;
}

void
BufferCanvas::CreateAttachmentsChecked() noexcept
{
  CreateAttachments();

#ifdef HAVE_MULTISAMPLE_FBO
  if (samples > 0) {
    /* drivers advertise multisampling and then refuse particular
       format and size combinations; check instead of assuming */
    AttachAll();
    const GLenum status = glCheckFramebufferStatus(FBO::FRAMEBUFFER);
    GLFrameBuffer::Unbind();

    if (status != GL_FRAMEBUFFER_COMPLETE) {
      LogFmt("Multisampled FBO incomplete (0x{:x}), "
             "falling back to no anti-aliasing for this buffer",
             unsigned(status));

      DestroyAttachments();
      samples = 0;
      CreateAttachments();
    }
  }
#endif
}

void
BufferCanvas::Create(PixelSize new_size) noexcept
{
  assert(!active);

  Destroy();

  samples = ChooseSamples();

  texture = new GLTexture(INTERNAL_FORMAT, new_size, FORMAT, TYPE, true);
  frame_buffer = new GLFrameBuffer();

  CreateAttachmentsChecked();

  Canvas::Create(new_size);
}

void
BufferCanvas::Destroy() noexcept
{
  assert(!active);

  if (IsDefined()) {
    DestroyAttachments();

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

  /* the renderbuffers must match the new size; reallocating them is
     simpler than resizing in place, and Activate() attaches them
     again before anything is drawn */
  DestroyAttachments();

  /* re-evaluate MSAA support: a previous resize may have fallen back
     to samples=0 after an incomplete framebuffer at a different
     size, and that should not permanently disable MSAA */
  samples = ChooseSamples();

  CreateAttachmentsChecked();

  Canvas::Create(new_size);
}

void
BufferCanvas::AttachAll() noexcept
{
  GetDrawFrameBuffer().Bind();

  bool color_attached = false;

#ifdef HAVE_MULTISAMPLE_FBO
  if (color_buffer != nullptr) {
    /* explicit resolve: draw into the multisampled renderbuffer */
    color_buffer->AttachFramebuffer(FBO::COLOR_ATTACHMENT0);
    color_attached = true;
  } else if (samples > 0) {
    /* implicit resolve: the driver keeps the multisample buffer out
       of sight and resolves into the texture on unbind */
    texture->AttachFramebufferMultisample(FBO::COLOR_ATTACHMENT0, samples);
    color_attached = true;
  }
#endif

  if (!color_attached)
    texture->AttachFramebuffer(FBO::COLOR_ATTACHMENT0);

  if (stencil_buffer != nullptr) {
    if (OpenGL::render_buffer_stencil == OpenGL::render_buffer_depth_stencil)
      /* we don't need a depth buffer, but we must attach it to the
         FBO if the stencil Renderbuffer has one */
      stencil_buffer->AttachFramebuffer(FBO::DEPTH_ATTACHMENT);

    stencil_buffer->AttachFramebuffer(FBO::STENCIL_ATTACHMENT);
  }
}

void
BufferCanvas::FinishDrawing() noexcept
{
#ifdef HAVE_MULTISAMPLE_FBO
  if (msaa_frame_buffer != nullptr) {
    const GLsizei width = GLsizei(GetWidth()), height = GLsizei(GetHeight());

    /* as an attachment target, GL_FRAMEBUFFER means the draw
       framebuffer, so the texture lands on the resolve target */
    frame_buffer->Bind(FBO::DRAW_FRAMEBUFFER);
    texture->AttachFramebuffer(FBO::COLOR_ATTACHMENT0);

    msaa_frame_buffer->Bind(FBO::READ_FRAMEBUFFER);

    FBO::BlitFramebuffer(0, 0, width, height,
                         0, 0, width, height,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }
#endif

  /* this resets the read and the draw binding alike, and is what
     triggers the implicit resolve where that is the mechanism */
  GLFrameBuffer::Unbind();
}

void
BufferCanvas::Activate() noexcept
{
  assert(IsDefined());
  assert(!active);
  assert(frame_buffer != nullptr);

  /* activate the frame buffer */
  AttachAll();

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

  FinishDrawing();

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
