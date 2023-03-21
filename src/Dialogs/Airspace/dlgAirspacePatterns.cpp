// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Language/Language.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "ui/control/List.hpp"
#include "Screen/Layout.hpp"
#include "Look/AirspaceLook.hpp"
#include "util/Macros.hpp"

#include <cassert>

/* brush patterns are only available on GDI */
#ifdef HAVE_HATCHED_BRUSH

class AirspacePatternsDialog : public ListItemRenderer {
  const AirspaceLook &look;

public:
  AirspacePatternsDialog(const AirspaceLook &_look)
    :look(_look) {}

  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned i) noexcept override {
    assert(i < ARRAY_SIZE(AirspaceLook::brushes));

    const unsigned padding = Layout::GetTextPadding();

    canvas.SetBackgroundTransparent();
    canvas.Select(look.brushes[i]);
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawRectangle(rc.WithPadding(padding));
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
