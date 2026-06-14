// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EmptyDownloadList.hpp"

#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"

void
DrawEmptyDownloadHint(TextRowRenderer &renderer, Canvas &canvas,
                      PixelRect rc) noexcept
{
  renderer.DrawTextRow(canvas, rc, _("Press here to download"));
}

unsigned
LayoutEmptyDownloadRow(TextRowRenderer &renderer) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  return renderer.CalculateLayout(*look.list.font);
}

void
DrawEmptyDownloadHint(TwoTextRowsRenderer &renderer, Canvas &canvas,
                      PixelRect rc) noexcept
{
  renderer.DrawFirstRow(canvas, rc, _("None"));
  renderer.DrawSecondRow(canvas, rc, _("Press here to download"));
}

unsigned
LayoutEmptyDownloadRow(TwoTextRowsRenderer &renderer) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  return renderer.CalculateLayout(*look.list.font, look.small_font);
}
