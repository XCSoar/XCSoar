// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyRenderer.hpp"
#include "Topography/TopographyFileRenderer.hpp"
#include "TopographyStore.hpp"
#include "TopographyFile.hpp"

TopographyRenderer::TopographyRenderer(const TopographyStore &_store,
                                       const TopographyLook &look) noexcept
  :store(_store)
{
  auto previous = files.before_begin();
  for (const auto &file : store)
    previous = files.emplace_after(previous, file, look);
}

TopographyRenderer::~TopographyRenderer() noexcept = default;

void
TopographyRenderer::Draw(Canvas &canvas,
                         const WindowProjection &projection) noexcept
{
  for (auto &i : files)
    i.Paint(canvas, projection);
}

void
TopographyRenderer::DrawLabels(Canvas &canvas,
                               const WindowProjection &projection,
                               LabelBlock &label_block) noexcept
{
  for (auto &i : files)
    i.PaintLabels(canvas, projection, label_block);
}
