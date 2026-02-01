// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TextRowRenderer.hpp"
#include "ui/control/List.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"

/**
 * A base class that combines TextRowRenderer with ListItemRenderer,
 * providing automatic OnListResized() handling. Subclasses only need
 * to implement OnPaintItem().
 */
class TextRowListItemRenderer : public ListItemRenderer {
protected:
  TextRowRenderer row_renderer;

public:
  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  unsigned OnListResized() noexcept override {
    const Font *font = UIGlobals::GetDialogLook().list.font;
    return font ? row_renderer.CalculateLayout(*font) : 0;
  }
};
