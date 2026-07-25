// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DitherPass.hpp"
#include "Shaders.hpp"
#include "Program.hpp"
#include "Texture.hpp"
#include "FrameBuffer.hpp"
#include "FBO.hpp"
#include "Globals.hpp"
#include "Scope.hpp"
#include "VertexPointer.hpp"
#include "Attribute.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Luminosity.hpp"
#include "ui/canvas/memory/Dither.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "ui/dim/Rect.hpp"
#include "util/AllocatedArray.hxx"

#include <algorithm>
#include <cstring>
#include <memory>

namespace OpenGL {

bool enable_dither_pass = false;

DitherAlgorithm dither_algorithm = DitherAlgorithm::SIERRA_LITE;

DitherPassSettings dither_pass_settings;

static Dither sierra_lite_dither;

static std::unique_ptr<GLTexture> capture_texture;

static std::unique_ptr<GLFrameBuffer> terrain_fbo;
static std::unique_ptr<GLTexture> terrain_render_texture;
static std::unique_ptr<GLTexture> terrain_result_texture;
static PixelSize terrain_fbo_size;

static void
EnsureCaptureTexture(PixelSize size)
{
  if (capture_texture == nullptr ||
      capture_texture->GetSize() != size)
    capture_texture.reset(new GLTexture(GL_RGB, size, GL_RGB,
                                        GL_UNSIGNED_BYTE, true));
}

static void
EnsureTerrainFBO(PixelSize size)
{
  if (terrain_fbo != nullptr && terrain_fbo_size == size)
    return;

  GLint saved_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saved_framebuffer);

  terrain_fbo_size = size;
  terrain_render_texture.reset(new GLTexture(GL_RGB, size, GL_RGB,
                                             GL_UNSIGNED_BYTE, false));
  terrain_result_texture.reset(new GLTexture(GL_RGB, size, GL_RGB,
                                             GL_UNSIGNED_BYTE, false));
  terrain_fbo = std::make_unique<GLFrameBuffer>();
  terrain_fbo->Bind();
  terrain_render_texture->AttachFramebuffer(FBO::COLOR_ATTACHMENT0);

  FBO::BindFramebuffer(FBO::FRAMEBUFFER, GLuint(saved_framebuffer));
}

static void
DrawTexturedQuad(const BulkPixelPoint (&vertices)[4],
                 const GLfloat *texcoord, GLProgram &shader) noexcept
{
  const ScopeVertexPointer vp(vertices);

  shader.Use();
  glEnableVertexAttribArray(Attribute::TEXCOORD);
  glVertexAttribPointer(Attribute::TEXCOORD, 2, GL_FLOAT, GL_FALSE,
                        0, texcoord);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableVertexAttribArray(Attribute::TEXCOORD);
}

static void
FlipRowsVertically(uint8_t *data, unsigned width, unsigned height) noexcept
{
  const unsigned row_bytes = width * 3;
  static AllocatedArray<uint8_t> temp_row;
  temp_row.GrowDiscard(row_bytes);

  for (unsigned y = 0; y < height / 2; ++y) {
    uint8_t *row_a = data + y * row_bytes;
    uint8_t *row_b = data + (height - 1 - y) * row_bytes;
    std::memcpy(temp_row.data(), row_a, row_bytes);
    std::memcpy(row_a, row_b, row_bytes);
    std::memcpy(row_b, temp_row.data(), row_bytes);
  }
}

static void
BuildViewportTexcoords(const BulkPixelPoint (&vertices)[4],
                       const PixelSize &allocated,
                       GLfloat result[8]) noexcept
{
  for (unsigned i = 0; i < 4; ++i) {
    result[i * 2] = GLfloat(int(vertices[i].x)) / GLfloat(allocated.width);
    result[i * 2 + 1] =
      GLfloat(int(vertices[i].y)) / GLfloat(allocated.height);
  }
}

void
ApplyDitherUniforms() noexcept
{
  dither_shader->Use();
  glUniform1f(dither_snap_high, dither_pass_settings.snap_high);
  glUniform1f(dither_snap_low, dither_pass_settings.snap_low);
  glUniform1f(dither_gamma, dither_pass_settings.gamma);
}

static void
DrawGeoBitmapBayer(const GLTexture &texture,
                   const BulkPixelPoint (&vertices)[4],
                   const GLfloat *texcoord) noexcept
{
  ApplyDitherUniforms();
  const_cast<GLTexture &>(texture).Bind();
  DrawTexturedQuad(vertices, texcoord, *dither_shader);
}

static void
DrawGeoBitmapSierraLite(const GLTexture &texture,
                        const BulkPixelPoint (&vertices)[4],
                        const GLfloat *texcoord) noexcept
{
  GLint saved_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saved_framebuffer);

  GLint saved_viewport[4];
  glGetIntegerv(GL_VIEWPORT, saved_viewport);
  const unsigned viewport_width = unsigned(saved_viewport[2]);
  const unsigned viewport_height = unsigned(saved_viewport[3]);
  if (viewport_width == 0 || viewport_height == 0)
    return;

  const PixelSize viewport_size{viewport_width, viewport_height};
  EnsureTerrainFBO(viewport_size);

  terrain_fbo->Bind();
  glViewport(0, 0, viewport_width, viewport_height);

  glClearColor(1.f, 1.f, 1.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  const_cast<GLTexture &>(texture).Bind();
  DrawTexturedQuad(vertices, texcoord, *luminosity_shader);

  const unsigned n_pixels = viewport_width * viewport_height;
  static AllocatedArray<uint8_t> rgb_buffer;
  static AllocatedArray<uint8_t> grey_buffer;
  static AllocatedArray<uint8_t> output_buffer;
  rgb_buffer.GrowDiscard(n_pixels * 3);
  grey_buffer.GrowDiscard(n_pixels);
  output_buffer.GrowDiscard(n_pixels);

  glReadPixels(0, 0, viewport_width, viewport_height,
               GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer.data());

  FlipRowsVertically(rgb_buffer.data(), viewport_width, viewport_height);

  const uint8_t *src = rgb_buffer.data();
  uint8_t *grey = grey_buffer.data();
  for (unsigned i = 0; i < n_pixels; ++i, src += 3)
    grey[i] = Luminosity::FromRGB(src[0], src[1], src[2]);

  sierra_lite_dither.DitherGreyscale(grey, viewport_width,
                                     output_buffer.data(), viewport_width,
                                     viewport_width, viewport_height);

  uint8_t *dest_pixels = rgb_buffer.data();
  const uint8_t *out = output_buffer.data();
  for (unsigned i = 0; i < n_pixels; ++i, dest_pixels += 3, ++out) {
    const uint8_t v = *out ? 0xff : 0x00;
    dest_pixels[0] = dest_pixels[1] = dest_pixels[2] = v;
  }

  FBO::BindFramebuffer(FBO::FRAMEBUFFER, GLuint(saved_framebuffer));
  glViewport(saved_viewport[0], saved_viewport[1],
             saved_viewport[2], saved_viewport[3]);

  terrain_result_texture->Bind();
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                 viewport_width, viewport_height,
                 GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer.data());

  GLfloat result_texcoord[8];
  BuildViewportTexcoords(vertices,
                         terrain_result_texture->GetAllocatedSize(),
                         result_texcoord);
  const_cast<GLTexture &>(*terrain_result_texture).Bind();
  DrawTexturedQuad(vertices, result_texcoord, *texture_shader);
}

void
DrawGeoBitmapWithDither(const GLTexture &texture, [[maybe_unused]] PixelSize bitmap_size,
                        const BulkPixelPoint (&vertices)[4],
                        const GLfloat *texcoord)
{
  switch (dither_algorithm) {
  case DitherAlgorithm::NONE:
    const_cast<GLTexture &>(texture).Bind();
    DrawTexturedQuad(vertices, texcoord, *luminosity_shader);
    break;

  case DitherAlgorithm::BAYER:
    DrawGeoBitmapBayer(texture, vertices, texcoord);
    break;

  case DitherAlgorithm::SIERRA_LITE:
    DrawGeoBitmapSierraLite(texture, vertices, texcoord);
    break;
  }
}

void
ApplyGreyscalePass(Canvas &canvas)
{
  if (!enable_dither_pass)
    return;

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  const PixelSize size{unsigned(viewport[2]), unsigned(viewport[3])};
  if (size.width == 0 || size.height == 0)
    return;

  GLFrameBuffer::Unbind();
  EnsureCaptureTexture(size);

  capture_texture->Bind();
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                      size.width, size.height);

  canvas.ClearWhite();

  luminosity_shader->Use();
  capture_texture->Bind();
  capture_texture->Draw(canvas.GetRect(), capture_texture->GetRect());
}

} // namespace OpenGL
