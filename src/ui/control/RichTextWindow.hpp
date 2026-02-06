// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LinkableWindow.hpp"
#include "util/MarkdownParser.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Color.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Font;
struct WrappedText;
struct SegmentedLine;
struct TextSegment;
struct FocusItem;

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
  const Font *heading1_font = nullptr;
  const Font *heading2_font = nullptr;

  /** Whether dark mode is active (affects text/background colors) */
  bool dark_mode = false;

  /** Background color (from DialogLook) */
  Color background_color = COLOR_WHITE;

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

  /**
   * Set when focus navigation reached the end (down) or start (up)
   * of all focusable items.  While set, the next DOWN/UP returns
   * false so the dialog can cycle focus to the next tab stop
   * (e.g. pager buttons).  Cleared when the window receives focus.
   */
  mutable bool focus_exhausted_down = false;
  mutable bool focus_exhausted_up = false;

  /** Cache of loaded bitmaps keyed by URL */
  mutable std::map<std::string, Bitmap> image_cache;

  /**
   * Per-line Y offsets and heights, accounting for block images
   * that are taller than a normal text line.
   */
  mutable std::vector<int> line_y_offsets;
  mutable std::vector<int> line_heights;
  mutable unsigned line_layout_width = 0;

private:
  /**
   * Load or retrieve a cached bitmap for the given image URL.
   * Supports "resource:IDB_NAME" for compiled-in resources.
   * @return pointer to the bitmap, or nullptr if not loadable
   */
  const Bitmap *LoadImage(const std::string &url) const noexcept;

  /**
   * Find the block image (if any) whose placeholder text falls
   * within [line_start, line_start+line_length).
   */
  [[gnu::pure]]
  const MarkdownImage *FindBlockImageForLine(
    std::size_t line_start,
    std::size_t line_length) const noexcept;

  /**
   * Find any image (block or inline) whose placeholder position
   * falls within [start, start+length).
   */
  [[gnu::pure]]
  const MarkdownImage *FindImageAtPosition(
    std::size_t start,
    std::size_t length) const noexcept;

  /**
   * Return the colour to use for a heading at the given text offset.
   * If a !!! admonition marker precedes the heading, the colour
   * is determined by the admonition type; otherwise the default
   * heading colour is returned.
   */
  [[gnu::pure]]
  Color GetAdmonitionColor(std::size_t heading_start) const noexcept;

  /**
   * Compute per-line Y offsets and heights, accounting for block
   * images that may be taller than a text line.
   */
  void EnsureLineLayout() const noexcept;
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

  /**
   * Reset all layout caches.  Called when text or size changes.
   */
  void InvalidateLayout() noexcept;

  /** Render an inline image for a segment, if present.
   * @return true if an image was rendered (caller should skip text) */
  bool RenderInlineImage(Canvas &canvas, const TextSegment &seg,
                         int &x, int y, int cur_line_height,
                         int text_line_height) const noexcept;

  /** Render an underlined link segment with hit-rect registration. */
  void RenderLinkSegment(Canvas &canvas, const TextSegment &seg,
                         const char *text_data, int &x, int text_y,
                         int visible_top,
                         int text_line_height) noexcept;

  /** Render a checkbox segment with hit-rect registration. */
  void RenderCheckboxSegment(Canvas &canvas, const TextSegment &seg,
                             int &x, int text_y, int visible_top,
                             int text_line_height) noexcept;

  /** Render a plain text segment (heading, bold, list item, normal). */
  void RenderPlainSegment(Canvas &canvas, const TextSegment &seg,
                          const char *text_data,
                          int &x, int text_y) const noexcept;

  /** Set focus to a FocusItem and scroll to make it visible. */
  void ScrollToFocusItem(const FocusItem &item,
                         int text_line_height) noexcept;

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
   * @param _heading1_font Font for H1 headings (optional, falls back to bold)
   * @param _heading2_font Font for H2 headings (optional, falls back to bold)
   */
  void SetFont(const Font &_font,
               const Font *_bold_font = nullptr,
               const Font *_heading1_font = nullptr,
               const Font *_heading2_font = nullptr) noexcept {
    font = &_font;
    bold_font = _bold_font ? _bold_font : &_font;
    heading1_font = _heading1_font ? _heading1_font : bold_font;
    heading2_font = _heading2_font ? _heading2_font : bold_font;
  }

  /**
   * Enable or disable dark mode rendering.
   *
   * @param _background_color The dialog background color from DialogLook
   */
  void SetDarkMode(bool _dark_mode,
                   Color _background_color = COLOR_WHITE) noexcept {
    dark_mode = _dark_mode;
    background_color = _background_color;
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

  /**
   * Return the appropriate font for a heading level.
   * H1 uses heading1_font, H2 uses heading2_font, H3 uses bold_font.
   */
  [[gnu::pure]]
  const Font &GetHeadingFont(TextStyle style) const noexcept {
    switch (style) {
    case TextStyle::Heading1:
      return heading1_font ? *heading1_font : GetBoldFont();
    case TextStyle::Heading2:
      return heading2_font ? *heading2_font : GetBoldFont();
    default:
      return GetBoldFont();
    }
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
   * Find the style index of the checkbox span containing text_pos.
   * @return index into parsed.styles, or SIZE_MAX if not found
   */
  [[gnu::pure]]
  std::size_t FindCheckboxStyleIndex(
    std::size_t text_pos) const noexcept;

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
