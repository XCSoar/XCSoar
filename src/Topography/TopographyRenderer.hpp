/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef TOPOGRAPHY_RENDERER_HPP
#define TOPOGRAPHY_RENDERER_HPP

#include "Topography/TopographyStore.hpp"
#include "Util/StaticArray.hxx"

class Canvas;
class WindowProjection;
class LabelBlock;
class TopographyFileRenderer;
struct TopographyLook;

/**
 * Class used to manage and render vector topography layers
 */
class TopographyRenderer : private NonCopyable {
  const TopographyStore &store;
  StaticArray<TopographyFileRenderer *, TopographyStore::MAXTOPOGRAPHY> files;

public:
  TopographyRenderer(const TopographyStore &store, const TopographyLook &look);

  TopographyRenderer(const TopographyRenderer &) = delete;

  ~TopographyRenderer();

  const TopographyStore &GetStore() const {
    return store;
  }

  /**
   * Draws the topography to the given canvas
   * @param canvas The drawing canvas
   * @param rc The area to draw in
   */
  void Draw(Canvas &canvas, const WindowProjection &projection) const;

  void DrawLabels(Canvas &canvas, const WindowProjection &projection,
                  LabelBlock &label_block) const;
};

#endif
