/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Airspace.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Language/Language.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Features.hpp"
#include "UIGlobals.hpp"
#include "Look/AirspaceLook.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

/* brush patterns are only available on GDI */
#ifdef HAVE_HATCHED_BRUSH

static const AirspaceLook *look;

static void
OnAirspacePatternsPaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  assert(i < ARRAY_SIZE(AirspaceLook::brushes));

  canvas.SetBackgroundTransparent();
  canvas.Select(look->brushes[i]);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.Rectangle(rc.left + Layout::FastScale(2),
                   rc.top + Layout::FastScale(2),
                   rc.right - Layout::FastScale(2),
                   rc.bottom - Layout::FastScale(2));
}

int
dlgAirspacePatternsShowModal(const AirspaceLook &_look)
{
  look = &_look;

  return ListPicker(UIGlobals::GetMainWindow(), _("Select Pattern"),
                    ARRAY_SIZE(AirspaceLook::brushes), 0, Layout::FastScale(18),
                    OnAirspacePatternsPaintListItem);
}

#endif /* HAVE_HATCHED_BRUSH */
