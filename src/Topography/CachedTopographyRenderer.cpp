// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CachedTopographyRenderer.hpp"
#include "TopographyStore.hpp"

#ifndef ENABLE_OPENGL

void
CachedTopographyRenderer::Draw(Canvas &canvas,
                               const WindowProjection &projection) noexcept
{
  if (renderer.GetStore().GetSerial() != last_serial ||
      !cache.Check(projection)) {
    last_serial = renderer.GetStore().GetSerial();

    Canvas &buffer_canvas = cache.Begin(canvas, projection);
    buffer_canvas.ClearWhite();
    renderer.Draw(buffer_canvas, projection);
    cache.Commit(canvas, projection);
  }

  cache.CopyTransparentWhiteTo(canvas, projection);
}

#endif
