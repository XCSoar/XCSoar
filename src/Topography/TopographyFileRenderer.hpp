/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef TOPOGRAPHY_FILE_RENDERER_HPP
#define TOPOGRAPHY_FILE_RENDERER_HPP

#include "Screen/Pen.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Icon.hpp"
#include "Util/NonCopyable.hpp"

#ifndef ENABLE_OPENGL
#include "Topography/ShapeRenderer.hpp"
#endif

class TopographyFile;
class Canvas;
class WindowProjection;
class LabelBlock;

/**
 * Class used to manage and render vector topography layers
 */
class TopographyFileRenderer : private NonCopyable {
  const TopographyFile &file;

#ifndef ENABLE_OPENGL
  mutable ShapeRenderer shape_renderer;
#endif

  Pen pen;
  Brush brush;

  MaskedIcon icon;

public:
  TopographyFileRenderer(const TopographyFile &file);

  /**
   * Paints the polygons, lines and points/icons in the TopographyFile
   * @param canvas The canvas to paint on
   * @param bitmap_canvas Temporary canvas for the icon
   * @param projection
   */
  void Paint(Canvas &canvas, const WindowProjection &projection) const;

  /**
   * Paints a topography label if the space is available in the LabelBlock
   * @param canvas The canvas to paint on
   * @param projection
   * @param label_block The LabelBlock class to use for decluttering
   * @param settings_map
   */
  void PaintLabels(Canvas &canvas,
                   const WindowProjection &projection, LabelBlock &label_block) const;
};

#endif
