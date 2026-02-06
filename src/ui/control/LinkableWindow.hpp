// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

#include <vector>
#include <optional>
#include <cstddef>

/**
 * A segment of a link's screen rectangle.
 * Links that wrap across lines have multiple segments.
 */
struct LinkSegment {
  std::size_t link_index;  ///< The logical link index
  PixelRect rect;          ///< Screen rectangle for this segment
  int top;                 ///< Top coordinate for sorting
};

/**
 * Base class for windows that contain clickable links.
 *
 * Provides:
 * - Link rectangle tracking for hit-testing
 * - Keyboard focus navigation between links (Up/Down/Enter)
 * - Mouse click handling
 * - Scroll-to-link integration with parent scroll containers
 *
 * Subclasses must:
 * - Call RegisterLinkRect() during OnPaint() for each link segment
 * - Implement OnLinkActivated() to handle link clicks/activation
 */
class LinkableWindow : public PaintWindow {
protected:
  /** Screen rectangles for link segments (updated during paint) */
  std::vector<LinkSegment> link_segments;

  /** Number of unique links registered */
  std::size_t link_count = 0;

  /** Currently focused link index, or nullopt if none */
  std::optional<std::size_t> focused_link;

public:
  /**
   * Register a link segment's screen rectangle during painting.
   * Call this for each segment of each link to enable hit-testing and focus.
   * Links that wrap across lines should call this once per segment.
   *
   * @param index The link index
   * @param rect The screen rectangle of this segment
   */
  void RegisterLinkRect(std::size_t index, PixelRect rect) noexcept;

  /**
   * Clear all link segments. Call at start of OnPaint().
   */
  void ClearLinkRects() noexcept {
    link_segments.clear();
    link_count = 0;
  }

  /**
   * Get the number of unique links registered.
   */
  [[gnu::pure]]
  std::size_t GetLinkCount() const noexcept {
    return link_count;
  }

  /**
   * Get access to link segments for iteration.
   */
  [[gnu::pure]]
  const std::vector<LinkSegment> &GetLinkSegments() const noexcept {
    return link_segments;
  }

  /**
   * Get the focused link index, if any.
   */
  [[gnu::pure]]
  std::optional<std::size_t> GetFocusedLink() const noexcept {
    return focused_link;
  }

  /**
   * Check if the specified link is currently focused.
   */
  [[gnu::pure]]
  bool IsLinkFocused(std::size_t index) const noexcept {
    return focused_link.has_value() && focused_link.value() == index;
  }

  /**
   * Clear focus from any link.
   */
  void ClearLinkFocus() noexcept {
    focused_link.reset();
  }

protected:
  // PaintWindow overrides
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;

  /**
   * Called when a link is activated (clicked or Enter pressed).
   *
   * @param index The link index
   * @return true if handled
   */
  virtual bool OnLinkActivated(std::size_t index) noexcept = 0;

  /**
   * Activate the currently focused link.
   */
  void ActivateFocusedLink() noexcept;

  /**
   * Scroll the parent container to make the focused link visible.
   */
  void ScrollToFocusedLink() noexcept;

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

  /**
   * Find the first visible link and focus it.
   * @return true if a visible link was found and focused
   */
  bool FindFirstVisibleLink() noexcept;

  /**
   * Find the last visible link and focus it.
   * @return true if a visible link was found and focused
   */
  bool FindLastVisibleLink() noexcept;
};
