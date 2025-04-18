// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ListWidget.hpp"
#include "Renderer/TextRowRenderer.hpp"

/**
 * A combination of #ListWidget and #TextRowRenderer.
 */
class TextListWidget : public ListWidget {
protected:
  TextRowRenderer row_renderer;

  [[gnu::pure]]
  virtual const TCHAR *GetRowText(unsigned i) const noexcept = 0;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

protected:
  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, PixelRect rc,
                   unsigned i) noexcept override;
};
