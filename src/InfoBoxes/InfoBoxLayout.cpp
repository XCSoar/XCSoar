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

#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Border.hpp"
#include "Util/Macros.hpp"

#include <stdio.h>

/**
 * The number of info boxes in each geometry.
 */
static constexpr unsigned char geometry_counts[] = {
  8, 8, 8, 8, 8, 8,
  9, 5, 12, 24, 12,
  12, 9, 8, 4, 4, 4, 4,
};

namespace InfoBoxLayout
{
  gcc_const
  static InfoBoxSettings::Geometry
  ValidateGeometry(InfoBoxSettings::Geometry geometry,
                   UPixelScalar width, UPixelScalar height);

  static void
  CalcInfoBoxSizes(Layout &layout,
                   PixelRect rc, InfoBoxSettings::Geometry geometry);
}

static int
MakeTopRow(const InfoBoxLayout::Layout &layout,
           PixelRect *p, unsigned n, PixelScalar left, PixelScalar top)
{
  PixelRect *const end = p + n;
  const PixelScalar bottom = top + layout.control_height;
  while (p < end) {
    p->left = left;
    left += layout.control_width;
    p->right = left;
    p->top = top;
    p->bottom = bottom;

    ++p;
  }

  return bottom;
}

static int
MakeBottomRow(const InfoBoxLayout::Layout &layout,
              PixelRect *p, unsigned n, PixelScalar left, PixelScalar bottom)
{
  PixelScalar top = bottom - layout.control_height;
  MakeTopRow(layout, p, n, left, top);
  return top;
}

static int
MakeLeftColumn(const InfoBoxLayout::Layout &layout,
               PixelRect *p, unsigned n, PixelScalar left, PixelScalar top)
{
  PixelRect *const end = p + n;
  const PixelScalar right = left + layout.control_width;
  while (p < end) {
    p->left = left;
    p->right = right;
    p->top = top;
    top += layout.control_height;
    p->bottom = top;

    ++p;
  }

  return right;
}

static int
MakeRightColumn(const InfoBoxLayout::Layout &layout,
                PixelRect *p, unsigned n, PixelScalar right, PixelScalar top)
{
  PixelScalar left = right - layout.control_width;
  MakeLeftColumn(layout, p, n, left, top);
  return left;
}

InfoBoxLayout::Layout
InfoBoxLayout::Calculate(PixelRect rc, InfoBoxSettings::Geometry geometry)
{
  geometry = ValidateGeometry(geometry, rc.right - rc.left,
                              rc.bottom - rc.top);

  Layout layout;

  layout.geometry = geometry;
  layout.count = geometry_counts[(unsigned)geometry];
  assert(layout.count <= InfoBoxSettings::Panel::MAX_CONTENTS);

  CalcInfoBoxSizes(layout, rc, geometry);

  layout.ClearVario();

  switch (geometry) {
  case InfoBoxSettings::Geometry::TOP_4_BOTTOM_4:
    rc.top = MakeTopRow(layout, layout.positions, 4, rc.left, rc.top);
    rc.bottom = MakeBottomRow(layout, layout.positions + 4, 4,
                              rc.left, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
    layout.vario.left = rc.right - layout.control_width;
    layout.vario.right = rc.right;
    layout.vario.top = rc.bottom - layout.control_height * 2;
    layout.vario.bottom = rc.bottom;

    /* fall through */

  case InfoBoxSettings::Geometry::BOTTOM_8:
    rc.bottom = MakeBottomRow(layout, layout.positions + 4, 4,
                              rc.left, rc.bottom);
    rc.bottom = MakeBottomRow(layout, layout.positions, 4,
                              rc.left, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::TOP_8:
    rc.top = MakeTopRow(layout, layout.positions, 4, rc.left, rc.top);
    rc.top = MakeTopRow(layout, layout.positions + 4, 4, rc.left, rc.top);
    break;

  case InfoBoxSettings::Geometry::LEFT_4_RIGHT_4:
    rc.left = MakeLeftColumn(layout, layout.positions, 4, rc.left, rc.top);
    rc.right = MakeRightColumn(layout, layout.positions + 4, 4,
                               rc.right, rc.top);
    break;

  case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    layout.vario.left = rc.right - layout.control_width;
    layout.vario.right = rc.right;
    layout.vario.top = 0;
    layout.vario.bottom = layout.vario.top + layout.control_height * 3;

    rc.left = MakeLeftColumn(layout, layout.positions, 6, rc.left, rc.top);
    rc.right = MakeRightColumn(layout, layout.positions + 6, 3, rc.right,
                               rc.top + 3 * layout.control_height);
    break;

  case InfoBoxSettings::Geometry::LEFT_8:
    rc.left = MakeLeftColumn(layout, layout.positions, 4, rc.left, rc.top);
    rc.left = MakeLeftColumn(layout, layout.positions + 4, 4, rc.left, rc.top);
    break;

  case InfoBoxSettings::Geometry::RIGHT_8:
    rc.right = MakeRightColumn(layout, layout.positions + 4, 4,
                               rc.right, rc.top);
    rc.right = MakeRightColumn(layout, layout.positions, 4, rc.right, rc.top);
    break;

  case InfoBoxSettings::Geometry::RIGHT_12:
    rc.right = MakeRightColumn(layout, layout.positions + 6, 6,
                               rc.right, rc.top);
    rc.right = MakeRightColumn(layout, layout.positions, 6, rc.right, rc.top);
    break;

  case InfoBoxSettings::Geometry::BOTTOM_12:
    rc.bottom = MakeBottomRow(layout, layout.positions + 6, 6,
                              rc.left, rc.bottom);
    rc.bottom = MakeBottomRow(layout, layout.positions, 6,
                              rc.left, rc.bottom);
    break;

  case InfoBoxSettings::Geometry::TOP_12:
    rc.top = MakeTopRow(layout, layout.positions, 6, rc.left, rc.top);
    rc.top = MakeTopRow(layout, layout.positions + 6, 6, rc.left, rc.top);
    break;

  case InfoBoxSettings::Geometry::RIGHT_24:
    rc.right = MakeRightColumn(layout, layout.positions + 16, 8,
                               rc.right, rc.top);
    rc.right = MakeRightColumn(layout, layout.positions + 8, 8,
                               rc.right, rc.top);
    rc.right = MakeRightColumn(layout, layout.positions, 8, rc.right, rc.top);
    break;

  case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
    layout.vario.left = rc.right - layout.control_width;
    layout.vario.right = rc.right;
    layout.vario.top = 0;
    layout.vario.bottom = layout.vario.top + layout.control_height * 3;

    rc.right = MakeRightColumn(layout, layout.positions + 6, 3,
                               rc.right, rc.top + 3 * layout.control_height);
    rc.right = MakeRightColumn(layout, layout.positions, 6, rc.right, rc.top);
    break;

  case InfoBoxSettings::Geometry::RIGHT_5:
    rc.right = MakeRightColumn(layout, layout.positions, 5, rc.right, rc.top);
    break;
  case InfoBoxSettings::Geometry::TOP_4:
    rc.top = MakeTopRow(layout, layout.positions, 4, rc.left, rc.top);
    break;
  case InfoBoxSettings::Geometry::BOTTOM_4:
    rc.bottom = MakeBottomRow(layout, layout.positions, 4, rc.left, rc.bottom);
    break;
  case InfoBoxSettings::Geometry::LEFT_4:
    rc.left = MakeLeftColumn(layout, layout.positions, 4, rc.left, rc.top);
    break;
  case InfoBoxSettings::Geometry::RIGHT_4:
    rc.right = MakeRightColumn(layout, layout.positions, 4, rc.right, rc.top);
    break;
  };

  layout.remaining = rc;
  return layout;
}

static InfoBoxSettings::Geometry
InfoBoxLayout::ValidateGeometry(InfoBoxSettings::Geometry geometry,
                                UPixelScalar width, UPixelScalar height)
{
  if ((unsigned)geometry >= ARRAY_SIZE(geometry_counts))
    /* out of range */
    geometry = InfoBoxSettings::Geometry::TOP_4_BOTTOM_4;

  if (width > height) {
    /* landscape */

    switch (geometry) {
    case InfoBoxSettings::Geometry::TOP_4_BOTTOM_4:
      return InfoBoxSettings::Geometry::LEFT_4_RIGHT_4;

    case InfoBoxSettings::Geometry::BOTTOM_8:
      return InfoBoxSettings::Geometry::RIGHT_8;

    case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
      return InfoBoxSettings::Geometry::RIGHT_9_VARIO;

    case InfoBoxSettings::Geometry::TOP_8:
      return InfoBoxSettings::Geometry::LEFT_8;

    case InfoBoxSettings::Geometry::LEFT_4_RIGHT_4:
    case InfoBoxSettings::Geometry::LEFT_8:
    case InfoBoxSettings::Geometry::RIGHT_8:
    case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
    case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    case InfoBoxSettings::Geometry::LEFT_4:
    case InfoBoxSettings::Geometry::RIGHT_4:
      break;

    case InfoBoxSettings::Geometry::RIGHT_5:
    case InfoBoxSettings::Geometry::RIGHT_12:
    case InfoBoxSettings::Geometry::RIGHT_24:
      break;

    case InfoBoxSettings::Geometry::BOTTOM_12:
    case InfoBoxSettings::Geometry::TOP_12:
      return InfoBoxSettings::Geometry::RIGHT_12;

    case InfoBoxSettings::Geometry::BOTTOM_4:
    case InfoBoxSettings::Geometry::TOP_4:
      return InfoBoxSettings::Geometry::RIGHT_4;
    }
  } else if (width == height) {
    /* square */
    geometry = InfoBoxSettings::Geometry::RIGHT_5;
  } else {
    /* portrait */

    switch (geometry) {
    case InfoBoxSettings::Geometry::TOP_4_BOTTOM_4:
    case InfoBoxSettings::Geometry::BOTTOM_8:
    case InfoBoxSettings::Geometry::BOTTOM_4:
    case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
    case InfoBoxSettings::Geometry::BOTTOM_12:
    case InfoBoxSettings::Geometry::TOP_8:
    case InfoBoxSettings::Geometry::TOP_4:
    case InfoBoxSettings::Geometry::TOP_12:
      break;

    case InfoBoxSettings::Geometry::LEFT_4_RIGHT_4:
      return InfoBoxSettings::Geometry::TOP_4_BOTTOM_4;

    case InfoBoxSettings::Geometry::LEFT_8:
      return InfoBoxSettings::Geometry::TOP_8;

    case InfoBoxSettings::Geometry::LEFT_4:
       return InfoBoxSettings::Geometry::TOP_4;

    case InfoBoxSettings::Geometry::RIGHT_8:
    case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
    case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    case InfoBoxSettings::Geometry::RIGHT_5:
      return InfoBoxSettings::Geometry::BOTTOM_8;

    case InfoBoxSettings::Geometry::RIGHT_12:
    case InfoBoxSettings::Geometry::RIGHT_24:
      return InfoBoxSettings::Geometry::BOTTOM_12;

    case InfoBoxSettings::Geometry::RIGHT_4:
      return InfoBoxSettings::Geometry::BOTTOM_4;
    }
  }

  return geometry;
}

void
InfoBoxLayout::CalcInfoBoxSizes(Layout &layout, PixelRect rc,
                                InfoBoxSettings::Geometry geometry)
{
  switch (geometry) {
  case InfoBoxSettings::Geometry::TOP_4_BOTTOM_4:
  case InfoBoxSettings::Geometry::BOTTOM_8:
  case InfoBoxSettings::Geometry::BOTTOM_12:
  case InfoBoxSettings::Geometry::TOP_8:
  case InfoBoxSettings::Geometry::TOP_12:
    // calculate control dimensions
    layout.control_width = 2 * (rc.right - rc.left) / layout.count;
    layout.control_height = (rc.bottom - rc.top) / CONTROLHEIGHTRATIO;
    break;

  case InfoBoxSettings::Geometry::TOP_4:
  case InfoBoxSettings::Geometry::BOTTOM_4:
    // calculate control dimensions
    layout.control_width = (rc.right - rc.left) / layout.count;
    layout.control_height = (rc.bottom - rc.top) / CONTROLHEIGHTRATIO;
    break;

  case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
    // calculate control dimensions
    layout.control_width = 2 * (rc.right - rc.left) / (layout.count + 2);
    layout.control_height = (rc.bottom - rc.top) / CONTROLHEIGHTRATIO;
    break;

  case InfoBoxSettings::Geometry::LEFT_4_RIGHT_4:
  case InfoBoxSettings::Geometry::LEFT_8:
  case InfoBoxSettings::Geometry::RIGHT_8:
    // calculate control dimensions
    layout.control_width = (rc.right - rc.left) / CONTROLHEIGHTRATIO * 1.3;
    layout.control_height = 2 * (rc.bottom - rc.top) / layout.count;
    break;

  case InfoBoxSettings::Geometry::LEFT_4:
  case InfoBoxSettings::Geometry::RIGHT_4:
    // calculate control dimensions
    layout.control_width = (rc.right - rc.left) / CONTROLHEIGHTRATIO * 1.3;
    layout.control_height = (rc.bottom - rc.top) / layout.count;
    break;

  case InfoBoxSettings::Geometry::RIGHT_9_VARIO:
  case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
  case InfoBoxSettings::Geometry::RIGHT_12:
    // calculate control dimensions
    layout.control_height = (rc.bottom - rc.top) / 6;
    // preserve relative shape
    layout.control_width = layout.control_height * 1.44;
    break;

  case InfoBoxSettings::Geometry::RIGHT_5:
    // calculate control dimensions
    layout.control_width = (rc.right - rc.left) * 0.2;
    layout.control_height = (rc.bottom - rc.top) / 5;
    break;

  case InfoBoxSettings::Geometry::RIGHT_24:
    layout.control_height = (rc.bottom - rc.top) / 8;
    layout.control_width = layout.control_height * 1.44;
    break;
  }
}

int
InfoBoxLayout::GetBorder(InfoBoxSettings::Geometry geometry, unsigned i)
{
  unsigned border = 0;

  switch (geometry) {
  case InfoBoxSettings::Geometry::TOP_4_BOTTOM_4:
    if (i < 4)
      border |= BORDERBOTTOM;
    else
      border |= BORDERTOP;

    if (i != 3 && i != 7)
      border |= BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::BOTTOM_8:
  case InfoBoxSettings::Geometry::BOTTOM_8_VARIO:
  case InfoBoxSettings::Geometry::BOTTOM_4:
    border |= BORDERTOP;

    if (i != 3 && i != 7)
      border |= BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::BOTTOM_12:
    border |= BORDERTOP;

    if (i != 5 && i != 11)
      border |= BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::TOP_8:
  case InfoBoxSettings::Geometry::TOP_4:
    border |= BORDERBOTTOM;

    if (i != 3 && i != 7)
      border |= BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::TOP_12:
    border |= BORDERBOTTOM;

    if (i != 5 && i != 11)
      border |= BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::LEFT_4_RIGHT_4:
    if (i != 3 && i != 7)
      border |= BORDERBOTTOM;

    if (i < 4)
      border |= BORDERRIGHT;
    else
      border |= BORDERLEFT;
    break;

  case InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO:
    if ((i != 0) && (i != 6))
      border |= BORDERTOP;
    if (i < 6)
      border |= BORDERRIGHT;
    else
      border |= BORDERLEFT;
    break;

  case InfoBoxSettings::Geometry::LEFT_8:
  case InfoBoxSettings::Geometry::LEFT_4:
    if (i != 3 && i != 7)
      border |= BORDERBOTTOM;

    border |= BORDERRIGHT;
    break;

  case InfoBoxSettings::Geometry::RIGHT_8:
  case InfoBoxSettings::Geometry::RIGHT_4:
    if (i != 3 && i != 7)
      border |= BORDERBOTTOM;

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

  case InfoBoxSettings::Geometry::RIGHT_12:
    if (i % 6 != 0)
      border |= BORDERTOP;
    border |= BORDERLEFT;
    break;

  case InfoBoxSettings::Geometry::RIGHT_24:
    if (i % 8 != 0)
      border |= BORDERTOP;
    border |= BORDERLEFT;
    break;
  }

  return border;
}
