// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapPreviewBufferWindow.hpp"
#include "ui/canvas/Canvas.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/WindowCanvas.hpp"
#endif

MapPreviewBufferWindow::MapPreviewBufferWindow(
  const AirspaceLook &airspace_look,
  const TopographyLook &topography_look) noexcept
  : layers(airspace_look, topography_look)
{
}

MapPreviewBufferWindow::~MapPreviewBufferWindow() noexcept = default;

void
MapPreviewBufferWindow::InitialiseAirspaceBuffer() noexcept
{
#ifndef ENABLE_OPENGL
  WindowCanvas canvas(*this);
  airspace_buffer.Create(canvas);
#endif
}

void
MapPreviewBufferWindow::DeinitialiseAirspaceBuffer() noexcept
{
#ifndef ENABLE_OPENGL
  airspace_buffer.Destroy();
#endif
}

void
MapPreviewBufferWindow::ResizePreviewProjection(PixelSize new_size) noexcept
{
#ifndef ENABLE_OPENGL
  airspace_buffer.Grow(new_size);
#endif

  projection.SetScreenSize(new_size);
  projection.SetScreenOrigin(PixelRect{new_size}.GetCenter());
  projection.UpdateScreenBounds();
}

void
MapPreviewBufferWindow::PaintTerrainAndTopography(Canvas &canvas) noexcept
{
  layers.RenderTerrain(canvas, projection);
  layers.RenderTopography(canvas, projection);
}

void
MapPreviewBufferWindow::PaintAirspace(Canvas &canvas,
                                      bool enabled) noexcept
{
  if (!enabled)
    return;

#ifndef ENABLE_OPENGL
  layers.RenderAirspace(canvas, airspace_buffer, projection);
#else
  layers.RenderAirspace(canvas, projection);
#endif
}
