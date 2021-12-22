/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
