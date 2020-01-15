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

#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Border.hpp"
#include "Util/Macros.hpp"
#include "Util/Clamp.hpp"

/**
 * The number of info boxes in each geometry.
 */
static constexpr unsigned char geometry_counts[] = {
  8, 8, 8, 8, 8, 8,
  9, 5, 12, 24, 12,
  12, 9, 8, 4, 4, 4, 4,
  8, 16, 15,
};

namespace InfoBoxLayout
{
  gcc_const
  static InfoBoxSettings::Geometry
  ValidateGeometry(InfoBoxSettings::Geometry geometry, PixelSize screen_size);

  static void
  CalcInfoBoxSizes(Layout &layout, PixelSize screen_size,
                   InfoBoxSettings::Geometry geometry, unsigned ib_scale);
}

static int
MakeTopRow(const InfoBoxLayout::Layout &layout,
           PixelRect *p, unsigned n, int left, int right, int top)
{
  PixelRect *const end = p + n;
  const int bottom = top + layout.control_size.cy;
  while (p < end) {
    p->left = left;
    left += layout.control_size.cx;
    p->right = left;
    p->top = top;
    p->bottom = bottom;

    ++p;
  }

  /* assign remaining pixels to last InfoBox */
  p[-1].right = right;

  return bottom;
}

static int
MakeBottomRow(const InfoBoxLayout::Layout &layout,
              PixelRect *p, unsigned n, int left, int right, int bottom)
{
  int top = bottom - layout.control_size.cy;
  MakeTopRow(layout, p, n, left, right, top);
  return top;
}

static int
MakeLeftColumn(const InfoBoxLayout::Layout &layout,
               PixelRect *p, unsigned n, int left, int top, int bottom)
{
  PixelRect *const end = p + n;
  const int right = left + layout.control_size.cx;
  while (p < end) {
    p->left = left;
    p->right = right;
    p->top = top;
    top += layout.control_size.cy;
    p->bottom = top;

    ++p;
  }

  /* assign remaining pixels to last InfoBox */
  p[-1].bottom = bottom;

  return right;
}

static int
MakeRightColumn(const InfoBoxLayout::Layout &layout,
                PixelRect *p, unsigned n, int right, int top, int bottom)
{
  int left = right - layout.control_size.cx;
  MakeLeftColumn(layout, p, n, left, top, bottom);
  return left;
}

InfoBoxLayout::Layout
InfoBoxLayout::Calculate(PixelRect rc, InfoBoxSettings::Geometry geometry, unsigned ib_scale)
{
  const PixelSize screen_size = rc.GetSize();

  geometry = ValidateGeometry(geometry, screen_size);

  Layout layout;

  layout.geometry = geometry;
  layout.landscape = screen_size.cx > screen_size.cy;
  layout.count = geometry_counts[(unsigned)geometry];
  assert(layout.count <= InfoBoxSettings::Panel::MAX_CONTENTS);

  CalcInfoBoxSizes(layout, screen_size, geometry, ib_scale);

  layout.ClearVario();

  unsigned right = rc.right;

  switch (geometry) {
  case InfoBoxSettings::Geometry::SPLIT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_SPLIT_8:
    if (layout.landscape) {
      rc.left = MakeLeftColumn(layout, layout.positions, 4,
                               rc.left, rc.top, rc.bottom);
      rc.right = MakeRightColumn(layout, layout.positions + 4, 4,
                                 rc.right, rc.top, rc.bottom);
    } else {
      rc.top = MakeTopRow(layout, layout.positions, 4,
                          rc.left, rc.right, rc.top);
      rc.bottom = MakeBottomRow(layout, layout.positions + 4, 4,
                                rc.left, rc.right, rc.bottom);
    }

    break;

  case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
    layout.vario.left = rc.right - layout.control_size.cx;
    layout.vario.right = rc.right;
    layout.vario.top = rc.bottom - layout.control_size.cy * 2;
    layout.vario.bottom = rc.bottom;

    right = layout.vario.left;

    /* fall through */

  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_8:
    if (layout.landscape) {
      rc.right = MakeRightColumn(layout, layout.positions + 4, 4,
                                 rc.right, rc.top, rc.bottom);
      rc.right = MakeRightColumn(layout, layout.positions, 4,
                                 rc.right, rc.top, rc.bottom);
    } else {
      rc.bottom = MakeBottomRow(layout, layout.positions + 4, 4,
                                rc.left, right, rc.bottom);
      rc.bottom = MakeBottomRow(layout, layout.positions, 4,
                                rc.left, right, rc.bottom);
    }

    break;

  case InfoBoxSettings::Geometry::TOP_8_VARIO:
    layout.vario.left = rc.right - layout.control_size.cx;
    layout.vario.right = rc.right;
    layout.vario.top = rc.top;
    layout.vario.bottom = rc.top + layout.control_size.cy * 2;

    right = layout.vario.left;

    /* fall through */

  case InfoBoxSettings::Geometry::TOP_LEFT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_8:
    if (layout.landscape) {
      rc.left = MakeLeftColumn(layout, layout.positions, 4,
                               rc.left, rc.top, rc.bottom);
      rc.left = MakeLeftColumn(layout, layout.positions + 4, 4,
                               rc.left, rc.top, rc.bottom);
    } else {
      rc.top = MakeTopRow(layout, layout.positions, 4,
                          rc.left, right, rc.top);
      rc.top = MakeTopRow(layout, layout.positions + 4, 4,
                          rc.left, right, rc.top);
    }

    break;

  case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    layout.vario.left = rc.right - layout.control_size.cx;
    layout.vario.right = rc.right;
    layout.vario.top = 0;
    layout.vario.bottom = layout.vario.top + layout.control_size.cy * 3;

    rc.left = MakeLeftColumn(layout, layout.positions, 6,
                             rc.left, rc.top, rc.bottom);
    rc.right = MakeRightColumn(layout, layout.positions + 6, 3, rc.right,
                               rc.top + 3 * layout.control_size.cy, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::LEFT_12_RIGHT_3_VARIO:
    layout.vario.left = rc.right - layout.control_size.cx;
    layout.vario.right = rc.right;
    layout.vario.top = 0;
    layout.vario.bottom = layout.vario.top + layout.control_size.cy * 3;

    rc.right = MakeRightColumn(layout, layout.positions + 6, 3, rc.right,
                               rc.top + 3 * layout.control_size.cy, rc.bottom);

    layout.control_size.cx = layout.control_size.cy * 1.1;
    rc.left = MakeLeftColumn(layout, layout.positions, 6,
                             rc.left, rc.top, rc.bottom);
    rc.left = MakeLeftColumn(layout, layout.positions + 9, 6,
                             rc.left, rc.top, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_12:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_12:
    if (layout.landscape) {
      rc.right = MakeRightColumn(layout, layout.positions + 6, 6,
                                 rc.right, rc.top, rc.bottom);
      rc.right = MakeRightColumn(layout, layout.positions, 6,
                                 rc.right, rc.top, rc.bottom);
    } else {
      rc.bottom = MakeBottomRow(layout, layout.positions + 6, 6,
                                rc.left, rc.right, rc.bottom);
      rc.bottom = MakeBottomRow(layout, layout.positions, 6,
                                rc.left, rc.right, rc.bottom);
    }

    break;

  case InfoBoxSettings::Geometry::TOP_LEFT_12:
    if (layout.landscape) {
      rc.left = MakeLeftColumn(layout, layout.positions, 6,
                               rc.left, rc.top, rc.bottom);
      rc.left = MakeLeftColumn(layout, layout.positions + 6, 6,
                               rc.left, rc.top, rc.bottom);
    } else {
      rc.top = MakeTopRow(layout, layout.positions, 6,
                          rc.left, rc.right, rc.top);
      rc.top = MakeTopRow(layout, layout.positions + 6, 6,
                          rc.left, rc.right, rc.top);
    }
    break;

  case InfoBoxSettings::Geometry::RIGHT_16:
    rc.right = MakeRightColumn(layout, layout.positions + 8, 8,
                               rc.right, rc.top, rc.bottom);
    rc.right = MakeRightColumn(layout, layout.positions, 8,
                               rc.right, rc.top, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::RIGHT_24:
    rc.right = MakeRightColumn(layout, layout.positions + 16, 8,
                               rc.right, rc.top, rc.bottom);
    rc.right = MakeRightColumn(layout, layout.positions + 8, 8,
                               rc.right, rc.top, rc.bottom);
    rc.right = MakeRightColumn(layout, layout.positions, 8,
                               rc.right, rc.top, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
    layout.vario.left = rc.right - layout.control_size.cx;
    layout.vario.right = rc.right;
    layout.vario.top = 0;
    layout.vario.bottom = layout.vario.top + layout.control_size.cy * 3;

    rc.right = MakeRightColumn(layout, layout.positions + 6, 3,
                               rc.right,
                               rc.top + 3 * layout.control_size.cy, rc.bottom);
    rc.right = MakeRightColumn(layout, layout.positions, 6,
                               rc.right, rc.top, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::RIGHT_5:
    rc.right = MakeRightColumn(layout, layout.positions, 5,
                               rc.right, rc.top, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::TOP_LEFT_4:
  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_4:
    if (layout.landscape)
      rc.left = MakeLeftColumn(layout, layout.positions, 4,
                               rc.left, rc.top, rc.bottom);
    else
      rc.top = MakeTopRow(layout, layout.positions, 4,
                          rc.left, rc.right, rc.top);
    break;

  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_4:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_4:
    if (layout.landscape)
      rc.right = MakeRightColumn(layout, layout.positions, 4,
                                 rc.right, rc.top, rc.bottom);
    else
      rc.bottom = MakeBottomRow(layout, layout.positions, 4,
                                rc.left, rc.right, rc.bottom);
    break;
  };

  layout.remaining = rc;
  return layout;
}

static InfoBoxSettings::Geometry
InfoBoxLayout::ValidateGeometry(InfoBoxSettings::Geometry geometry,
                                PixelSize screen_size)
{
  if ((unsigned)geometry >= ARRAY_SIZE(geometry_counts))
    /* out of range */
    geometry = InfoBoxSettings::Geometry::SPLIT_8;

  if (screen_size.cx > screen_size.cy) {
    /* landscape */

    switch (geometry) {
    case InfoBoxSettings::Geometry::SPLIT_8:
    case InfoBoxSettings::Geometry::BOTTOM_RIGHT_8:
    case InfoBoxSettings::Geometry::TOP_LEFT_8:
    case InfoBoxSettings::Geometry::OBSOLETE_SPLIT_8:
    case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_8:
    case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_8:
    case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
    case InfoBoxSettings::Geometry::RIGHT_5:
    case InfoBoxSettings::Geometry::BOTTOM_RIGHT_12:
    case InfoBoxSettings::Geometry::RIGHT_16:
    case InfoBoxSettings::Geometry::RIGHT_24:
    case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_12:
    case InfoBoxSettings::Geometry::TOP_LEFT_12:
    case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    case InfoBoxSettings::Geometry::LEFT_12_RIGHT_3_VARIO:
      break;

    case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
      return InfoBoxSettings::Geometry::RIGHT_9_VARIO;

    case InfoBoxSettings::Geometry::TOP_LEFT_4:
    case InfoBoxSettings::Geometry::BOTTOM_RIGHT_4:
    case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_4:
    case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_4:
      break;

    case InfoBoxSettings::Geometry::TOP_8_VARIO:
      return InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO;
    }
#ifdef	HANDLE_SQUARE_SCREEN_SIZE // remove this code section if we are certain
  } else if (screen_size.cx == screen_size.cy) {
    /* square */
    // do we really need to cover the square case ??
    // it's probably not intended to fix it to RIGHT_5 !!!
    geometry = InfoBoxSettings::Geometry::RIGHT_5;
#endif
  } else {
    /* portrait */

    switch (geometry) {
    case InfoBoxSettings::Geometry::SPLIT_8:
    case InfoBoxSettings::Geometry::BOTTOM_RIGHT_8:
    case InfoBoxSettings::Geometry::TOP_LEFT_8:
    case InfoBoxSettings::Geometry::OBSOLETE_SPLIT_8:
    case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_8:
    case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_8:
      break;

    case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
      return InfoBoxSettings::Geometry::BOTTOM_8_VARIO;

    case InfoBoxSettings::Geometry::RIGHT_5:
    case InfoBoxSettings::Geometry::BOTTOM_RIGHT_12:
      break;

    case InfoBoxSettings::Geometry::RIGHT_16:
      return InfoBoxSettings::Geometry::BOTTOM_RIGHT_12;

    case InfoBoxSettings::Geometry::RIGHT_24:
      return InfoBoxSettings::Geometry::BOTTOM_RIGHT_12;

    case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_12:
    case InfoBoxSettings::Geometry::TOP_LEFT_12:
      break;

    case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
      return InfoBoxSettings::Geometry::BOTTOM_8_VARIO;

    case InfoBoxSettings::Geometry::LEFT_12_RIGHT_3_VARIO:
      return InfoBoxSettings::Geometry::BOTTOM_8_VARIO;

    case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
    case InfoBoxSettings::Geometry::TOP_LEFT_4:
    case InfoBoxSettings::Geometry::BOTTOM_RIGHT_4:
    case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_4:
    case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_4:
    case InfoBoxSettings::Geometry::TOP_8_VARIO:
      break;
    }
  }

  return geometry;
}

static unsigned
CalculateInfoBoxRowHeight(unsigned screen_height, unsigned control_width, unsigned scale)
{
  return Clamp(scale * control_width * 5 / 700,
    75 * control_width * 5 / 700,
    scale * control_width * 5 / 700);
}

static unsigned
CalculateInfoBoxColumnWidth(unsigned screen_width, unsigned control_height, unsigned scale)
{
  return (scale * control_height * 7 / 500);
}

void
InfoBoxLayout::CalcInfoBoxSizes(Layout &layout, PixelSize screen_size,
                                InfoBoxSettings::Geometry geometry, unsigned ib_scale)
{
  const bool landscape = screen_size.cx > screen_size.cy;

  switch (geometry) {
  case InfoBoxSettings::Geometry::SPLIT_8:
  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_8:
  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_12:
  case InfoBoxSettings::Geometry::TOP_LEFT_8:
  case InfoBoxSettings::Geometry::TOP_LEFT_12:
    if (landscape) {
      layout.control_size.cy = 2 * screen_size.cy / layout.count;
      layout.control_size.cx = CalculateInfoBoxColumnWidth(screen_size.cx,
						 layout.control_size.cy, ib_scale);
    } else {
      layout.control_size.cx = 2 * screen_size.cx / layout.count;
      layout.control_size.cy = CalculateInfoBoxRowHeight(screen_size.cy,
						 layout.control_size.cx, ib_scale);
    }

    break;

  case InfoBoxSettings::Geometry::TOP_LEFT_4:
  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_4:
    if (landscape) {
      layout.control_size.cy = screen_size.cy / layout.count;
      layout.control_size.cx = CalculateInfoBoxColumnWidth(screen_size.cx,
						   layout.control_size.cy, ib_scale);
    } else {
      layout.control_size.cx = screen_size.cx / layout.count;
      layout.control_size.cy = CalculateInfoBoxRowHeight(screen_size.cy,
						 layout.control_size.cx, ib_scale);
    }

    break;

  case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
    // calculate control dimensions
    layout.control_size.cx = 2 * screen_size.cx / (layout.count + 2);
    layout.control_size.cy = CalculateInfoBoxRowHeight(screen_size.cy,
					       layout.control_size.cx, ib_scale);
    break;

  case InfoBoxSettings::Geometry::TOP_8_VARIO:
    // calculate control dimensions
    layout.control_size.cx = 2 * screen_size.cx / (layout.count + 2);
    layout.control_size.cy = CalculateInfoBoxRowHeight(screen_size.cy,
					       layout.control_size.cx, ib_scale);
    break;

  case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
  case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    // calculate control dimensions
    layout.control_size.cy = screen_size.cy / 6;
    // preserve relative shape
    layout.control_size.cx = layout.control_size.cy * 1.44;
    break;

  case InfoBoxSettings::Geometry::LEFT_12_RIGHT_3_VARIO:
    // calculate control dimensions
    layout.control_size.cy = screen_size.cy / 6;
    // preserve relative shape
    layout.control_size.cx = layout.control_size.cy * 1.35;
    break;

  case InfoBoxSettings::Geometry::RIGHT_5:
    // calculate control dimensions
    layout.control_size.cx = screen_size.cx / 5;
    layout.control_size.cy = screen_size.cy / 5;
    break;

  case InfoBoxSettings::Geometry::RIGHT_16:
    layout.control_size.cy = screen_size.cy / 8;
    layout.control_size.cx = layout.control_size.cy * 1.44;
    break;

  case InfoBoxSettings::Geometry::RIGHT_24:
    layout.control_size.cy = screen_size.cy / 8;
    layout.control_size.cx = layout.control_size.cy * 1.44;
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_SPLIT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_4:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_4:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_12:
    gcc_unreachable();
  }
}

int
InfoBoxLayout::GetBorder(InfoBoxSettings::Geometry geometry, bool landscape,
                         unsigned i)
{
  unsigned border = 0;

  switch (geometry) {
  case InfoBoxSettings::Geometry::SPLIT_8:
    if (landscape) {
      if (i != 3 && i != 7)
        border |= BORDERBOTTOM;

      if (i < 4)
        border |= BORDERRIGHT;
      else
        border |= BORDERLEFT;
    } else {
      if (i < 4)
        border |= BORDERBOTTOM;
      else
        border |= BORDERTOP;

      if (i != 3 && i != 7)
        border |= BORDERRIGHT;
    }

    break;

  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_4:
  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_8:
    if (landscape) {
      if (i != 3 && i != 7)
        border |= BORDERBOTTOM;

      border |= BORDERLEFT;
    } else {
      border |= BORDERTOP;

      if (i != 3 && i != 7)
        border |= BORDERRIGHT;
    }

    break;

  case InfoBoxSettings::Geometry::BOTTOM_RIGHT_12:
    if (landscape) {
      if (i % 6 != 0)
        border |= BORDERTOP;
      border |= BORDERLEFT;
    } else {
      border |= BORDERTOP;

      if (i != 5 && i != 11)
        border |= BORDERRIGHT;
    }

    break;

  case InfoBoxSettings::Geometry::TOP_LEFT_12:
    if (landscape) {
      if (i % 6 != 0)
        border |= BORDERTOP;
      border |= BORDERRIGHT;
    } else {
      border |= BORDERBOTTOM;

      if (i != 5 && i != 11)
        border |= BORDERRIGHT;
    }

    break;

  case InfoBoxSettings::Geometry::TOP_LEFT_8:
  case InfoBoxSettings::Geometry::TOP_LEFT_4:
    if (landscape) {
      if (i != 3 && i != 7)
        border |= BORDERBOTTOM;

      border |= BORDERRIGHT;
    } else {
      border |= BORDERBOTTOM;

      if (i != 3 && i != 7)
        border |= BORDERRIGHT;
    }

    break;

  case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
    border |= BORDERTOP|BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::TOP_8_VARIO:
    border |= BORDERBOTTOM|BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    if (i != 0)
      border |= BORDERTOP;
    if (i < 6)
      border |= BORDERRIGHT;
    else
      border |= BORDERLEFT;
    break;

  case InfoBoxSettings::Geometry::LEFT_12_RIGHT_3_VARIO:
    if (!((i == 0) ||(i == 9)))
      border |= BORDERTOP;
    if (i < 12)
      border |= BORDERRIGHT;
    else
      border |= BORDERLEFT;
    break;

  case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
    if (i != 0)
      border |= BORDERTOP;
    if (i < 6)
      border |= BORDERLEFT|BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::RIGHT_5:
    border |= BORDERLEFT;
    if (i != 0)
      border |= BORDERTOP;
    break;

  case InfoBoxSettings::Geometry::RIGHT_16:
    if (i % 8 != 0)
      border |= BORDERTOP;
    border |= BORDERLEFT;
    break;

  case InfoBoxSettings::Geometry::RIGHT_24:
    if (i % 8 != 0)
      border |= BORDERTOP;
    border |= BORDERLEFT;
    break;

  case InfoBoxSettings::Geometry::OBSOLETE_SPLIT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_TOP_LEFT_4:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_8:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_4:
  case InfoBoxSettings::Geometry::OBSOLETE_BOTTOM_RIGHT_12:
    gcc_unreachable();
  }

  return border;
}
