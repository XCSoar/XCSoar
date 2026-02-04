// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

#include <tchar.h>
#include <vector>
#include <cstddef>
#include <optional>

class Canvas;

class QuickGuideLinkWindow : public PaintWindow {
public:
  explicit QuickGuideLinkWindow() noexcept;

protected:
  unsigned DrawLink(Canvas &canvas, std::size_t index, PixelRect rc,
                    const TCHAR *text) noexcept;

  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;

  virtual bool OnLinkActivated(std::size_t index) noexcept = 0;

  std::vector<PixelRect> link_rects;
  std::optional<std::size_t> focused_link;

private:
  /**
   * Try to focus the next link.
   * @return true if handled (link focused), false if parent should scroll
   */
  bool FocusNextLink() noexcept;

  /**
   * Try to focus the previous link.
   * @return true if handled (link focused), false if parent should scroll
   */
  bool FocusPreviousLink() noexcept;
  void ActivateFocusedLink() noexcept;
  void ScrollToFocusedLink() noexcept;
  bool FindFirstVisibleLink() noexcept;
  bool FindLastVisibleLink() noexcept;
};
