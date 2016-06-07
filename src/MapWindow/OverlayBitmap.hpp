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

#ifndef XCSOAR_MAP_OVERLAY_BITMAP_HPP
#define XCSOAR_MAP_OVERLAY_BITMAP_HPP

#include "Overlay.hpp"
#include "Screen/Bitmap.hpp"
#include "Geo/Quadrilateral.hpp"
#include "Geo/GeoBounds.hpp"
#include "Util/tstring.hpp"

#include <stdexcept>

class Canvas;
class WindowProjection;

/**
 * A georeferenced bitmap that can be rendered in the #MapWindow.
 *
 * @see MapWindow:SetOverlay()
 */
class MapOverlayBitmap final : public MapOverlay {
  Bitmap bitmap;

  /**
   * The geo reference for #bitmap.
   */
  GeoQuadrilateral bounds;

  /**
   * The smallest rectangle that contains #bounds, for fast visibility
   * checking in Draw().
   */
  GeoBounds simple_bounds;

  bool use_bitmap_alpha = true;

  float alpha = 1;

  tstring label;

public:
  /**
   * Load a GeoTIFF file.
   */
  MapOverlayBitmap(Path path) throw(std::runtime_error);

  /**
   * Move an existing #Bitmap with a geo reference.
   */
  MapOverlayBitmap(Bitmap &&_bitmap, GeoQuadrilateral _bounds,
                   tstring::const_pointer _label) noexcept
    :bitmap(std::move(_bitmap)), bounds(_bounds),
     simple_bounds(bounds.GetBounds()),
     label(_label) {}

  template<typename T>
  void SetLabel(T &&_label) {
    label = std::forward<T>(_label);
  }

  /**
   * By default, this class uses the bitmap's alpha channel.  This
   * method disables the alpha channel.
   */
  void IgnoreBitmapAlpha() {
    use_bitmap_alpha = false;
  }

  /**
   * Apply a constant alpha value.
   */
  void SetAlpha(float _alpha) {
    alpha = _alpha;
  }

  /* virtual methods from class MapOverlay */
  const TCHAR *GetLabel() const override {
    return label.c_str();
  }

  bool IsInside(GeoPoint p) const override;
  void Draw(Canvas &canvas,
            const WindowProjection &projection) noexcept override;
};

#endif
