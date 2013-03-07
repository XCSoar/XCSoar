/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Terrain/TerrainRenderer.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Screen/Ramp.hpp"
#include "Projection/WindowProjection.hpp"
#include "Util/Macros.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Compatibility.hpp"
#endif

#include <assert.h>

static constexpr ColorRamp terrain_colors[][NUM_COLOR_RAMP_LEVELS] = {
  {
    {0,           0x70, 0xc0, 0xa7},
    {250,         0xca, 0xe7, 0xb9},
    {500,         0xf4, 0xea, 0xaf},
    {750,         0xdc, 0xb2, 0x82},
    {1000,        0xca, 0x8e, 0x72},
    {1250,        0xde, 0xc8, 0xbd},
    {1500,        0xe3, 0xe4, 0xe9},
    {1750,        0xdb, 0xd9, 0xef},
    {2000,        0xce, 0xcd, 0xf5},
    {2250,        0xc2, 0xc1, 0xfa},
    {2500,        0xb7, 0xb9, 0xff},
    {5000,        0xb7, 0xb9, 0xff},
    {6000,        0xb7, 0xb9, 0xff}
  },
  {
    {0,           0x70, 0xc0, 0xa7},
    {500,         0xca, 0xe7, 0xb9},
    {1000,        0xf4, 0xea, 0xaf},
    {1500,        0xdc, 0xb2, 0x82},
    {2000,        0xca, 0x8e, 0x72},
    {2500,        0xde, 0xc8, 0xbd},
    {3000,        0xe3, 0xe4, 0xe9},
    {3500,        0xdb, 0xd9, 0xef},
    {4000,        0xce, 0xcd, 0xf5},
    {4500,        0xc2, 0xc1, 0xfa},
    {5000,        0xb7, 0xb9, 0xff},
    {6000,        0xb7, 0xb9, 0xff},
    {7000,        0xb7, 0xb9, 0xff}
  },
  { // Imhof Type 7, geomteric 1.35 9
    {0,    153, 178, 169},
    {368,  180, 205, 181},
    {496,  225, 233, 192},
    {670,  255, 249, 196},
    {905,  255, 249, 196},
    {1222, 255, 219, 173},
    {1650, 254, 170, 136},
    {2227, 253, 107, 100},
    {3007, 255, 255, 255},
    {5000, 255, 255, 255},
    {6000, 255, 255, 255},
    {7000, 255, 255, 255},
    {8000, 255, 255, 255}
  },
  { // Imhof Type 4, geomteric 1.5 8
    {0,    175, 224, 203},
    {264,  211, 237, 211},
    {396,  254, 254, 234},
    {594,  252, 243, 210},
    {891,  237, 221, 195},
    {1336, 221, 199, 175},
    {2004, 215, 170, 148},
    {3007, 255, 255, 255},
    {4000, 255, 255, 255},
    {5000, 255, 255, 255},
    {6000, 255, 255, 255},
    {7000, 255, 255, 255},
    {8000, 255, 255, 255}
  },
  { // Imhof Type 12, geomteric  1.5 8
    {0,    165, 220, 201},
    {399,  219, 239, 212},
    {558,  254, 253, 230},
    {782,  254, 247, 211},
    {1094,  254, 237, 202},
    {1532, 254, 226, 207},
    {2145, 254, 209, 204},
    {3004, 255, 255, 255},
    {4000, 255, 255, 255},
    {5000, 255, 255, 255},
    {6000, 255, 255, 255},
    {7000, 255, 255, 255},
    {8000, 255, 255, 255}
  },
  { // Imhof Atlas der Schweiz
    {0,     47, 101, 147},
    {368,   58, 129, 152},
    {496,  117, 148, 153},
    {670,  155, 178, 140},
    {905,  192, 190, 139},
    {1222, 215, 199, 137},
    {1650, 229, 203, 171},
    {2227, 246, 206, 171},
    {3007, 252, 246, 244},
    {5001, 252, 246, 244},
    {7000, 252, 246, 244},
    {8000, 252, 246, 244},
    {9000, 252, 246, 244}
  },
  { // ICAO
    {0,           180, 205, 181},
    {199,         180, 205, 181},
    {200,         225, 233, 192},
    {499,         225, 233, 192},
    {500,         255, 249, 196},
    {999,         255, 249, 196},
    {1000,        255, 219, 173},
    {1499,        255, 219, 173},
    {1500,        254, 170, 136},
    {1999,        254, 170, 136},
    {2000,        253, 107, 100},
    {2499,        253, 107, 100},
    {2500,        255, 255, 255}
  },
  { // Grey
    {0,           220, 220, 220},
    {100,         220, 220, 220},
    {200,         220, 220, 220},
    {300,         220, 220, 220},
    {500,         220, 220, 220},
    {700,         220, 220, 220},
    {1000,        220, 220, 220},
    {1250,        220, 220, 220},
    {1500,        220, 220, 220},
    {1750,        220, 220, 220},
    {2000,        220, 220, 220},
    {2250,        220, 220, 220},
    {2500,        220, 220, 220}
  },
  { // White
    {0,           255, 255, 255},
    {100,         255, 255, 255},
    {200,         255, 255, 255},
    {300,         255, 255, 255},
    {500,         255, 255, 255},
    {700,         255, 255, 255},
    {1000,        255, 255, 255},
    {1250,        255, 255, 255},
    {1500,        255, 255, 255},
    {1750,        255, 255, 255},
    {2000,        255, 255, 255},
    {2250,        255, 255, 255},
    {2500,        255, 255, 255}
  },

  /**
   * "Gaudy".  Imitates SeeYouMobile's default ramp.
   */
  {
    {0,           0x00, 0x80, 0x00},
    {1000,        0xff, 0xff, 0x00},
    {1500,        0xc0, 0x20, 0x00},
    {2800,        0xff, 0xff, 0xff},
    {3000,        0xff, 0xff, 0xff},
    {3100,        0xff, 0xff, 0xff},
    {3200,        0xff, 0xff, 0xff},
    {3300,        0xff, 0xff, 0xff},
    {3400,        0xff, 0xff, 0xff},
    {3500,        0xff, 0xff, 0xff},
    {3600,        0xff, 0xff, 0xff},
    {3700,        0xff, 0xff, 0xff},
    {3800,        0xff, 0xff, 0xff},
  }
};
static_assert(ARRAY_SIZE(terrain_colors) == TerrainRendererSettings::NUM_RAMPS,
              "mismatched size");

// map scale is approximately 2 points on the grid
// therefore, want one to one mapping if mapscale is 0.5
// there are approx 30 pixels in mapscale
// 240/QUANTISATION_PIXELS resolution = 6 pixels per terrain
// (mapscale/30)  km/pixels
//        0.250   km/terrain
// (0.25*30/mapscale) pixels/terrain
//  mapscale/(0.25*30)
//  mapscale/7.5 terrain units/pixel
//
// this is for TerrainInfo.StepSize = 0.0025;
TerrainRenderer::TerrainRenderer(const RasterTerrain *_terrain)
  :terrain(_terrain),
   last_sun_azimuth(Angle::Zero()),
   last_color_ramp(NULL)
{
  assert(terrain != NULL);
  settings.SetDefaults();
}

void
TerrainRenderer::CopyTo(Canvas &canvas, unsigned width, unsigned height) const
{
  raster_renderer.GetImage().StretchTo(raster_renderer.GetWidth(),
                                        raster_renderer.GetHeight(), canvas,
                                        width, height);
}

void
TerrainRenderer::SetSettings(const TerrainRendererSettings &_settings)
{
  if (settings == _settings)
    return;

  settings = _settings;

#ifdef ENABLE_OPENGL
  raster_renderer.Invalidate();
#else
  compare_projection.Clear();
#endif
}

#ifdef ENABLE_OPENGL
/**
 * Checks if the size difference of any dimension is more than a
 * factor of two.  This is used to check whether the terrain has to be
 * redrawn after zooming in.
 */
static bool
IsLargeSizeDifference(const GeoBounds &a, const GeoBounds &b)
{
  assert(a.IsValid());
  assert(b.IsValid());

  return a.GetWidth().Native() > Double(b.GetWidth().Native()) ||
    a.GetHeight().Native() > Double(b.GetHeight().Native());
}
#endif

void
TerrainRenderer::Generate(const WindowProjection &map_projection,
                          const Angle sunazimuth)
{
#ifdef ENABLE_OPENGL
  const GeoBounds &old_bounds = raster_renderer.GetBounds();
  const GeoBounds &new_bounds = map_projection.GetScreenBounds();
  assert(new_bounds.IsValid());

  if (old_bounds.IsValid() && old_bounds.IsInside(new_bounds) &&
      !IsLargeSizeDifference(old_bounds, new_bounds) &&
      terrain_serial == terrain->GetSerial() &&
      sunazimuth.CompareRoughly(last_sun_azimuth) &&
      !raster_renderer.UpdateQuantisation())
    /* no change since previous frame */
    return;

#else
  if (compare_projection.Compare(map_projection) &&
      terrain_serial == terrain->GetSerial() &&
      sunazimuth.CompareRoughly(last_sun_azimuth))
    /* no change since previous frame */
    return;

  compare_projection = CompareProjection(map_projection);
#endif

  terrain_serial = terrain->GetSerial();

  last_sun_azimuth = sunazimuth;

  const bool do_water = true;
  const unsigned height_scale = 4;
  const int interp_levels = 2;
  const bool is_terrain = true;
  const bool do_shading = is_terrain &&
                          settings.slope_shading != SlopeShading::OFF;

  const ColorRamp *const color_ramp = &terrain_colors[settings.ramp][0];
  if (color_ramp != last_color_ramp) {
    raster_renderer.ColorTable(color_ramp, do_water,
                               height_scale, interp_levels);
    last_color_ramp = color_ramp;
  }

  {
    RasterTerrain::Lease map(*terrain);
    raster_renderer.ScanMap(map, map_projection);
  }

  raster_renderer.GenerateImage(do_shading, height_scale,
                                settings.contrast, settings.brightness,
                                sunazimuth);
}

/**
 * Draws the terrain to the given canvas
 * @param canvas The drawing canvas
 * @param map_projection The Projection
 * @param sunazimuth Azimuth of the sun (for terrain shading)
 */
void
TerrainRenderer::Draw(Canvas &canvas,
                      const WindowProjection &map_projection) const
{
#ifdef ENABLE_OPENGL
  const GeoBounds &bounds = raster_renderer.GetBounds();
  assert(bounds.IsValid());

  const RasterPoint vertices[] = {
    map_projection.GeoToScreen(bounds.GetNorthWest()),
    map_projection.GeoToScreen(bounds.GetNorthEast()),
    map_projection.GeoToScreen(bounds.GetSouthWest()),
    map_projection.GeoToScreen(bounds.GetSouthEast()),
  };

  glVertexPointer(2, GL_VALUE, 0, vertices);

  const GLTexture &texture = raster_renderer.BindAndGetTexture();
  const PixelSize allocated = texture.GetAllocatedSize();

  const int src_x = 0, src_y = 0, src_width = raster_renderer.GetWidth(),
    src_height = raster_renderer.GetHeight();

  GLfloat x0 = (GLfloat)src_x / allocated.cx;
  GLfloat y0 = (GLfloat)src_y / allocated.cy;
  GLfloat x1 = (GLfloat)(src_x + src_width) / allocated.cx;
  GLfloat y1 = (GLfloat)(src_y + src_height) / allocated.cy;

  const GLfloat coord[] = {
    x0, y0,
    x1, y0,
    x0, y1,
    x1, y1,
  };

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLEnable scope(GL_TEXTURE_2D);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, coord);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
  CopyTo(canvas, map_projection.GetScreenWidth(),
         map_projection.GetScreenHeight());
#endif
}
