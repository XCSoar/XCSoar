// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LinkableWindow.hpp"
#include "util/MarkdownParser.hpp"

#include <memory>
#include <vector>

class Font;
struct WrappedText;
struct SegmentedLine;

/**
 * A window showing multi-line text with Markdown formatting.
 *
 * Features:
 * - Automatic word wrapping (using TextWrapper)
 * - Bold text: **bold** or __bold__
 * - Headings: # H1, ## H2, ### H3
 * - List items: - item or * item
 * - Markdown links: [display text](url)
 * - Raw URL detection: http://, https://, xcsoar://
 * - Keyboard and mouse navigation (inherited from LinkableWindow)
 *
 * Keyboard navigation:
 * - Up/Down: navigate between links or scroll text
 * - Enter: activate focused link
 *
 * Mouse:
 * - Click on link to activate
 *
 * Designed to be hosted in a VScrollWidget for scrolling support.
 */
class RichTextWindow : public LinkableWindow {
  const Font *font = nullptr;
  const Font *bold_font = nullptr;
  const Font *heading_font = nullptr;

  /** Parsed text with links and styles extracted */
  ParsedMarkdown parsed;

  /** Cached content height (0 = needs recalculation) */
  mutable unsigned cached_content_height = 0;

  /** Width used for cached height calculation */
  mutable unsigned cached_height_width = 0;

  /** Wrapped lines from TextWrapper (opaque pointer to avoid header include) */
  mutable std::unique_ptr<WrappedText> wrapped_text;

  /** Width used for wrapped_text calculation */
  mutable unsigned wrapped_text_width = 0;

  /** Lines with link segment information */
  mutable std::unique_ptr<std::vector<SegmentedLine>> segmented_lines;

  /** Width used for segmented_lines calculation */
  mutable unsigned segmented_lines_width = 0;

  /** Checkbox toggle states (non-zero = toggled from original) */
  mutable std::vector<uint8_t> checkbox_toggled;

  /** Checkbox hit rectangles for click detection */
  struct CheckboxRect {
    PixelRect rect;
    std::size_t style_index;  ///< Index into parsed.styles
  };
  mutable std::vector<CheckboxRect> checkbox_rects;

  /** Currently focused checkbox style_index (into parsed.styles), or nullopt */
  mutable std::optional<std::size_t> focused_checkbox_style;

private:
  /**
   * Ensure wrapped_text is populated for current width.
   */
  void EnsureWrappedText() const noexcept;

  /**
   * Ensure segmented_lines is populated (calls EnsureWrappedText first).
   */
  void EnsureSegmentedLines() const noexcept;

  /**
   * Get viewport information for culling.
   */
  void GetVisibleArea(int &visible_top, int &visible_bottom,
                      int &viewport_height) const noexcept;

public:
  RichTextWindow() noexcept;
  ~RichTextWindow() noexcept;

  void Create(ContainerWindow &parent, PixelRect rc,
              const WindowStyle style = WindowStyle{});

  /**
   * Set the fonts for rendering.
   *
   * @param _font Main text font
   * @param _bold_font Font for bold text (optional, falls back to main)
   * @param _heading_font Font for headings (optional, falls back to bold or main)
   */
  void SetFont(const Font &_font,
               const Font *_bold_font = nullptr,
               const Font *_heading_font = nullptr) noexcept {
    font = &_font;
    bold_font = _bold_font ? _bold_font : &_font;
    heading_font = _heading_font ? _heading_font : (bold_font ? bold_font : &_font);
  }

  [[gnu::pure]]
  const Font &GetFont() const noexcept {
    assert(font != nullptr);
    return *font;
  }

  [[gnu::pure]]
  const Font &GetBoldFont() const noexcept {
    return bold_font ? *bold_font : GetFont();
  }

  [[gnu::pure]]
  const Font &GetHeadingFont() const noexcept {
    return heading_font ? *heading_font : GetBoldFont();
  }

  /**
   * Set the text content.
   *
   * @param text The text to display
   * @param parse_markdown If true, Markdown formatting is parsed and
   *                       rendered (bold, headings, lists, links).
   *                       If false, text is displayed as plain text
   *                       (faster for large texts).
   */
  void SetText(const char *text, bool parse_markdown = true);

  /**
   * Get the total content height in pixels.
   * This accounts for text wrapping at the current width.
   * Used by VScrollWidget to set virtual height.
   */
  [[gnu::pure]]
  unsigned GetContentHeight() const noexcept;

  /**
   * Get the parsed links.
   */
  [[gnu::pure]]
  const std::vector<MarkdownLink> &GetLinks() const noexcept {
    return parsed.links;
  }

  /**
   * Check if a checkbox is currently checked (considering toggles).
   */
  [[gnu::pure]]
  bool IsCheckboxChecked(std::size_t style_index) const noexcept;

  /**
   * Toggle a checkbox state.
   */
  void ToggleCheckbox(std::size_t style_index) noexcept;

  /**
   * Find checkbox at screen position.
   * @return style index or SIZE_MAX if not found
   */
  [[gnu::pure]]
  std::size_t FindCheckboxAt(PixelPoint p) const noexcept;

  /**
   * Check if a checkbox is currently focused.
   */
  [[gnu::pure]]
  bool IsCheckboxFocused(std::size_t style_index) const noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;

  /**
   * Called when a link is activated (clicked or Enter pressed).
   * Override to handle custom URI schemes (e.g., xcsoar://).
   * Default implementation handles http:// and https:// only.
   * @return true if the link was handled
   */
  bool OnLinkActivated(std::size_t index) noexcept override;
};
