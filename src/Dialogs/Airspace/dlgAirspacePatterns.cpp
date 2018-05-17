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

#include "Airspace.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Language/Language.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Features.hpp"
#include "Form/List.hpp"
#include "Look/AirspaceLook.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

/* brush patterns are only available on GDI */
#ifdef HAVE_HATCHED_BRUSH

class AirspacePatternsDialog : public ListItemRenderer {
  const AirspaceLook &look;

public:
  AirspacePatternsDialog(const AirspaceLook &_look)
    :look(_look) {}

  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned i) override {
    assert(i < ARRAY_SIZE(AirspaceLook::brushes));

    const unsigned padding = Layout::GetTextPadding();

    canvas.SetBackgroundTransparent();
    canvas.Select(look.brushes[i]);
    canvas.SetTextColor(COLOR_BLACK);
    canvas.Rectangle(rc.left + padding, rc.top + padding,
                     rc.right - padding, rc.bottom - padding);
  }
};

int
dlgAirspacePatternsShowModal(const AirspaceLook &look)
{
  AirspacePatternsDialog dialog(look);

  return ListPicker(_("Select Pattern"),
                    ARRAY_SIZE(AirspaceLook::brushes), 0,
                    Layout::GetMaximumControlHeight(),
                    dialog);
}

#endif /* HAVE_HATCHED_BRUSH */
