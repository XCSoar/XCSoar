// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ColorListDialog.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Language/Language.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/control/List.hpp"
#include "Screen/Layout.hpp"
#include "Look/AirspaceLook.hpp"
#include "util/Macros.hpp"

#include <cassert>

static void
OnPaintListItem(Canvas &canvas, const PixelRect rc, unsigned i) noexcept
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
  canvas.DrawRectangle(rc2);
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
