// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

#include <tchar.h>
#include <vector>
#include <cstddef>

class Canvas;

class QuickGuideLinkWindow : public PaintWindow {
public:
  explicit QuickGuideLinkWindow() noexcept;

protected:
  unsigned DrawLink(Canvas &canvas, std::size_t index, PixelRect rc,
                    const TCHAR *text) noexcept;

  bool OnMouseUp(PixelPoint p) noexcept override;

  virtual bool OnLinkActivated(std::size_t index) noexcept = 0;
  std::vector<PixelRect> link_rects;
};
