/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_RASTER_RENDERER_HPP
#define XCSOAR_RASTER_RENDERER_HPP

#include "Terrain/HeightMatrix.hpp"
#include "Screen/RawBitmap.hpp"
#include "Util/NonCopyable.hpp"

#define NUM_COLOR_RAMP_LEVELS 13

class Canvas;
class RasterMap;
class WindowProjection;
struct COLORRAMP;

class RasterRenderer : private NonCopyable {
  /** screen dimensions in coarse pixels */
  unsigned quantisation_pixels;

  /** step size used for slope calculations */
  unsigned quantisation_effective;

  HeightMatrix height_matrix;
  RawBitmap *image;

  fixed pixel_size;

  BGRColor color_table[256 * 128];

public:
  RasterRenderer();
  ~RasterRenderer();

  unsigned GetQuantisation() const {
    return quantisation_pixels;
  }

  const HeightMatrix &GetHeightMatrix() const {
    return height_matrix;
  }

  unsigned get_width() const {
    return height_matrix.get_width();
  }

  unsigned get_height() const {
    return height_matrix.get_height();
  }

  /**
   * Generate the color table.
   */
  void ColorTable(const COLORRAMP *color_ramp, bool do_water,
                  unsigned height_scale, int interp_levels);

  /**
   * Scan the map and fill the height matrix.
   */
  void ScanMap(const RasterMap &map, const WindowProjection &projection);

  /**
   * Convert the height matrix into the image.
   */
  void GenerateImage(bool is_terrain, bool do_shading,
                     unsigned height_scale, int contrast, int brightness,
                     const Angle sunazimuth, const Angle sunelevation);

  const RawBitmap &GetImage() const {
    return *image;
  }

protected:
  /**
   * Convert the height matrix into the image, without shading.
   */
  void GenerateUnshadedImage(bool is_terrain, unsigned height_scale);

  /**
   * Convert the height matrix into the image, with slope shading.
   */
  void GenerateSlopeImage(bool is_terrain, unsigned height_scale, int contrast,
                          const int sx, const int sy, const int sz);

  /**
   * Convert the height matrix into the image, with slope shading.
   */
  void GenerateSlopeImage(bool is_terrain, unsigned height_scale,
                          int contrast, int brightness,
                          const Angle sunazimuth, const Angle sunelevation);
};

#endif
