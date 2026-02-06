// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RichTextWindow.hpp"
#include "ui/canvas/TextWrapper.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/SubCanvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/event/KeyCode.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"
#include "Asset.hpp"
#include "system/OpenLink.hpp"
#include "util/UriSchemes.hpp"
#include "util/Macros.hpp"

#include <algorithm>
#include <cstdint>

/**
 * A segment of text within a wrapped line.
 * May be regular text, a link, or styled text (bold, heading, etc.).
 */
struct TextSegment {
  std::size_t start;
  std::size_t length;
  std::size_t link_index;
  TextStyle style;

  [[gnu::pure]]
  bool IsLink() const noexcept { return link_index != SIZE_MAX; }

  [[gnu::pure]]
  bool IsBold() const noexcept { return style == TextStyle::Bold; }

  [[gnu::pure]]
  bool IsHeading() const noexcept {
    return style == TextStyle::Heading1 ||
           style == TextStyle::Heading2 ||
           style == TextStyle::Heading3;
  }

  [[gnu::pure]]
  bool IsListItem() const noexcept { return style == TextStyle::ListItem; }

  [[gnu::pure]]
  bool IsCheckbox() const noexcept {
    return style == TextStyle::Checkbox ||
           style == TextStyle::CheckboxChecked;
  }

  [[gnu::pure]]
  bool IsCheckboxChecked() const noexcept {
    return style == TextStyle::CheckboxChecked;
  }
};

/**
 * A single wrapped line containing one or more segments.
 */
struct SegmentedLine {
  std::vector<TextSegment> segments;
};

/**
 * Draw a simple checkbox (box with optional checkmark).
 * @param focused If true, draw a focus ring around the checkbox
 */
static void
DrawSimpleCheckbox(Canvas &canvas, PixelRect box_rc, bool checked,
                   bool focused) noexcept
{
  const unsigned pen_width = Layout::ScalePenWidth(1);
  const unsigned focus_width = Layout::ScalePenWidth(2);

  // Draw focus ring if focused
  if (focused) {
    PixelRect focus_rc = box_rc;
    focus_rc.Grow(focus_width);
    canvas.Select(Pen(focus_width, COLOR_XCSOAR_LIGHT));
    canvas.SelectHollowBrush();
    canvas.DrawRectangle(focus_rc);
  }

  // Draw box outline
  canvas.Select(Pen(pen_width, COLOR_DARK_GRAY));
  canvas.SelectHollowBrush();
  canvas.DrawRectangle(box_rc);

  if (checked) {
    // Draw checkmark
    const int cx = (box_rc.left + box_rc.right) / 2;
    const int cy = (box_rc.top + box_rc.bottom) / 2;
    const int size = (box_rc.right - box_rc.left) / 2;

    canvas.Select(Pen(focus_width, COLOR_XCSOAR_DARK));
    // Draw checkmark as two lines
    canvas.DrawLine({cx - size + 1, cy}, {cx - size/3, cy + size - 1});
    canvas.DrawLine({cx - size/3, cy + size - 1},
                     {cx + size - 1, cy - size + 2});
  }
}

RichTextWindow::RichTextWindow() noexcept = default;
RichTextWindow::~RichTextWindow() noexcept = default;

void
RichTextWindow::Create(ContainerWindow &parent, PixelRect rc,
                       const WindowStyle style)
{
  PaintWindow::Create(parent, rc, style);
}

void
RichTextWindow::SetText(const char *text, bool parse_markdown)
{
  if (text != nullptr) {
    if (parse_markdown)
      parsed = ParseMarkdown(text);
    else {
      parsed = ParsedMarkdown{};
      parsed.text = text;
    }
  } else {
    parsed = ParsedMarkdown{};
  }

  ClearLinkRects();
  ClearLinkFocus();

  // Initialize checkbox toggle states (all 0 = use original state)
  checkbox_toggled.assign(parsed.styles.size(), 0);
  checkbox_rects.clear();
  focused_checkbox_style.reset();

  // Invalidate all caches
  cached_content_height = 0;
  cached_height_width = 0;
  wrapped_text.reset();
  wrapped_text_width = 0;
  segmented_lines.reset();
  segmented_lines_width = 0;

  Invalidate();
}

void
RichTextWindow::GetVisibleArea(int &visible_top, int &visible_bottom,
                               int &viewport_height) const noexcept
{
  const PixelPoint top_left = GetPosition().GetTopLeft();

  viewport_height = 2000;
  if (ContainerWindow *parent = GetParent())
    viewport_height = parent->GetSize().height;

  if (top_left.y < 0) {
    visible_top = -top_left.y;
    visible_bottom = visible_top + viewport_height;
  } else {
    visible_top = 0;
    visible_bottom = viewport_height;
  }
}

void
RichTextWindow::EnsureWrappedText() const noexcept
{
  if (font == nullptr || parsed.text.empty())
    return;

  const int padding = Layout::GetTextPadding();
  const auto size = GetSize();
  if (size.width == 0)
    return;

  const unsigned text_width = size.width - padding * 2;

  if (wrapped_text && wrapped_text_width == text_width)
    return;

  AnyCanvas canvas;
  canvas.Select(*font);
  wrapped_text = std::make_unique<WrappedText>(
    WrapText(canvas, text_width, parsed.text));
  wrapped_text_width = text_width;
}

/**
 * Find the style at a given position.
 */
static TextStyle
GetStyleAt(const std::vector<StyledSpan> &styles, std::size_t pos) noexcept
{
  for (const auto &span : styles) {
    if (pos >= span.start && pos < span.end)
      return span.style;
  }
  return TextStyle::Normal;
}

/**
 * Find the next boundary (link or style change) after pos.
 */
static std::size_t
FindNextBoundary(const std::vector<MarkdownLink> &links,
                 const std::vector<StyledSpan> &styles,
                 std::size_t pos, std::size_t line_end) noexcept
{
  std::size_t next = line_end;

  // Check link boundaries
  for (const auto &link : links) {
    if (link.start > pos && link.start < next)
      next = link.start;
    if (link.end > pos && link.end < next)
      next = link.end;
  }

  // Check style boundaries
  for (const auto &span : styles) {
    if (span.start > pos && span.start < next)
      next = span.start;
    if (span.end > pos && span.end < next)
      next = span.end;
  }

  return next;
}

void
RichTextWindow::EnsureSegmentedLines() const noexcept
{
  EnsureWrappedText();

  if (!wrapped_text || wrapped_text->lines.empty())
    return;

  const int padding = Layout::GetTextPadding();
  const auto size = GetSize();
  if (size.width == 0)
    return;

  const unsigned text_width = size.width - padding * 2;

  if (segmented_lines && segmented_lines_width == text_width)
    return;

  segmented_lines = std::make_unique<std::vector<SegmentedLine>>();
  segmented_lines->reserve(wrapped_text->lines.size());

  // If no links and no styles, each line is a single normal segment
  if (parsed.links.empty() && parsed.styles.empty()) {
    for (const auto &line : wrapped_text->lines) {
      SegmentedLine seg_line;
      if (line.length > 0)
        seg_line.segments.push_back({line.start, line.length, SIZE_MAX, TextStyle::Normal});
      segmented_lines->push_back(std::move(seg_line));
    }
    segmented_lines_width = text_width;
    return;
  }

  // Segment each line by link and style boundaries
  for (const auto &line : wrapped_text->lines) {
    SegmentedLine seg_line;
    std::size_t pos = line.start;
    const std::size_t line_end = line.start + line.length;

    while (pos < line_end) {
      // Find link at current position
      std::size_t link_idx = SIZE_MAX;
      for (std::size_t i = 0; i < parsed.links.size(); ++i) {
        if (pos >= parsed.links[i].start && pos < parsed.links[i].end) {
          link_idx = i;
          break;
        }
      }

      // Find style at current position
      TextStyle style = GetStyleAt(parsed.styles, pos);

      // Find next boundary (end of current link/style or start of next)
      std::size_t seg_end = line_end;
      if (link_idx != SIZE_MAX)
        seg_end = std::min(parsed.links[link_idx].end, seg_end);

      seg_end = std::min(seg_end, FindNextBoundary(parsed.links, parsed.styles, pos, line_end));

      if (seg_end > pos)
        seg_line.segments.push_back({pos, seg_end - pos, link_idx, style});
      pos = seg_end;
    }

    segmented_lines->push_back(std::move(seg_line));
  }

  segmented_lines_width = text_width;
}

unsigned
RichTextWindow::GetContentHeight() const noexcept
{
  if (font == nullptr || parsed.text.empty())
    return 0;

  const int padding = Layout::GetTextPadding();
  const auto size = GetSize();
  if (size.width == 0)
    return 0;

  const unsigned text_width = size.width - padding * 2;

  if (cached_content_height > 0 && cached_height_width == text_width)
    return cached_content_height;

  EnsureWrappedText();

  if (!wrapped_text)
    return 0;

  const unsigned content_height =
    wrapped_text->GetHeight(font->GetLineSpacing()) + padding * 2;

  cached_content_height = content_height;
  cached_height_width = text_width;

  return content_height;
}

void
RichTextWindow::OnResize(PixelSize new_size) noexcept
{
  PaintWindow::OnResize(new_size);

  cached_content_height = 0;
  cached_height_width = 0;
  wrapped_text.reset();
  wrapped_text_width = 0;
  segmented_lines.reset();
  segmented_lines_width = 0;

  if (!parsed.text.empty())
    Invalidate();
}

void
RichTextWindow::OnSetFocus() noexcept
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
RichTextWindow::OnKillFocus() noexcept
{
  PaintWindow::OnKillFocus();
  ClearLinkFocus();
  Invalidate();
}

void
RichTextWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.ClearWhite();

  if (parsed.text.empty() || font == nullptr)
    return;

  const int padding = Layout::GetTextPadding();
  const int line_height = font->GetLineSpacing();

  int visible_top, visible_bottom, viewport_height;
  GetVisibleArea(visible_top, visible_bottom, viewport_height);

  EnsureSegmentedLines();

  if (!segmented_lines || segmented_lines->empty())
    return;

  const int first_line = std::max(0, (visible_top - padding) / line_height);
  const int last_line = std::min(static_cast<int>(segmented_lines->size()),
                                 (visible_bottom + line_height) / line_height);

  const auto rc = canvas.GetRect();
  const PixelSize sub_size{rc.GetWidth(), static_cast<unsigned>(viewport_height)};
  SubCanvas sub_canvas(canvas, PixelPoint{0, visible_top}, sub_size);
  sub_canvas.SetBackgroundTransparent();

  ClearLinkRects();
  checkbox_rects.clear();

  const char *text_data = parsed.text.c_str();
  int y = padding + first_line * line_height - visible_top;

  // Indentation for list items and checkboxes (roughly 2 spaces)
  const int list_indent = font->TextSize("  ").width;

  for (int i = first_line; i < last_line; ++i) {
    if (i < 0 || static_cast<std::size_t>(i) >= segmented_lines->size())
      continue;

    const SegmentedLine &line = (*segmented_lines)[i];

    // Check if line starts with a list item or checkbox and apply indent
    int x = padding;
    if (!line.segments.empty()) {
      const TextSegment &first_seg = line.segments.front();
      if (first_seg.IsCheckbox() || first_seg.IsListItem())
        x += list_indent;
    }

    for (const TextSegment &seg : line.segments) {
      tstring_view seg_text(text_data + seg.start, seg.length);

      // Select font based on style
      const Font *seg_font = font;
      if (seg.IsHeading() && heading_font)
        seg_font = heading_font;
      else if (seg.IsBold() && bold_font)
        seg_font = bold_font;

      sub_canvas.Select(*seg_font);

      if (seg.IsLink()) {
        const bool is_focused = IsLinkFocused(seg.link_index);
        sub_canvas.SetTextColor(is_focused ? COLOR_XCSOAR_LIGHT : COLOR_XCSOAR);

        const PixelSize text_size = sub_canvas.CalcTextSize(seg_text);
        const int seg_width = static_cast<int>(text_size.width);
        sub_canvas.DrawText({x, y}, seg_text);

        const int underline_offset = Layout::ScalePenWidth(2);
        const int underline_y = y + line_height - underline_offset;
        sub_canvas.DrawLine({x, underline_y}, {x + seg_width, underline_y});

        PixelRect link_rect{x, y + visible_top,
                            x + seg_width, y + visible_top + line_height};
        RegisterLinkRect(seg.link_index, link_rect);

        if (is_focused && HasFocus()) {
          const int focus_pad = Layout::ScalePenWidth(2);
          PixelRect focus_rc{x - focus_pad, y - focus_pad,
                             x + seg_width + focus_pad, y + line_height + focus_pad};
          sub_canvas.DrawFocusRectangle(focus_rc);
        }

        x += seg_width;
      } else if (seg.IsHeading()) {
        // Headings in dark blue
        sub_canvas.SetTextColor(COLOR_XCSOAR_DARK);
        const PixelSize text_size = sub_canvas.CalcTextSize(seg_text);
        sub_canvas.DrawText({x, y}, seg_text);
        x += static_cast<int>(text_size.width);
      } else if (seg.IsBold()) {
        // Bold text stays black, add spacing around it
        sub_canvas.SetTextColor(COLOR_BLACK);
        const int bold_spacing = sub_canvas.CalcTextSize(" ").width / 2;
        x += bold_spacing;  // Space before bold
        const PixelSize text_size = sub_canvas.CalcTextSize(seg_text);
        sub_canvas.DrawText({x, y}, seg_text);
        x += static_cast<int>(text_size.width);
        x += bold_spacing;  // Space after bold
      } else if (seg.IsCheckbox()) {
        // Find style index for this checkbox
        std::size_t style_idx = SIZE_MAX;
        for (std::size_t si = 0; si < parsed.styles.size(); ++si) {
          const auto &span = parsed.styles[si];
          if (seg.start >= span.start && seg.start < span.end &&
              (span.style == TextStyle::Checkbox ||
               span.style == TextStyle::CheckboxChecked)) {
            style_idx = si;
            break;
          }
        }

        // Draw checkbox graphic (larger on touch screens)
        const int box_margin = Layout::ScalePenWidth(2);
        const int box_size = HasTouchScreen()
          ? std::max(line_height - box_margin, static_cast<int>(Layout::GetMinimumControlHeight()) / 2)
          : line_height - box_margin * 2;
        const int box_y = y + (line_height - box_size) / 2;
        PixelRect box_rc{x, box_y, x + box_size, box_y + box_size};

        bool checked = (style_idx != SIZE_MAX)
          ? IsCheckboxChecked(style_idx)
          : seg.IsCheckboxChecked();
        bool focused = (style_idx != SIZE_MAX) && IsCheckboxFocused(style_idx);
        DrawSimpleCheckbox(sub_canvas, box_rc, checked, focused);

        // Register for click detection - match the visual checkbox size
        if (style_idx != SIZE_MAX) {
          PixelRect click_rc{x, box_y + visible_top,
                             x + box_size, box_y + box_size + visible_top};
          checkbox_rects.push_back({click_rc, style_idx});
        }

        const int box_gap = Layout::Scale(4);
        x += box_size + box_gap;  // Box plus small gap
      } else if (seg.IsListItem()) {
        // List bullet in dark gray with a space after
        sub_canvas.SetTextColor(COLOR_DARK_GRAY);
        const PixelSize text_size = sub_canvas.CalcTextSize(seg_text);
        sub_canvas.DrawText({x, y}, seg_text);
        x += static_cast<int>(text_size.width);
        x += sub_canvas.CalcTextSize(" ").width;  // Add space after bullet
      } else {
        // Normal text
        sub_canvas.SetTextColor(COLOR_BLACK);
        const PixelSize text_size = sub_canvas.CalcTextSize(seg_text);
        sub_canvas.DrawText({x, y}, seg_text);
        x += static_cast<int>(text_size.width);
      }
    }

    y += line_height;
  }
}

bool
RichTextWindow::OnLinkActivated(std::size_t index) noexcept
{
  if (index >= parsed.links.size())
    return false;

  const char *url = parsed.links[index].url.c_str();

  if (IsExternalUriScheme(url))
    return OpenLink(url);

  return false;
}

bool
RichTextWindow::OnKeyCheck(unsigned key_code) const noexcept
{
  // Handle keys if we have checkboxes (links handled by base class)
  if (!checkbox_rects.empty()) {
    switch (key_code) {
    case KEY_UP:
    case KEY_DOWN:
      return true;
    case KEY_RETURN:
      // Only claim RETURN if a checkbox is focused
      if (focused_checkbox_style.has_value())
        return true;
      break;
    }
  }

  return LinkableWindow::OnKeyCheck(key_code);
}

bool
RichTextWindow::IsCheckboxChecked(std::size_t style_index) const noexcept
{
  if (style_index >= parsed.styles.size())
    return false;

  const auto &style = parsed.styles[style_index];
  bool original_checked = (style.style == TextStyle::CheckboxChecked);

  // XOR with toggle state
  if (style_index < checkbox_toggled.size() && checkbox_toggled[style_index])
    return !original_checked;

  return original_checked;
}

void
RichTextWindow::ToggleCheckbox(std::size_t style_index) noexcept
{
  if (style_index >= checkbox_toggled.size())
    return;

  checkbox_toggled[style_index] = !checkbox_toggled[style_index];
  Invalidate();
}

std::size_t
RichTextWindow::FindCheckboxAt(PixelPoint p) const noexcept
{
  for (const auto &cb : checkbox_rects) {
    if (cb.rect.Contains(p))
      return cb.style_index;
  }
  return SIZE_MAX;
}

bool
RichTextWindow::IsCheckboxFocused(std::size_t style_index) const noexcept
{
  return focused_checkbox_style.has_value() &&
         focused_checkbox_style.value() == style_index;
}

bool
RichTextWindow::OnKeyDown(unsigned key_code) noexcept
{
  // Build sorted list of focusable items (checkboxes and links by Y position)
  struct FocusItem {
    int y_pos;
    PixelRect rect;
    bool is_checkbox;
    std::size_t index;  // style_index for checkboxes, link index for links

    bool operator<(const FocusItem &other) const noexcept {
      if (y_pos != other.y_pos)
        return y_pos < other.y_pos;
      // Checkboxes before links at same position
      return is_checkbox && !other.is_checkbox;
    }
  };

  std::vector<FocusItem> items;
  items.reserve(checkbox_rects.size() + link_segments.size());

  for (const auto &cb : checkbox_rects)
    items.push_back({cb.rect.top, cb.rect, true, cb.style_index});

  for (const auto &seg : link_segments)
    items.push_back({seg.top, seg.rect, false, seg.link_index});

  std::sort(items.begin(), items.end());

  if (items.empty())
    return LinkableWindow::OnKeyDown(key_code);

  // Find current focus position in sorted list
  std::optional<std::size_t> current_pos;
  for (std::size_t i = 0; i < items.size(); ++i) {
    const auto &item = items[i];
    if (item.is_checkbox && focused_checkbox_style.has_value() &&
        focused_checkbox_style.value() == item.index) {
      current_pos = i;
      break;
    }
    if (!item.is_checkbox && focused_link.has_value() &&
        focused_link.value() == item.index) {
      current_pos = i;
      break;
    }
  }

  auto SetFocusToItem = [this](const FocusItem &item) {
    if (item.is_checkbox) {
      focused_checkbox_style = item.index;
      focused_link.reset();
      // Scroll to focused checkbox
      ContainerWindow *parent = GetParent();
      if (parent != nullptr) {
        const PixelRect &cb_rc = item.rect;
        const PixelRect window_rect = GetPosition();
        const int parent_height = parent->GetSize().height;
        const int padding = Layout::Scale(20);

        const int cb_top = cb_rc.top + window_rect.top;
        const int cb_bottom = cb_rc.bottom + window_rect.top;

        if (cb_top < padding || cb_bottom > parent_height - padding) {
          PixelRect scroll_rc;
          scroll_rc.left = 0;
          scroll_rc.right = 1;
          if (cb_top < padding) {
            scroll_rc.top = cb_rc.top - padding;
            scroll_rc.bottom = scroll_rc.top + 1;
          } else {
            scroll_rc.top = cb_rc.bottom + padding;
            scroll_rc.bottom = scroll_rc.top + 1;
          }
          parent->ScrollTo(scroll_rc);
        }
      }
    } else {
      focused_link = item.index;
      focused_checkbox_style.reset();
      ScrollToFocusedLink();
    }
    Invalidate();
  };

  switch (key_code) {
  case KEY_DOWN:
    if (!current_pos.has_value()) {
      // No focus - select first item
      SetFocusToItem(items.front());
      return true;
    } else if (current_pos.value() + 1 < items.size()) {
      // Move to next item
      SetFocusToItem(items[current_pos.value() + 1]);
      return true;
    } else {
      // At end - clear focus and let parent scroll
      focused_checkbox_style.reset();
      focused_link.reset();
      Invalidate();
      if (ContainerWindow *parent = GetParent())
        return parent->InjectKeyPress(key_code);
    }
    break;

  case KEY_UP:
    if (!current_pos.has_value()) {
      // No focus - select last item
      SetFocusToItem(items.back());
      return true;
    } else if (current_pos.value() > 0) {
      // Move to previous item
      SetFocusToItem(items[current_pos.value() - 1]);
      return true;
    } else {
      // At start - clear focus and let parent scroll
      focused_checkbox_style.reset();
      focused_link.reset();
      Invalidate();
      if (ContainerWindow *parent = GetParent())
        return parent->InjectKeyPress(key_code);
    }
    break;

  case KEY_RETURN:
    if (focused_checkbox_style.has_value()) {
      ToggleCheckbox(focused_checkbox_style.value());
      return true;
    }
    if (focused_link.has_value()) {
      ActivateFocusedLink();
      return true;
    }
    break;
  }

  return LinkableWindow::OnKeyDown(key_code);
}

bool
RichTextWindow::OnMouseUp(PixelPoint p) noexcept
{
  // Check for checkbox click first
  std::size_t cb_index = FindCheckboxAt(p);
  if (cb_index != SIZE_MAX) {
    ToggleCheckbox(cb_index);
    return true;
  }

  // Fall through to link handling
  return LinkableWindow::OnMouseUp(p);
}
