// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TransparentRendererCache.hpp"

#ifndef ENABLE_OPENGL
#include "Projection/WindowProjection.hpp"
#include "ui/canvas/Features.hpp"

bool
TransparentRendererCache::Check(const WindowProjection &projection) const
{
  assert(projection.IsValid());

  return buffer.IsDefined() &&
    buffer.GetSize() == projection.GetScreenSize() &&
    compare_projection.Compare(projection);
}

Canvas &
TransparentRendererCache::Begin(Canvas &canvas,
                                const WindowProjection &projection)
{
  assert(canvas.IsDefined());
  assert(projection.IsValid());

  const auto size = projection.GetScreenSize();
  if (buffer.IsDefined())
    buffer.Resize(size);
  else
    buffer.Create(canvas, size);

  compare_projection = CompareProjection(projection);
  return buffer;
}

void
TransparentRendererCache::Commit([[maybe_unused]] Canvas &canvas,
                                 [[maybe_unused]] const WindowProjection &projection)
{
  assert(canvas.IsDefined());
  assert(projection.IsValid());
  assert(compare_projection.IsDefined());
  assert(buffer.IsDefined());
  assert(Check(projection));

  empty = false;
}

void
TransparentRendererCache::CopyAndTo(Canvas &canvas,
                                    const WindowProjection &projection) const
{
  if (empty)
    return;

  canvas.CopyAnd({0, 0}, projection.GetScreenSize(), buffer, {0, 0});
}

void
TransparentRendererCache::CopyTransparentWhiteTo(Canvas &canvas,
                                                 const WindowProjection &projection) const
{
  if (empty)
    return;

  canvas.CopyTransparentWhite({0, 0},
                              projection.GetScreenSize(),
                              buffer, {0, 0});
}

#ifdef HAVE_ALPHA_BLEND

void
TransparentRendererCache::AlphaBlendTo(Canvas &canvas,
                                       const WindowProjection &projection,
                                       uint8_t alpha) const
{
  assert(canvas.IsDefined());
  assert(buffer.IsDefined());
  assert(projection.IsValid());
  assert(compare_projection.IsDefined());
  assert(Check(projection));

  if (empty)
    return;

  const auto screen_size = projection.GetScreenSize();

#ifdef USE_MEMORY_CANVAS
  canvas.AlphaBlendNotWhite({0, 0}, screen_size,
                            buffer, {0, 0}, screen_size,
                            alpha);
#else
  canvas.AlphaBlend({0, 0}, screen_size,
                    buffer, {0, 0}, screen_size,
                    alpha);
#endif
}

#endif

#endif
