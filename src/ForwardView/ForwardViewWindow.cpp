// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ForwardViewWindow.hpp"

#ifdef ENABLE_OPENGL
#include "Look/TopographyLook.hpp"
#include "ui/canvas/opengl/Scissor.hpp"
#endif

ForwardViewWindow::ForwardViewWindow(const CrossSectionLook &look,
                                     const AirspaceLook &airspace_look,
                                     const TopographyLook &topography_look,
                                     const bool inverse) noexcept
  :renderer(look, airspace_look, topography_look, inverse) {}

void
ForwardViewWindow::ReadBlackboard(const MoreData &basic,
                                  const DerivedInfo &calculated,
                                  const GlideSettings &glide_settings,
                                  const GlidePolar &glide_polar,
                                  const MapSettings &map_settings,
                                  RoughTimeDelta utc_offset) noexcept
{
  renderer.ReadBlackboard(basic, calculated,
                          glide_settings, glide_polar,
                          map_settings, utc_offset);
  Invalidate();
}

void
ForwardViewWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

#ifdef ENABLE_OPENGL
  const GLCanvasScissor scissor(rc);
#endif

  renderer.Paint(canvas, rc);
}
