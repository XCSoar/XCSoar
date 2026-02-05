// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "Renderer/TextRenderer.hpp"
#include "util/tstring.hpp"

#include <vector>
#include <tchar.h>

class Font;

/**
 * A parsed link within the text.
 */
struct RichTextLink {
  /** Character offset in the text where link starts */
  std::size_t start;

  /** Character offset in the text where link ends */
  std::size_t end;

  /** The URL (what to open when activated) */
  tstring url;

  /** Screen rectangle of the link (updated during paint) */
  PixelRect rect{};
};

/**
 * A window showing multi-line text with clickable links.
 *
 * Links are automatically detected:
 * - http:// and https:// URLs open in browser
 * - xcsoar:// URLs trigger internal actions
 *
 * Keyboard navigation:
 * - Up/Down: scroll text
 * - Tab/Shift+Tab: focus next/previous link
 * - Enter: activate focused link
 *
 * Mouse:
 * - Click on link to activate
 */
class RichTextWindow : public PaintWindow {
  const Font *font = nullptr;

  tstring text;

  /** Parsed links in the text */
  std::vector<RichTextLink> links;

  /** Currently focused link index, or -1 if none */
  int focused_link = -1;

  /** First visible line (scroll offset) */
  unsigned origin = 0;

  TextRenderer renderer;

public:
  void Create(ContainerWindow &parent, PixelRect rc,
              const WindowStyle style = WindowStyle{});

  void SetFont(const Font &_font) noexcept {
    font = &_font;
  }

  [[gnu::pure]]
  const Font &GetFont() const noexcept {
    assert(font != nullptr);
    return *font;
  }

  /**
   * Set the text content. Links are automatically parsed.
   */
  void SetText(const TCHAR *text) noexcept;

  [[gnu::pure]]
  unsigned GetVisibleRows() const noexcept;

  [[gnu::pure]]
  unsigned GetRowCount() const noexcept;

  void ScrollVertically(int delta_lines) noexcept;
  void ScrollTo(unsigned new_origin) noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;

  /**
   * Called when a link is activated (clicked or Enter pressed).
   * Override to handle custom URI schemes.
   * Default implementation handles http://, https://, and xcsoar://.
   * @return true if the link was handled
   */
  virtual bool OnLinkActivated(const TCHAR *url) noexcept;

private:
  /** Parse text and extract links */
  void ParseLinks() noexcept;

  /** Focus the next link (returns true if focus changed) */
  bool FocusNextLink() noexcept;

  /** Focus the previous link (returns true if focus changed) */
  bool FocusPreviousLink() noexcept;

  /** Activate the currently focused link */
  void ActivateFocusedLink() noexcept;

  /** Find link at screen position, or -1 if none */
  [[gnu::pure]]
  int FindLinkAt(PixelPoint p) const noexcept;

  /** Ensure focused link is visible by scrolling if needed */
  void ScrollToFocusedLink() noexcept;
};
