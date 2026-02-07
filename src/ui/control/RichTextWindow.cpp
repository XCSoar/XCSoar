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
#include "ResourceLookup.hpp"
#include "system/OpenLink.hpp"
#include "util/StringCompare.hxx"
#include "util/UriSchemes.hpp"
#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif
#include <algorithm>
#include <cstdint>
#include <string_view>

/**
 * Safe proxy for "is this a touch device with larger controls".
 *
 * Cannot use HasTouchScreen() directly because on Wayland/libinput
 * it dereferences UI::event_queue which may not exist yet when
 * RichTextWindow is first measured during startup.
 *
 * Layout::GetMaximumControlHeight() is always initialised before any
 * UI code runs and is larger than the minimum only on touch devices.
 */
[[gnu::pure]]
static bool
IsTouchLayout() noexcept
{
  return Layout::GetMaximumControlHeight() >
         Layout::GetMinimumControlHeight();
}

/**
 * Content margin for the rich text area (top, left, right, bottom).
 * Larger than GetTextPadding() for comfortable reading.
 * Touch layouts get extra padding for easier interaction.
 * Includes extra right-side margin so link spacing (added during
 * rendering but not accounted for by the text wrapper) does not
 * push characters past the visible edge.
 */
static int
GetContentPadding() noexcept
{
  return Layout::Scale(IsTouchLayout() ? 14 : 10);
}

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

  /** True if this line is a wrapped continuation of a list-item
      paragraph (i.e. not the first line, but the paragraph starts
      with "- ").  Used to apply a hanging indent. */
  bool is_list_continuation = false;
};

/**
 * Draw a simple checkbox (box with optional checkmark).
 * @param focused If true, draw a focus ring around the checkbox
 */
static void
DrawSimpleCheckbox(Canvas &canvas, PixelRect box_rc, bool checked,
                   bool focused, bool dark_mode) noexcept
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
  canvas.Select(Pen(pen_width,
                     dark_mode ? COLOR_LIGHT_GRAY : COLOR_DARK_GRAY));
  canvas.SelectHollowBrush();
  canvas.DrawRectangle(box_rc);

  if (checked) {
    // Draw checkmark
    const int cx = (box_rc.left + box_rc.right) / 2;
    const int cy = (box_rc.top + box_rc.bottom) / 2;
    const int size = (box_rc.right - box_rc.left) / 2;

    canvas.Select(Pen(focus_width,
                       dark_mode ? COLOR_XCSOAR_LIGHT : COLOR_XCSOAR_DARK));
    // Draw checkmark as two lines
    canvas.DrawLine({cx - size + 1, cy}, {cx - size/3, cy + size - 1});
    canvas.DrawLine({cx - size/3, cy + size - 1},
                     {cx + size - 1, cy - size + 2});
  }
}

RichTextWindow::RichTextWindow() noexcept = default;
RichTextWindow::~RichTextWindow() noexcept = default;

const Bitmap *
RichTextWindow::LoadImage(const std::string &url) const noexcept
{
  auto it = image_cache.find(url);
  if (it != image_cache.end())
    return it->second.IsDefined() ? &it->second : nullptr;

  Bitmap bitmap;

  if (StringStartsWith(url.c_str(), "resource:")) {
    const char *name = url.c_str() + 9;

#ifdef ENABLE_OPENGL
    /* On OpenGL, prefer the _RGBA variant (PNG with alpha channel)
       over the base resource (BMP with white background) so that
       images composite correctly on non-white backgrounds. */
    const std::string rgba_name = std::string(name) + "_RGBA";
    ResourceId rgba_id = LookupResourceByName(rgba_name.c_str());
    if (rgba_id.IsDefined())
      bitmap.Load(rgba_id);
#endif

    if (!bitmap.IsDefined()) {
      ResourceId id = LookupResourceByName(name);
      if (id.IsDefined())
        bitmap.Load(id);
    }
  }

  bool defined = bitmap.IsDefined();
  auto [pos, _] = image_cache.emplace(url, std::move(bitmap));

  return defined ? &pos->second : nullptr;
}

const MarkdownImage *
RichTextWindow::FindBlockImageForLine(
  std::size_t line_start,
  std::size_t line_length) const noexcept
{
  const std::size_t line_end = line_start + line_length;

  for (const auto &img : parsed.images) {
    if (!img.is_block)
      continue;
    if (img.position >= line_start && img.position < line_end)
      return &img;
  }
  return nullptr;
}

const MarkdownImage *
RichTextWindow::FindImageAtPosition(
  std::size_t start,
  std::size_t length) const noexcept
{
  const std::size_t end = start + length;

  for (const auto &img : parsed.images) {
    if (img.position >= start && img.position < end)
      return &img;
  }
  return nullptr;
}

/**
 * Find the admonition colour for a heading at the given text position.
 * Returns the colour if a !!! marker precedes this heading (within a
 * small window to account for intervening blank lines), otherwise
 * returns the default heading colour.
 */
Color
RichTextWindow::GetAdmonitionColor(std::size_t heading_start) const noexcept
{
  for (const auto &adm : parsed.admonitions) {
    /* The marker position is where the !!! line was consumed.
       The heading follows on the next non-blank line, so its start
       is at most a few newlines later. */
    if (adm.position <= heading_start &&
        heading_start - adm.position <= 4) {
      switch (adm.type) {
      case AdmonitionType::WARNING:
        return dark_mode ? COLOR_INVERSE_RED : COLOR_RED;
      case AdmonitionType::CAUTION:
        return COLOR_ORANGE;
      case AdmonitionType::IMPORTANT:
        return dark_mode ? COLOR_ADMONITION_IMPORTANT_DARK
                         : COLOR_ADMONITION_IMPORTANT;
      case AdmonitionType::NOTE:
        return dark_mode ? COLOR_XCSOAR_LIGHT : COLOR_XCSOAR;
      case AdmonitionType::TIP:
        return dark_mode ? COLOR_GREEN : COLOR_ADMONITION_TIP;
      }
    }
  }

  return dark_mode ? COLOR_XCSOAR_LIGHT : COLOR_XCSOAR_DARK;
}

void
RichTextWindow::EnsureLineLayout() const noexcept
{
  EnsureWrappedText();

  if (!wrapped_text)
    return;

  const int padding = GetContentPadding();
  const auto size = GetSize();
  if (size.width <= unsigned(padding * 2))
    return;

  const unsigned text_width = size.width - padding * 2;

  if (!line_y_offsets.empty() && line_layout_width == text_width)
    return;

  /* Add extra inter-line spacing on touch screens for larger tap
     targets and more comfortable reading */
  const int touch_line_pad = IsTouchLayout() ? Layout::Scale(2) : 0;
  const int base_line_height =
    (font ? font->GetLineSpacing() : 20) + touch_line_pad;
  const std::size_t n = wrapped_text->lines.size();

  line_y_offsets.resize(n);
  line_heights.resize(n);

  /* Helper: find the heading style that covers the start of a line,
     so we can use the correct (larger) font height for heading lines */
  auto GetLineHeadingStyle =
    [this](std::size_t line_start) noexcept -> TextStyle {
    for (const auto &span : parsed.styles) {
      if (line_start >= span.start && line_start < span.end) {
        if (span.style == TextStyle::Heading1 ||
            span.style == TextStyle::Heading2 ||
            span.style == TextStyle::Heading3)
          return span.style;
      }
    }
    return TextStyle::Normal;
  };

  int y = 0;
  for (std::size_t i = 0; i < n; ++i) {
    const auto &line = wrapped_text->lines[i];
    line_y_offsets[i] = y;

    /* Determine the text line height, accounting for headings */
    const TextStyle heading_style = GetLineHeadingStyle(line.start);
    const int line_height =
      (heading_style != TextStyle::Normal)
        ? GetHeadingFont(heading_style).GetLineSpacing()
            + touch_line_pad
        : base_line_height;

    const MarkdownImage *block_img =
      FindBlockImageForLine(line.start, line.length);

    if (block_img != nullptr) {
      const Bitmap *bmp = LoadImage(block_img->url);
      if (bmp != nullptr && bmp->IsDefined()) {
        PixelSize img_size = bmp->GetSize();
        if (img_size.width == 0)
          img_size.width = 1;
        /* Scale to fit within text_width, maintaining aspect ratio */
        unsigned target_w = std::min(text_width, img_size.width);
        unsigned target_h = img_size.height * target_w / img_size.width;
        /* Add small vertical padding around the image */
        line_heights[i] = static_cast<int>(target_h) + padding;
      } else {
        line_heights[i] = line_height;
      }
    } else {
      /* Check for inline images that may be taller than text */
      const MarkdownImage *inline_img =
        FindImageAtPosition(line.start, line.length);
      if (inline_img != nullptr && !inline_img->is_block) {
        const Bitmap *bmp = LoadImage(inline_img->url);
        if (bmp != nullptr && bmp->IsDefined()) {
          /* Inline images render at 2x line height; add padding */
          int img_h = base_line_height * 2 + padding;
          line_heights[i] = std::max(line_height, img_h);
        } else {
          line_heights[i] = line_height;
        }
      } else {
        line_heights[i] = line_height;
      }
    }

    y += line_heights[i];
  }

  line_layout_width = text_width;
}

void
RichTextWindow::Create(ContainerWindow &parent, PixelRect rc,
                       const WindowStyle style)
{
  PaintWindow::Create(parent, rc, style);
}

void
RichTextWindow::InvalidateLayout() noexcept
{
  cached_content_height = 0;
  cached_height_width = 0;
  wrapped_text.reset();
  wrapped_text_width = 0;
  segmented_lines.reset();
  segmented_lines_width = 0;
  line_y_offsets.clear();
  line_heights.clear();
  line_layout_width = 0;
}

std::size_t
RichTextWindow::FindCheckboxStyleIndex(
  std::size_t text_pos) const noexcept
{
  for (std::size_t i = 0; i < parsed.styles.size(); ++i) {
    const auto &span = parsed.styles[i];
    if (text_pos >= span.start && text_pos < span.end &&
        (span.style == TextStyle::Checkbox ||
         span.style == TextStyle::CheckboxChecked))
      return i;
  }
  return SIZE_MAX;
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

  InvalidateLayout();
  image_cache.clear();

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

  const int padding = GetContentPadding();
  const auto size = GetSize();
  if (size.width <= unsigned(padding * 2))
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

  const int padding = GetContentPadding();
  const auto size = GetSize();
  if (size.width <= unsigned(padding * 2))
    return;

  const unsigned text_width = size.width - padding * 2;

  if (segmented_lines && segmented_lines_width == text_width)
    return;

  segmented_lines = std::make_unique<std::vector<SegmentedLine>>();
  segmented_lines->reserve(wrapped_text->lines.size());

  /* Track whether the current paragraph is a list item so we can
     mark continuation (wrapped) lines for hanging-indent rendering. */
  bool in_list_paragraph = false;
  const auto &text = parsed.text;

  /* Check if a paragraph starts with a numbered list marker
     (e.g. "1. ", "12. ") in the processed text.  These are not
     tracked by the markdown parser as styled spans. */
  auto StartsWithNumberedList = [&text](std::size_t start,
                                        std::size_t length) noexcept {
    const std::size_t end = start + length;
    std::size_t i = start;
    if (i >= end || text[i] < '0' || text[i] > '9')
      return false;
    while (i < end && text[i] >= '0' && text[i] <= '9')
      ++i;
    return i < end && text[i] == '.' &&
           i + 1 < end && text[i + 1] == ' ';
  };

  /* Detect if a new paragraph starts with a list marker, checkbox,
     or numbered item so wrapped continuation lines get indented. */
  auto IsListParagraphStart =
    [&StartsWithNumberedList](const SegmentedLine &seg_line,
                              std::size_t line_start,
                              std::size_t line_length) noexcept {
    if (!seg_line.segments.empty()) {
      const auto &first = seg_line.segments.front();
      if (first.IsListItem() || first.IsCheckbox())
        return true;
    }
    return StartsWithNumberedList(line_start, line_length);
  };

  // If no links and no styles, each line is a single normal segment
  if (parsed.links.empty() && parsed.styles.empty()) {
    for (const auto &line : wrapped_text->lines) {
      const bool is_new_para =
        (line.start == 0 || text[line.start - 1] == '\n');

      SegmentedLine seg_line;
      if (line.length > 0)
        seg_line.segments.push_back({line.start, line.length, SIZE_MAX, TextStyle::Normal});

      if (is_new_para)
        in_list_paragraph =
          IsListParagraphStart(seg_line, line.start, line.length);
      else if (in_list_paragraph)
        seg_line.is_list_continuation = true;

      segmented_lines->push_back(std::move(seg_line));
    }
    segmented_lines_width = text_width;
    return;
  }

  // Segment each line by link and style boundaries
  for (const auto &line : wrapped_text->lines) {
    const bool is_new_para =
      (line.start == 0 || text[line.start - 1] == '\n');

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

    /* Detect list-item paragraph continuations (including
       checkboxes and numbered lists like "1. item") */
    if (is_new_para) {
      in_list_paragraph =
        IsListParagraphStart(seg_line, line.start, line.length);
    } else if (in_list_paragraph) {
      seg_line.is_list_continuation = true;
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

  const int padding = GetContentPadding();
  const auto size = GetSize();
  if (size.width <= unsigned(padding * 2))
    return 0;

  const unsigned text_width = size.width - padding * 2;

  if (cached_content_height > 0 && cached_height_width == text_width)
    return cached_content_height;

  EnsureLineLayout();

  if (line_y_offsets.empty())
    return 0;

  /* Total height = Y offset of last line + its height + padding*2 */
  const std::size_t n = line_y_offsets.size();
  const unsigned content_height =
    static_cast<unsigned>(line_y_offsets[n - 1] + line_heights[n - 1])
    + padding * 2;

  cached_content_height = content_height;
  cached_height_width = text_width;

  return content_height;
}

void
RichTextWindow::OnResize(PixelSize new_size) noexcept
{
  const auto old_size = GetSize();
  PaintWindow::OnResize(new_size);

  /* Text wrapping and line layout depend only on width.  Skip the
     expensive re-layout when only the height changed (e.g. during
     vertical resizes or resolution changes that don't alter width). */
  if (new_size.width != old_size.width)
    InvalidateLayout();

  if (!parsed.text.empty())
    Invalidate();
}

void
RichTextWindow::OnSetFocus() noexcept
{
  PaintWindow::OnSetFocus();
  focus_exhausted_down = false;
  focus_exhausted_up = false;
  Invalidate();
}

void
RichTextWindow::OnKillFocus() noexcept
{
  PaintWindow::OnKillFocus();
  ClearLinkFocus();
  Invalidate();
}

bool
RichTextWindow::RenderInlineImage(Canvas &canvas,
                                  const TextSegment &seg,
                                  int &x, int y,
                                  int cur_line_height,
                                  int text_line_height) const noexcept
{
  const MarkdownImage *inline_img =
    FindImageAtPosition(seg.start, seg.length);
  if (inline_img == nullptr || inline_img->is_block)
    return false;

  const Bitmap *bmp = LoadImage(inline_img->url);
  if (bmp == nullptr || !bmp->IsDefined())
    return false;

  PixelSize img_size = bmp->GetSize();
  if (img_size.height == 0)
    img_size.height = 1;
  /* Render at 2x text line height for visibility */
  unsigned target_h = static_cast<unsigned>(text_line_height) * 2;
  unsigned target_w = img_size.width * target_h / img_size.height;
  int img_y = y + (cur_line_height -
                   static_cast<int>(target_h)) / 2;

#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#endif
  canvas.Stretch({x, img_y},
                 {target_w, target_h},
                 *bmp, {0, 0}, img_size);
  x += static_cast<int>(target_w) + Layout::Scale(2);
  return true;
}

void
RichTextWindow::RenderLinkSegment(Canvas &canvas,
                                  const TextSegment &seg,
                                  const char *text_data,
                                  int &x, int text_y,
                                  int visible_top,
                                  int text_line_height) noexcept
{
  std::string_view seg_text(text_data + seg.start, seg.length);
  const bool is_focused = IsLinkFocused(seg.link_index);

  if (dark_mode)
    canvas.SetTextColor(is_focused ? COLOR_WHITE : COLOR_XCSOAR_LIGHT);
  else
    canvas.SetTextColor(is_focused ? COLOR_XCSOAR_LIGHT : COLOR_XCSOAR);

  const int link_spacing = canvas.CalcTextSize(" ").width;
  x += link_spacing;

  const PixelSize text_size = canvas.CalcTextSize(seg_text);
  const int seg_width = static_cast<int>(text_size.width);
  canvas.DrawText({x, text_y}, seg_text);

  /* Expand the touch target vertically on touch screens
     so links are easier to tap */
  const int touch_expand = IsTouchLayout() ? Layout::Scale(4) : 0;
  PixelRect link_rect{x, text_y + visible_top - touch_expand,
                      x + seg_width,
                      text_y + visible_top + text_line_height
                        + touch_expand};
  RegisterLinkRect(seg.link_index, link_rect);

  if (is_focused && HasFocus()) {
    const int focus_pad = Layout::ScalePenWidth(2);
    PixelRect focus_rc{x - focus_pad, text_y - focus_pad,
                       x + seg_width + focus_pad,
                       text_y + text_line_height + focus_pad};
    canvas.Select(Pen(Layout::ScalePenWidth(1),
                      dark_mode ? COLOR_LIGHT_GRAY
                                : COLOR_DARK_GRAY));
    canvas.SelectHollowBrush();
    canvas.DrawRectangle(focus_rc);
  }

  x += seg_width;
  x += link_spacing;
}

void
RichTextWindow::RenderCheckboxSegment(Canvas &canvas,
                                      const TextSegment &seg,
                                      int &x, int text_y,
                                      int visible_top,
                                      int text_line_height) noexcept
{
  const std::size_t style_idx = FindCheckboxStyleIndex(seg.start);

  const int box_margin = Layout::ScalePenWidth(2);
  const int box_size = IsTouchLayout()
    ? std::max(text_line_height - box_margin,
               static_cast<int>(
                 Layout::GetMinimumControlHeight()) / 2)
    : text_line_height - box_margin * 2;
  const int box_y = text_y + (text_line_height - box_size) / 2;
  PixelRect box_rc{x, box_y, x + box_size, box_y + box_size};

  bool checked = (style_idx != SIZE_MAX)
    ? IsCheckboxChecked(style_idx)
    : seg.IsCheckboxChecked();
  bool focused = (style_idx != SIZE_MAX) &&
                 IsCheckboxFocused(style_idx);
  DrawSimpleCheckbox(canvas, box_rc, checked, focused, dark_mode);

  if (style_idx != SIZE_MAX) {
    /* Expand hit area on touch screens for easier tapping */
    const int cb_expand = IsTouchLayout() ? Layout::Scale(4) : 0;
    PixelRect click_rc{x - cb_expand,
                       box_y + visible_top - cb_expand,
                       x + box_size + cb_expand,
                       box_y + box_size + visible_top + cb_expand};
    checkbox_rects.push_back({click_rc, style_idx});
  }

  const int box_gap = Layout::Scale(4);
  x += box_size + box_gap;
}

void
RichTextWindow::RenderPlainSegment(Canvas &canvas,
                                   const TextSegment &seg,
                                   const char *text_data,
                                   int &x, int text_y) const noexcept
{
  std::string_view seg_text(text_data + seg.start, seg.length);

  if (seg.IsHeading())
    canvas.SetTextColor(GetAdmonitionColor(seg.start));
  else if (seg.IsListItem())
    canvas.SetTextColor(dark_mode ? COLOR_LIGHT_GRAY : COLOR_DARK_GRAY);
  else
    canvas.SetTextColor(dark_mode ? COLOR_WHITE : COLOR_BLACK);

  const PixelSize text_size = canvas.CalcTextSize(seg_text);
  canvas.DrawText({x, text_y}, seg_text);
  x += static_cast<int>(text_size.width);

  /* List items get trailing whitespace so the bullet
     character does not crowd the next segment */
  if (seg.IsListItem())
    x += canvas.CalcTextSize(" ").width;
}

void
RichTextWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(background_color);

  if (parsed.text.empty() || font == nullptr)
    return;

  const int padding = GetContentPadding();
  const int text_line_height = font->GetLineSpacing();

  int visible_top, visible_bottom, viewport_height;
  GetVisibleArea(visible_top, visible_bottom, viewport_height);

  EnsureSegmentedLines();
  EnsureLineLayout();

  if (!segmented_lines || segmented_lines->empty() ||
      line_y_offsets.empty())
    return;

  const std::size_t n_lines = segmented_lines->size();

  /* Find first visible line using line layout Y offsets */
  std::size_t first_line = 0;
  for (std::size_t i = 0; i < n_lines; ++i) {
    if (padding + line_y_offsets[i] + line_heights[i] > visible_top) {
      first_line = i;
      break;
    }
  }

  /* Find last visible line */
  std::size_t last_line = n_lines;
  for (std::size_t i = first_line; i < n_lines; ++i) {
    if (padding + line_y_offsets[i] > visible_bottom) {
      last_line = i;
      break;
    }
  }

  const auto widget_size = GetSize();
  if (widget_size.width <= unsigned(padding * 2))
    return;
  const unsigned text_width = widget_size.width - padding * 2;

  const auto rc = canvas.GetRect();
  const PixelSize sub_size{rc.GetWidth(),
                           static_cast<unsigned>(viewport_height)};
  SubCanvas sub_canvas(canvas, PixelPoint{0, visible_top}, sub_size);
  sub_canvas.SetBackgroundTransparent();

  ClearLinkRects();
  checkbox_rects.clear();

  const char *text_data = parsed.text.c_str();

  // Indentation for list items and checkboxes (roughly 2 spaces)
  const int list_indent = font->TextSize("  ").width;

  for (std::size_t i = first_line; i < last_line; ++i) {
    const SegmentedLine &line = (*segmented_lines)[i];
    const int y = padding + line_y_offsets[i] - visible_top;
    const int cur_line_height = line_heights[i];

    /* Check if this line contains a block image */
    if (!line.segments.empty()) {
      const auto &first_seg = line.segments.front();
      const MarkdownImage *img =
        FindBlockImageForLine(first_seg.start,
                              line.segments.back().start +
                              line.segments.back().length -
                              first_seg.start);
      if (img != nullptr) {
        const Bitmap *bmp = LoadImage(img->url);
        if (bmp != nullptr && bmp->IsDefined()) {
          PixelSize img_size = bmp->GetSize();
          if (img_size.width == 0)
            img_size.width = 1;
          unsigned target_w = std::min(text_width, img_size.width);
          unsigned target_h =
            img_size.height * target_w / img_size.width;
          int img_x = padding +
            static_cast<int>(text_width - target_w) / 2;
          int img_y = y + (cur_line_height -
                           static_cast<int>(target_h)) / 2;

#ifdef ENABLE_OPENGL
          const ScopeAlphaBlend alpha_blend;
#endif
          sub_canvas.Stretch({img_x, img_y},
                             {target_w, target_h},
                             *bmp, {0, 0}, img_size);
          continue; // Skip normal text rendering for this line
        }
      }
    }

    /* Determine the effective text height for this line.
       Heading lines use a larger font, so we need to account for
       that when vertically centering. */
    int effective_text_height = text_line_height;
    if (!line.segments.empty()) {
      const TextSegment &first = line.segments.front();
      if (first.IsHeading())
        effective_text_height =
          GetHeadingFont(first.style).GetLineSpacing();
    }

    /* Normal text rendering -- vertically center text when the
       line is taller than normal (e.g. due to inline images) */
    const int text_y =
      y + (cur_line_height - effective_text_height) / 2;
    int x = padding;
    if (line.is_list_continuation) {
      /* Hanging indent for wrapped continuation of a list item */
      x += list_indent;
    } else if (!line.segments.empty()) {
      const TextSegment &first_seg = line.segments.front();
      if (first_seg.IsCheckbox() || first_seg.IsListItem())
        x += list_indent;
    }

    for (const TextSegment &seg : line.segments) {
      if (RenderInlineImage(sub_canvas, seg, x, y,
                            cur_line_height, text_line_height))
        continue;

      const Font *seg_font = font;
      if (seg.IsHeading())
        seg_font = &GetHeadingFont(seg.style);
      else if (seg.IsBold() && bold_font)
        seg_font = bold_font;

      sub_canvas.Select(*seg_font);

      if (seg.IsLink())
        RenderLinkSegment(sub_canvas, seg, text_data,
                          x, text_y, visible_top,
                          text_line_height);
      else if (seg.IsCheckbox())
        RenderCheckboxSegment(sub_canvas, seg,
                              x, text_y, visible_top,
                              text_line_height);
      else
        RenderPlainSegment(sub_canvas, seg, text_data,
                           x, text_y);
    }
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
  /* If focus navigation has been exhausted in this direction,
     tell the dialog we don't want the key so it can cycle focus
     to the next tab stop (e.g. pager buttons). */
  if (key_code == KEY_DOWN && focus_exhausted_down)
    return false;
  if (key_code == KEY_UP && focus_exhausted_up)
    return false;

  switch (key_code) {
  case KEY_UP:
  case KEY_DOWN:
    /* Claim UP/DOWN if we have any focusable items (links or
       checkboxes) in the full content, or if paint-time data
       shows visible items. */
    if (!parsed.links.empty() || !checkbox_rects.empty())
      return true;
    for (const auto &s : parsed.styles)
      if (s.style == TextStyle::Checkbox ||
          s.style == TextStyle::CheckboxChecked)
        return true;
    break;

  case KEY_RETURN:
    if (focused_checkbox_style.has_value() || focused_link.has_value())
      return true;
    break;
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

/**
 * A focusable item (link or checkbox) identified during keyboard
 * navigation.  Sorted by vertical position so UP/DOWN moves in
 * document order.
 */
struct FocusItem {
  int y_pos;          ///< Content-space Y coordinate
  int height;         ///< Approximate item height
  bool is_checkbox;
  std::size_t index;  ///< style_index for checkboxes, link_index for links

  bool operator<(const FocusItem &other) const noexcept {
    if (y_pos != other.y_pos)
      return y_pos < other.y_pos;
    /* Checkboxes before links at the same position */
    return is_checkbox && !other.is_checkbox;
  }
};

/**
 * Build a sorted list of all focusable items (links and checkboxes)
 * from the pre-computed segmented lines and line layout data.
 */
static std::vector<FocusItem>
BuildFocusItems(
  const RichTextWindow &self,
  const std::vector<SegmentedLine> &lines,
  const std::vector<int> &y_offsets,
  const std::vector<int> &heights,
  int padding) noexcept
{
  std::vector<FocusItem> items;

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const SegmentedLine &line = lines[i];
    const int y = padding + y_offsets[i];
    const int h = heights[i];

    for (const TextSegment &seg : line.segments) {
      /* Links */
      if (seg.IsLink() && seg.link_index != SIZE_MAX) {
        bool already = false;
        for (const auto &it : items)
          if (!it.is_checkbox && it.index == seg.link_index) {
            already = true;
            break;
          }
        if (!already)
          items.push_back({y, h, false, seg.link_index});
      }

      /* Checkboxes */
      if (seg.IsCheckbox()) {
        const std::size_t si = self.FindCheckboxStyleIndex(seg.start);
        if (si != SIZE_MAX) {
          bool already = false;
          for (const auto &it : items)
            if (it.is_checkbox && it.index == si) {
              already = true;
              break;
            }
          if (!already)
            items.push_back({y, h, true, si});
        }
      }
    }
  }

  std::sort(items.begin(), items.end());
  return items;
}

/**
 * Find the index of the currently focused item in the sorted list.
 */
[[gnu::pure]]
static std::optional<std::size_t>
FindCurrentFocusIndex(
  const std::vector<FocusItem> &items,
  std::optional<std::size_t> focused_checkbox,
  std::optional<std::size_t> focused_link) noexcept
{
  for (std::size_t i = 0; i < items.size(); ++i) {
    const auto &item = items[i];
    if (item.is_checkbox && focused_checkbox.has_value() &&
        focused_checkbox.value() == item.index)
      return i;
    if (!item.is_checkbox && focused_link.has_value() &&
        focused_link.value() == item.index)
      return i;
  }
  return std::nullopt;
}

void
RichTextWindow::ScrollToFocusItem(const FocusItem &item,
                                  int text_line_height) noexcept
{
  if (item.is_checkbox) {
    focused_checkbox_style = item.index;
    focused_link.reset();
  } else {
    focused_link = item.index;
    focused_checkbox_style.reset();
  }

  /* Scroll to the item using content coordinates */
  ContainerWindow *parent = GetParent();
  if (parent != nullptr) {
    const PixelRect window_rect = GetPosition();
    const int parent_height = parent->GetSize().height;
    const int scroll_margin = Layout::Scale(20);

    /* Convert content-space Y to parent coordinates */
    const int item_top = item.y_pos + window_rect.top;
    const int item_bottom =
      item.y_pos + text_line_height + window_rect.top;

    if (item_top < scroll_margin ||
        item_bottom > parent_height - scroll_margin) {
      PixelRect scroll_rc;
      scroll_rc.left = 0;
      scroll_rc.right = 1;
      if (item_top < scroll_margin) {
        scroll_rc.top = item.y_pos - scroll_margin;
        scroll_rc.bottom = scroll_rc.top + 1;
      } else {
        scroll_rc.top = item.y_pos + text_line_height + scroll_margin;
        scroll_rc.bottom = scroll_rc.top + 1;
      }
      parent->ScrollTo(scroll_rc);
    }
  }

  Invalidate();
}

bool
RichTextWindow::OnKeyDown(unsigned key_code) noexcept
{
  EnsureSegmentedLines();
  EnsureLineLayout();

  if (!segmented_lines || segmented_lines->empty() ||
      line_y_offsets.empty())
    return LinkableWindow::OnKeyDown(key_code);

  const int padding = GetContentPadding();
  const int text_line_height =
    font != nullptr ? font->GetLineSpacing() : 16;

  const auto items = BuildFocusItems(*this, *segmented_lines,
                                     line_y_offsets, line_heights,
                                     padding);
  if (items.empty())
    return LinkableWindow::OnKeyDown(key_code);

  const auto current_pos =
    FindCurrentFocusIndex(items, focused_checkbox_style,
                          focused_link);

  /* Maximum distance (in pixels) between two focus items before we
     let the scroll widget handle line-by-line scrolling instead of
     jumping directly.  This prevents jarring full-document jumps
     when links are far apart (e.g. one at the top, several at the
     bottom of a long text). */
  const int viewport_height = GetClientRect().GetHeight();
  const int max_jump = viewport_height;

  /* Current visible range in content-space coordinates.  When
     the window is scrolled inside a VScrollPanel, GetPosition().top
     becomes negative by the scroll offset. */
  const PixelRect win_pos = GetPosition();
  const int visible_top = -win_pos.top;
  const int visible_bottom = visible_top + viewport_height;

  switch (key_code) {
  case KEY_DOWN:
    if (!current_pos.has_value()) {
      /* No focus set.  When focus_exhausted_down is active (we
         cleared focus because the next link was too far away),
         only pick up items in the lower portion of the viewport
         to avoid jumping back to items we've already scrolled
         past.  Otherwise accept any visible item. */
      const int search_start = focus_exhausted_down
        ? visible_top + viewport_height / 3
        : visible_top - text_line_height;

      const FocusItem *best = nullptr;
      for (const auto &it : items) {
        if (it.y_pos >= search_start) {
          best = &it;
          break;
        }
      }
      if (best != nullptr && best->y_pos <= visible_bottom) {
        focus_exhausted_down = false;
        focus_exhausted_up = false;
        ScrollToFocusItem(*best, text_line_height);
        return true;
      }
      /* No suitable item — let scroll widget handle it */
      return false;
    } else if (current_pos.value() + 1 < items.size()) {
      const auto &cur = items[current_pos.value()];
      const auto &next = items[current_pos.value() + 1];
      if (next.y_pos - cur.y_pos > max_jump) {
        /* Next item is too far away — clear focus and let
           the scroll widget handle line-by-line scrolling. */
        focused_checkbox_style.reset();
        focused_link.reset();
        focus_exhausted_down = true;
        Invalidate();
        return false;
      }
      focus_exhausted_up = false;
      ScrollToFocusItem(next, text_line_height);
      return true;
    } else {
      focused_checkbox_style.reset();
      focused_link.reset();
      focus_exhausted_down = true;
      Invalidate();
      if (ContainerWindow *parent = GetParent())
        return parent->InjectKeyPress(key_code);
    }
    break;

  case KEY_UP:
    if (!current_pos.has_value()) {
      /* No focus set.  When focus_exhausted_up is active,
         only pick up items in the upper portion of the viewport
         to avoid jumping forward to items we've scrolled past.
         Otherwise accept any visible item. */
      const int search_end = focus_exhausted_up
        ? visible_bottom - viewport_height / 3
        : visible_bottom;

      const FocusItem *best = nullptr;
      for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if (it->y_pos <= search_end) {
          best = &*it;
          break;
        }
      }
      if (best != nullptr && best->y_pos >= visible_top - text_line_height) {
        focus_exhausted_up = false;
        focus_exhausted_down = false;
        ScrollToFocusItem(*best, text_line_height);
        return true;
      }
      /* No suitable item — let scroll widget handle it */
      return false;
    } else if (current_pos.value() > 0) {
      const auto &cur = items[current_pos.value()];
      const auto &prev = items[current_pos.value() - 1];
      if (cur.y_pos - prev.y_pos > max_jump) {
        /* Previous item is too far away — clear focus and let
           the scroll widget handle line-by-line scrolling. */
        focused_checkbox_style.reset();
        focused_link.reset();
        focus_exhausted_up = true;
        Invalidate();
        return false;
      }
      focus_exhausted_down = false;
      ScrollToFocusItem(prev, text_line_height);
      return true;
    } else {
      focused_checkbox_style.reset();
      focused_link.reset();
      focus_exhausted_up = true;
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
