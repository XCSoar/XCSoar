// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ForwardView/ForwardViewTopographyTexture.hpp"

#ifdef ENABLE_OPENGL

#include "ForwardView/ForwardViewGeometry.hpp"
#include "ForwardView/ForwardViewTerrain.hpp"
#include "Look/TopographyLook.hpp"
#include "Projection/WindowProjection.hpp"
#include "Topography/ForwardViewTopographyOverlay.hpp"
#include "Topography/TopographyRenderer.hpp"
#include "Topography/TopographyStore.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/opengl/FBO.hpp"
#include "ui/canvas/opengl/FrameBuffer.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/canvas/opengl/Init.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#include "ui/canvas/opengl/Shaders.hpp"
#include "ui/canvas/opengl/Texture.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace ForwardViewTopographyTexture {

static WindowProjection
BuildProjection(GeoPoint start, Angle track, double range,
                float aspect) noexcept
{
  return ForwardViewTopography::CorridorProjection(
    start, track, range, aspect,
    PixelRect(PixelSize{WIDTH, HEIGHT}));
}

Cache::Cache(const TopographyLook &_look) noexcept
  :look(_look) {}

Cache::~Cache() noexcept = default;

void
Cache::SetStore(TopographyStore *_store) noexcept
{
  if (store == _store)
    return;

  store = _store;
  renderer.reset();
  Invalidate();
}

void
Cache::Invalidate() noexcept
{
  valid = false;
}

void
Cache::BindTexture() const noexcept
{
  if (texture != nullptr) {
    glActiveTexture(GL_TEXTURE0);
    texture->Bind();
  }
}

UV
Cache::ComputeUV(double x, double y) const noexcept
{
  if (!valid || texture == nullptr)
    return {0.f, 0.f};

  const GeoPoint geo =
    ForwardViewTerrain::LocalToGeo(anchor, track, x, y);
  const WindowProjection projection =
    BuildProjection(anchor, track, range, aspect);
  const PixelPoint pixel = projection.GeoToScreen(geo);

  const unsigned tw = texture->GetWidth();
  const unsigned th = texture->GetHeight();
  if (tw == 0 || th == 0)
    return {0.f, 0.f};

  return {
    float(pixel.x) / float(tw),
    1.f - float(pixel.y) / float(th),
  };
}

bool
Cache::Ensure(GeoPoint build_anchor, Angle build_track,
              double build_range, float build_aspect,
              unsigned build_serial, bool *rebuilt) noexcept
{
  if (rebuilt != nullptr)
    *rebuilt = false;

  if (store == nullptr)
    return false;

  auto stale = [&]() noexcept {
    if (!valid)
      return true;

    if (store_serial != build_serial ||
        !build_track.CompareRoughly(track) ||
        build_range != range ||
        build_aspect != aspect)
      return true;

    const GeoVector rel(anchor, build_anchor);
    const Angle rel_bearing = rel.bearing - track;
    const double along = rel.distance * rel_bearing.cos();
    return along < 0. || along >= range * 0.7;
  };

  if (!stale())
    return texture != nullptr;

  if (renderer == nullptr)
    renderer = std::make_unique<TopographyRenderer>(*store, look);

  if (texture == nullptr) {
    texture = std::make_unique<GLTexture>(GL_RGBA, PixelSize{WIDTH, HEIGHT},
                                          GL_RGBA, GL_UNSIGNED_BYTE, false);
    frame_buffer = std::make_unique<GLFrameBuffer>();
  }

  GLint old_viewport[4];
  glGetIntegerv(GL_VIEWPORT, old_viewport);
  GLint old_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);
  GLboolean depth_was_enabled = glIsEnabled(GL_DEPTH_TEST);
  GLboolean stencil_was_enabled = glIsEnabled(GL_STENCIL_TEST);
  GLboolean scissor_was_enabled = glIsEnabled(GL_SCISSOR_TEST);

  const glm::mat4 old_projection_matrix = OpenGL::projection_matrix;
  const PixelPoint old_translate = OpenGL::translate;
  const UnsignedPoint2D old_size = OpenGL::viewport_size;

  frame_buffer->Bind();
  texture->AttachFramebuffer(FBO::COLOR_ATTACHMENT0);

  if (glCheckFramebufferStatus(FBO::FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    frame_buffer->Unbind();
    glBindFramebuffer(FBO::FRAMEBUFFER, GLuint(old_framebuffer));
    valid = false;
    return false;
  }

  if (depth_was_enabled)
    glDisable(GL_DEPTH_TEST);
  if (stencil_was_enabled)
    glDisable(GL_STENCIL_TEST);
  if (scissor_was_enabled)
    glDisable(GL_SCISSOR_TEST);

  const ScopeAlphaBlend blend;
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  OpenGL::SetupViewport({WIDTH, HEIGHT});

  Canvas canvas;
  canvas.Create(PixelSize{WIDTH, HEIGHT});

  WindowProjection projection =
    BuildProjection(build_anchor, build_track, build_range, build_aspect);

  const double map_scale =
    ForwardViewTopography::ForwardViewMapScale(
      PixelRect(PixelSize{WIDTH, HEIGHT}), build_range, build_aspect);
  store->ScanVisibility(projection, 4, map_scale);

  renderer->Draw(canvas, projection, false);

  frame_buffer->Unbind();
  glBindFramebuffer(FBO::FRAMEBUFFER, GLuint(old_framebuffer));
  glViewport(old_viewport[0], old_viewport[1],
             old_viewport[2], old_viewport[3]);

  OpenGL::projection_matrix = old_projection_matrix;
  OpenGL::translate = old_translate;
  OpenGL::viewport_size = old_size;
  OpenGL::UpdateShaderProjectionMatrix();
  OpenGL::UpdateShaderTranslate();

  if (depth_was_enabled)
    glEnable(GL_DEPTH_TEST);
  if (stencil_was_enabled)
    glEnable(GL_STENCIL_TEST);
  if (scissor_was_enabled)
    glEnable(GL_SCISSOR_TEST);

  valid = true;
  anchor = build_anchor;
  track = build_track;
  range = build_range;
  aspect = build_aspect;
  store_serial = build_serial;

  if (rebuilt != nullptr)
    *rebuilt = true;

  return true;
}

} // namespace ForwardViewTopographyTexture

#endif
