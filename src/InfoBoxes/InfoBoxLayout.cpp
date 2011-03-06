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

#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Profile/Profile.hpp"
#include "Sizes.h"

#include <stdio.h>

/**
 * The number of info boxes in each geometry.
 */
static const unsigned char geometry_counts[] = {
  8, 8, 8, 8, 8, 8,
  9, 5, 12,
};

namespace InfoBoxLayout
{
  Geometry InfoBoxGeometry = ibTop4Bottom4;
  unsigned ControlWidth;
  unsigned ControlHeight;
  bool fullscreen = false;

  unsigned numInfoWindows = 8;
  RECT positions[InfoBoxPanelConfig::MAX_INFOBOXES];
  RECT remaining;

  gcc_const
  static Geometry
  ValidateGeometry(Geometry geometry, unsigned width, unsigned height);

  static void
  LoadGeometry(unsigned width, unsigned height);

  static void
  CalcInfoBoxSizes(RECT rc);
}

void
InfoBoxLayout::Init(RECT rc)
{
  LoadGeometry(rc.right - rc.left, rc.bottom - rc.top);

  numInfoWindows = geometry_counts[InfoBoxGeometry];
  assert(numInfoWindows <= InfoBoxPanelConfig::MAX_INFOBOXES);

  CalcInfoBoxSizes(rc);
}

static int
MakeTopRow(RECT *p, unsigned n, int left, int top)
{
  RECT *const end = p + n;
  const int bottom = top + InfoBoxLayout::ControlHeight;
  while (p < end) {
    p->left = left;
    left += InfoBoxLayout::ControlWidth;
    p->right = left;
    p->top = top;
    p->bottom = bottom;

    ++p;
  }

  return bottom;
}

static int
MakeBottomRow(RECT *p, unsigned n, int left, int bottom)
{
  int top = bottom - InfoBoxLayout::ControlHeight;
  MakeTopRow(p, n, left, top);
  return top;
}

static int
MakeLeftColumn(RECT *p, unsigned n, int left, int top)
{
  RECT *const end = p + n;
  const int right = left + InfoBoxLayout::ControlWidth;
  while (p < end) {
    p->left = left;
    p->right = right;
    p->top = top;
    top += InfoBoxLayout::ControlHeight;
    p->bottom = top;

    ++p;
  }

  return right;
}

static int
MakeRightColumn(RECT *p, unsigned n, int right, int top)
{
  int left = right - InfoBoxLayout::ControlWidth;
  MakeLeftColumn(p, n, left, top);
  return left;
}

InfoBoxLayout::Layout
InfoBoxLayout::Calculate(RECT rc, Geometry geometry)
{
  Layout layout;
  layout.count = numInfoWindows;

  switch (geometry) {
  case ibTop4Bottom4:
    layout.count = 8;
    rc.top = MakeTopRow(layout.positions, 4, rc.left, rc.top);
    rc.bottom = MakeBottomRow(layout.positions + 4, 4, rc.left, rc.bottom);
    break;

  case ibBottom8:
    assert(numInfoWindows == 8);

    rc.bottom = MakeBottomRow(layout.positions + 4, 4, rc.left, rc.bottom);
    rc.bottom = MakeBottomRow(layout.positions, 4, rc.left, rc.bottom);
    break;

  case ibTop8:
    assert(numInfoWindows == 8);

    rc.top = MakeTopRow(layout.positions, 4, rc.left, rc.top);
    rc.top = MakeTopRow(layout.positions + 4, 4, rc.left, rc.top);
    break;

  case ibLeft4Right4:
    assert(numInfoWindows == 8);

    rc.left = MakeLeftColumn(layout.positions, 4, rc.left, rc.top);
    rc.right = MakeRightColumn(layout.positions + 4, 4, rc.right, rc.top);
    break;

  case ibLeft8:
    assert(numInfoWindows == 8);

    rc.left = MakeLeftColumn(layout.positions, 4, rc.left, rc.top);
    rc.left = MakeLeftColumn(layout.positions + 4, 4, rc.left, rc.top);
    break;

  case ibRight8:
    assert(numInfoWindows == 8);

    rc.right = MakeRightColumn(layout.positions + 4, 4, rc.right, rc.top);
    rc.right = MakeRightColumn(layout.positions, 4, rc.right, rc.top);
    break;

  case ibRight12:
    assert(numInfoWindows == 12);

    rc.right = MakeRightColumn(layout.positions + 6, 6, rc.right, rc.top);
    rc.right = MakeRightColumn(layout.positions, 6, rc.right, rc.top);
    break;

  case ibGNav:
    assert(numInfoWindows == 9);

    rc.right = MakeRightColumn(layout.positions + 6, 3,
                               rc.right, rc.top + 3 * ControlHeight);
    rc.right = MakeRightColumn(layout.positions, 6, rc.right, rc.top);
    break;

  case ibSquare:
    assert(numInfoWindows == 5);

    rc.right = MakeRightColumn(layout.positions, 5, rc.right, rc.top);
    break;
  };

  layout.remaining = rc;
  return layout;
}

static InfoBoxLayout::Geometry
InfoBoxLayout::ValidateGeometry(InfoBoxLayout::Geometry geometry,
                                unsigned width, unsigned height)
{
  if ((unsigned)geometry >=
      sizeof(geometry_counts) / sizeof(geometry_counts[0]))
    /* out of range */
    geometry = ibTop4Bottom4;

  if (width > height) {
    /* landscape */
    if (geometry < ibLeft4Right4)
      geometry = ibGNav;
  } else if (width == height) {
    /* square */
    geometry = ibSquare;
  } else {
    /* portrait */
    if (geometry >= ibLeft4Right4)
      geometry = ibTop4Bottom4;
  }

  return geometry;
}

void
InfoBoxLayout::LoadGeometry(unsigned width, unsigned height)
{
  unsigned tmp;
  if (Profile::Get(szProfileInfoBoxGeometry, tmp))
    InfoBoxGeometry = (Geometry)tmp;

  InfoBoxGeometry = ValidateGeometry(InfoBoxGeometry, width, height);
}

void
InfoBoxLayout::CalcInfoBoxSizes(RECT rc)
{
  switch (InfoBoxGeometry) {
  case ibTop4Bottom4:
  case ibBottom8:
  case ibTop8:
    // calculate control dimensions
    ControlWidth = 2 * (rc.right - rc.left) / numInfoWindows;
    ControlHeight = (rc.bottom - rc.top) / CONTROLHEIGHTRATIO;
    break;

  case ibLeft4Right4:
  case ibLeft8:
  case ibRight8:
    // calculate control dimensions
    ControlWidth = (rc.right - rc.left) / CONTROLHEIGHTRATIO * 1.3;
    ControlHeight = 2 * (rc.bottom - rc.top) / numInfoWindows;
    break;

  case ibGNav:
  case ibRight12:
    // calculate control dimensions
    ControlHeight = (rc.bottom - rc.top) / 6;
    ControlWidth = ControlHeight * 1.44; // preserve relative shape
    break;

  case ibSquare:
    // calculate control dimensions
    ControlWidth = (rc.right - rc.left) * 0.2;
    ControlHeight = (rc.bottom - rc.top) / 5;
    break;
  }
}
