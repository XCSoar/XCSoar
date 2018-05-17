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

#include "ColorListDialog.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Language/Language.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Form/List.hpp"
#include "Look/AirspaceLook.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

static void
OnPaintListItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(i < ARRAY_SIZE(AirspaceLook::preset_colors));

  const Color color(AirspaceLook::preset_colors[i]);

  PixelRect rc2 = rc;
  rc2.Grow(-(int)Layout::GetTextPadding());

#ifdef USE_GDI
  canvas.DrawFilledRectangle(rc2, color);
  canvas.SelectHollowBrush();
#else
  Brush brush(color);
  canvas.Select(brush);
#endif

  canvas.SelectBlackPen();
  canvas.Rectangle(rc2.left, rc2.top, rc2.right, rc2.bottom);
}

bool
ShowColorListDialog(RGB8Color &color)
{
  unsigned default_index = 0;
  for (unsigned i = 1; i < ARRAY_SIZE(AirspaceLook::preset_colors); ++i)
    if (AirspaceLook::preset_colors[i] == color)
      default_index = i;

  /*
  auto item_renderer = MakeListItemRenderer([](Canvas &canvas,
                                               const PixelRect rc, unsigned i){
                                              OnPaintListItem(canvas, rc, i);
                                            });
  */

  FunctionListItemRenderer item_renderer(OnPaintListItem);

  int index = ListPicker(_("Select Color"),
                         ARRAY_SIZE(AirspaceLook::preset_colors), default_index,
                         Layout::GetMaximumControlHeight(), item_renderer);

  if (index < 0)
    return false;

  assert((unsigned)index < ARRAY_SIZE(AirspaceLook::preset_colors));

  color = AirspaceLook::preset_colors[index];
  return true;
}
