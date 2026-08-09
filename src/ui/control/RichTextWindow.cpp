// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RichTextWindow.hpp"
#include "ui/canvas/TextWrapper.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/event/KeyCode.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"
#include "ResourceLookup.hpp"
#include "Form/CheckBox.hpp"
#include "Form/VScrollPanel.hpp"
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
 * The processed line starts a checkbox span (first line of a list item
 * with "- [ ]" / "- [x]"), not a wrapped continuation line.
 */
[[gnu::pure]]
static bool
LineSpanStartsWithCheckbox(const ParsedMarkdown &p,
                           std::size_t line_start) noexcept
{
  for (const auto &span : p.styles) {
    if ((span.style == TextStyle::Checkbox ||
         span.style == TextStyle::CheckboxChecked) &&
        span.start == line_start)
      return true;
  }
  return false;
}

/**
 * Pixel size of the square checkbox cell (not including suffix gap).
 * Touch layouts use at least the standard control row height so
 * checkboxes are as tappable as buttons in dialogs.
 */
[[gnu::pure]]
static int
CheckboxBoxSize(const Font &font) noexcept
{
  const int text_line_height = font.GetLineSpacing();
  const int box_margin = static_cast<int>(Layout::ScalePenWidth(2));
  if (!IsTouchLayout())
    return text_line_height - 2 * box_margin;

  const int from_text = text_line_height - 2 * box_margin;
  const int from_row = static_cast<int>(Layout::GetMaximumControlHeight()) -
                       2 * box_margin;
  return std::max({from_text, from_row, 4});
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

  /**
   * For #is_list_continuation: x offset from #padding to where body text
   * should start (same as x after drawing the list prefix on the first line).
   */
  int list_hang_offset = 0;
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

  const unsigned column_width = size.width - padding * 2;
  const unsigned text_width = CalcWrapTextWidth(column_width);
  if (text_width == 0)
    return;

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
        /* Scale to fit within the full column (not wrap slack). */
        unsigned target_w = std::min(column_width, img_size.width);
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

    if (IsTouchLayout() && font != nullptr &&
        block_img == nullptr &&
        LineSpanStartsWithCheckbox(parsed, line.start)) {
      const int need = CheckboxBoxSize(*font) + Layout::Scale(4);
      line_heights[i] = std::max(line_heights[i], need);
    }

    y += line_heights[i];
  }

  line_layout_width = text_width;

  if (!line_y_offsets.empty()) {
    const std::size_t n = line_y_offsets.size();
    cached_content_height =
      static_cast<unsigned>(line_y_offsets[n - 1] + line_heights[n - 1])
      + padding * 2;
    cached_height_width = text_width;
  }
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
  synced_scroll_height = 0;
  wrapped_text.reset();
  wrapped_text_width = 0;
  segmented_lines.reset();
  segmented_lines_width = 0;
  line_y_offsets.clear();
  line_heights.clear();
  line_layout_width = 0;
  checkbox_placeholders_expanded = false;
  InvalidateContentCache();
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
  /* Prefer VScrollPanel origin: rich-text children are sized to the
     physical viewport and do not Move() on every scroll tick. */
  if (const auto *panel =
        dynamic_cast<const VScrollPanel *>(GetParent())) {
    viewport_height = static_cast<int>(panel->GetSize().height);
    visible_top = static_cast<int>(panel->GetOrigin());
    visible_bottom = visible_top + viewport_height;
    return;
  }

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

unsigned
RichTextWindow::CalcWrapTextWidth(unsigned column_width) const noexcept
{
  if (font == nullptr || column_width == 0)
    return 0;

  unsigned text_width = column_width;
  bool has_list = false;
  for (const auto &span : parsed.styles) {
    if (span.style == TextStyle::ListItem ||
        span.style == TextStyle::Checkbox ||
        span.style == TextStyle::CheckboxChecked) {
      has_list = true;
      break;
    }
  }

  AnyCanvas measure;
  measure.Select(*font);

  /* Link paint adds a space before/after each segment. */
  if (!parsed.links.empty()) {
    const unsigned space_w =
      std::max(1u, measure.CalcTextSize(" ").width);
    if (text_width > space_w * 2)
      text_width -= space_w * 2;
  }

  /* List first lines paint with a left indent; continuations hang
     under the body.  WrapText uses one width for the whole doc, so
     reserve the hang so wrapped lines do not run past the edge. */
  if (has_list) {
    const unsigned hang = static_cast<unsigned>(
      measure.CalcTextSize("  ").width +
      measure.CalcTextSize("- ").width +
      measure.CalcTextSize(" ").width);
    if (text_width > hang)
      text_width -= hang;
  }

  return text_width;
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

  /* const API: expand placeholders before wrap (mutates parse). */
  const_cast<RichTextWindow *>(this)->ExpandCheckboxPlaceholders();

  const unsigned column_width = size.width - padding * 2;
  const unsigned text_width = CalcWrapTextWidth(column_width);
  if (text_width == 0)
    return;

  if (wrapped_text && wrapped_text_width == text_width)
    return;

  /* Wrap with the body font.  Never use H1/H2 for the whole document
     — one title heading would force every bullet to wrap early.
     Bold is only slightly wider; use it when present so bold runs
     are less likely to overflow. */
  bool has_bold = false;
  for (const auto &span : parsed.styles) {
    if (span.style == TextStyle::Bold ||
        span.style == TextStyle::Heading3) {
      has_bold = true;
      break;
    }
  }
  const Font &wrap_font = has_bold ? GetBoldFont() : *font;

  AnyCanvas canvas;
  canvas.Select(wrap_font);
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

[[gnu::pure]]
static bool
TextStartsWithNumberedList(const std::string &text,
                           std::size_t line_start,
                           std::size_t line_length) noexcept
{
  const std::size_t end = line_start + line_length;
  std::size_t i = line_start;
  if (i >= end || text[i] < '0' || text[i] > '9')
    return false;
  while (i < end && text[i] >= '0' && text[i] <= '9')
    ++i;
  return i < end && text[i] == '.' &&
         i + 1 < end && text[i + 1] == ' ';
}

[[gnu::pure]]
static int
MeasureNumberedListPrefixWidth(const Font &font, const std::string &text,
                               std::size_t line_start,
                               std::size_t line_length) noexcept
{
  const std::size_t end = line_start + line_length;
  std::size_t i = line_start;
  if (i >= end || text[i] < '0' || text[i] > '9')
    return 0;
  while (i < end && text[i] >= '0' && text[i] <= '9')
    ++i;
  if (i >= end || text[i] != '.' || i + 1 >= end || text[i + 1] != ' ')
    return 0;
  const std::size_t n = i + 2 - line_start;
  return font.TextSize(std::string_view(text.c_str() + line_start, n)).width;
}

/**
 * Width of checkbox + gap after it (must match #RenderCheckboxSegment).
 */
[[gnu::pure]]
static int
CheckboxPrefixWidth(const Font &font) noexcept
{
  const int box_size = CheckboxBoxSize(font);
  return box_size + Layout::Scale(4);
}

void
RichTextWindow::ExpandCheckboxPlaceholders() noexcept
{
  if (checkbox_placeholders_expanded || font == nullptr)
    return;

  const int need_px = CheckboxPrefixWidth(*font);
  AnyCanvas canvas;
  canvas.Select(*font);
  const int space_px =
    std::max(1, static_cast<int>(canvas.CalcTextSize(" ").width));
  const int n_spaces =
    std::max(2, (need_px + space_px - 1) / space_px);

  /* Widen/shrink from the end so earlier offsets stay stable. */
  for (std::size_t si = parsed.styles.size(); si-- > 0;) {
    auto &span = parsed.styles[si];
    if (span.style != TextStyle::Checkbox &&
        span.style != TextStyle::CheckboxChecked)
      continue;
    if (span.end <= span.start)
      continue;

    const int cur = static_cast<int>(span.end - span.start);
    const int delta = n_spaces - cur;
    if (delta == 0)
      continue;

    if (delta > 0)
      parsed.text.insert(span.start + cur, delta, ' ');
    else
      parsed.text.erase(span.start + n_spaces,
                        static_cast<std::size_t>(-delta));

    /* Other spans/links: shift from the edit point.  This span's
       end is updated explicitly so it is not double-shifted. */
    const std::size_t edit_pos = delta > 0
      ? span.start + cur
      : span.start + n_spaces;
    span.end = span.start + n_spaces;
    for (auto &other : parsed.styles) {
      if (&other == &span)
        continue;
      if (other.start >= edit_pos) {
        other.start += delta;
        other.end += delta;
      } else if (other.end > edit_pos)
        other.end += delta;
    }
    for (auto &link : parsed.links) {
      if (link.start >= edit_pos) {
        link.start += delta;
        link.end += delta;
      } else if (link.end > edit_pos)
        link.end += delta;
    }
    for (auto &img : parsed.images) {
      if (img.position >= edit_pos)
        img.position += delta;
    }
  }

  checkbox_placeholders_expanded = true;
  if (checkbox_toggled.size() != parsed.styles.size())
    checkbox_toggled.assign(parsed.styles.size(), 0);
}

/**
 * Horizontal distance from content #padding to where body text begins on the
 * first line of a list paragraph (same as x after drawing the list prefix).
 */
[[gnu::pure]]
static int
ComputeListBodyOffsetFromPadding(const Font &font,
                                 const SegmentedLine &first_line,
                                 const std::string &text,
                                 std::size_t line_start,
                                 std::size_t line_length,
                                 int list_indent) noexcept
{
  if (!first_line.segments.empty()) {
    const auto &first = first_line.segments.front();
    if (first.IsListItem())
      return list_indent + font.TextSize("- ").width +
             font.TextSize(" ").width;
    if (first.IsCheckbox())
      return list_indent + CheckboxPrefixWidth(font);
  }
  if (TextStartsWithNumberedList(text, line_start, line_length))
    return MeasureNumberedListPrefixWidth(font, text, line_start,
                                          line_length);

  return list_indent;
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

  /* Key off wrap width (not raw column) so hang/link slack stays in sync. */
  const unsigned text_width =
    CalcWrapTextWidth(size.width - padding * 2);
  if (text_width == 0)
    return;

  if (segmented_lines && segmented_lines_width == text_width)
    return;

  segmented_lines = std::make_unique<std::vector<SegmentedLine>>();
  segmented_lines->reserve(wrapped_text->lines.size());

  /* Track whether the current paragraph is a list item so we can
     mark continuation (wrapped) lines for hanging-indent rendering. */
  bool in_list_paragraph = false;
  int current_list_hang = 0;
  const auto &text = parsed.text;

  const int list_indent =
    (font != nullptr) ? font->TextSize("  ").width : 0;

  /* Detect if a new paragraph starts with a list marker, checkbox,
     or numbered item so wrapped continuation lines get indented. */
  auto IsListParagraphStart =
    [&text](const SegmentedLine &seg_line,
            std::size_t line_start,
            std::size_t line_length) noexcept {
    if (!seg_line.segments.empty()) {
      const auto &first = seg_line.segments.front();
      if (first.IsListItem() || first.IsCheckbox())
        return true;
    }
    return TextStartsWithNumberedList(text, line_start, line_length);
  };

  /* Plain-text "- " bullet (e.g. Credits NEWS without Markdown). */
  auto PlainLineStartsWithBullet =
    [&text](std::size_t start, std::size_t length) noexcept {
      return length >= 2 && text[start] == '-' && text[start + 1] == ' ';
    };

  // If no links and no styles, each line is a single normal segment
  if (parsed.links.empty() && parsed.styles.empty()) {
    for (const auto &line : wrapped_text->lines) {
      const bool is_new_para =
        (line.start == 0 || text[line.start - 1] == '\n');

      SegmentedLine seg_line;
      if (line.length > 0)
        seg_line.segments.push_back({line.start, line.length, SIZE_MAX, TextStyle::Normal});

      if (is_new_para) {
        in_list_paragraph =
          IsListParagraphStart(seg_line, line.start, line.length) ||
          PlainLineStartsWithBullet(line.start, line.length);
        if (in_list_paragraph && font != nullptr) {
          if (TextStartsWithNumberedList(text, line.start, line.length))
            current_list_hang = MeasureNumberedListPrefixWidth(
              *font, text, line.start, line.length);
          else if (PlainLineStartsWithBullet(line.start, line.length))
            current_list_hang = font->TextSize("- ").width;
          else
            current_list_hang = list_indent;
        } else
          current_list_hang = 0;
      } else if (in_list_paragraph) {
        seg_line.is_list_continuation = true;
        seg_line.list_hang_offset = current_list_hang;
      }

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
      if (in_list_paragraph && font != nullptr)
        current_list_hang = ComputeListBodyOffsetFromPadding(
          *font, seg_line, text, line.start, line.length, list_indent);
      else
        current_list_hang = 0;
    } else if (in_list_paragraph) {
      seg_line.is_list_continuation = true;
      seg_line.list_hang_offset = current_list_hang;
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

  const unsigned column_width = size.width - padding * 2;
  const unsigned text_width = CalcWrapTextWidth(column_width);

  if (cached_content_height > 0 && cached_height_width == text_width)
    return cached_content_height;

  /* Exact once already wrapped for this width (e.g. after paint). */
  if (wrapped_text && wrapped_text_width == text_width) {
    EnsureLineLayout();
    return line_y_offsets.empty() ? 0 : cached_content_height;
  }

  /* Cheap estimate so Show()/VScroll does not wrap yet.  Use the
     tallest font that may appear so heading pages are not clipped. */
  const int touch_line_pad = IsTouchLayout() ? Layout::Scale(2) : 0;
  unsigned line_height =
    static_cast<unsigned>(font->GetLineSpacing() + touch_line_pad);
  for (const auto &span : parsed.styles) {
    if (span.style != TextStyle::Heading1 &&
        span.style != TextStyle::Heading2 &&
        span.style != TextStyle::Heading3)
      continue;
    const unsigned h =
      static_cast<unsigned>(GetHeadingFont(span.style).GetLineSpacing() +
                            touch_line_pad);
    if (h > line_height)
      line_height = h;
  }

  const unsigned line_count =
    std::max(1u, EstimateWrappedLineCount(*font, text_width, parsed.text));

  cached_content_height = line_count * line_height + padding * 2;
  cached_height_width = text_width;
  return cached_content_height;
}

/**
 * After exact layout, resize the parent #VScrollPanel virtual height
 * so the scrollbar matches content (Show() only saw the estimate).
 */
void
RichTextWindow::SyncParentScrollHeight() noexcept
{
  if (cached_content_height == 0 ||
      synced_scroll_height == cached_content_height)
    return;

  auto *panel = dynamic_cast<VScrollPanel *>(GetParent());
  if (panel == nullptr)
    return;

  /* Move() may trigger OnResize() → InvalidateLayout(), which clears
     cached_content_height; keep the value we synced.  Stay at the
     physical viewport; scrolling uses panel origin, not child y. */
  const unsigned height = cached_content_height;
  const unsigned vh = std::max(panel->GetSize().height, height);
  panel->SetVirtualHeight(vh);
  Move(panel->GetPhysicalRect());
  panel->Invalidate();
  synced_scroll_height = height;
}

unsigned
RichTextWindow::CalculateExactContentHeight() const noexcept
{
  if (font == nullptr || parsed.text.empty())
    return 0;

  const int padding = GetContentPadding();
  const auto size = GetSize();
  if (size.width <= unsigned(padding * 2))
    return 0;

  cached_content_height = 0;
  /* EnsureLineLayout() returns early when the layout is still valid
     for this width, which would leave the height at 0. */
  line_y_offsets.clear();
  line_heights.clear();
  line_layout_width = 0;
  EnsureLineLayout();
  /* const API used by the harness; sync from OnPaint in the UI. */
  return cached_content_height;
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
  else if (new_size.height != old_size.height)
    /* Strip height tracks the viewport. */
    InvalidateContentCache();

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
  focused_checkbox_style.reset();
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
                                  int text_line_height,
                                  int content_y_origin) noexcept
{
  std::string_view seg_text(text_data + seg.start, seg.length);

  if (dark_mode)
    canvas.SetTextColor(COLOR_XCSOAR_LIGHT);
  else
    canvas.SetTextColor(COLOR_XCSOAR);

  const int link_spacing = canvas.CalcTextSize(" ").width;
  x += link_spacing;

  const PixelSize text_size = canvas.CalcTextSize(seg_text);
  const int seg_width = static_cast<int>(text_size.width);
  canvas.DrawText({x, text_y}, seg_text);

  const int touch_expand = IsTouchLayout() ? Layout::Scale(4) : 0;
  PixelRect link_rect{x, text_y - touch_expand,
                      x + seg_width,
                      text_y + text_line_height + touch_expand};
  link_rect.Offset(0, content_y_origin);
  content_hits.push_back({link_rect, seg.link_index, false,
                          seg.start, seg.length});

  x += seg_width;
  x += link_spacing;
}

void
RichTextWindow::RenderCheckboxSegment(Canvas &canvas,
                                      const TextSegment &seg,
                                      int &x, int y_line,
                                      int row_height,
                                      int content_y_origin) noexcept
{
  const std::size_t style_idx = FindCheckboxStyleIndex(seg.start);
  const int box_size = CheckboxBoxSize(*font);
  const int box_y = y_line + (row_height - box_size) / 2;
  PixelRect box_rc{x, box_y, x + box_size, box_y + box_size};

  bool checked = (style_idx != SIZE_MAX)
    ? IsCheckboxChecked(style_idx)
    : seg.IsCheckboxChecked();
  if (dialog_look != nullptr)
    DrawCheckBox(canvas, *dialog_look, box_rc, checked, false,
                 false, true);
  else
    DrawSimpleCheckbox(canvas, box_rc, checked, false, dark_mode);

  if (style_idx != SIZE_MAX) {
    const int cb_expand = IsTouchLayout() ? Layout::Scale(6) : 0;
    PixelRect click_rc{x - cb_expand,
                       box_y - cb_expand,
                       x + box_size + cb_expand,
                       box_y + box_size + cb_expand};
    click_rc.Offset(0, content_y_origin);
    content_hits.push_back({click_rc, style_idx, true});
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
RichTextWindow::PaintContent(Canvas &canvas, int y_origin,
                             int clip_top, int clip_bottom) noexcept
{
  if (parsed.text.empty() || font == nullptr ||
      !segmented_lines || segmented_lines->empty() ||
      line_y_offsets.empty())
    return;

  const int padding = GetContentPadding();
  const int text_line_height = font->GetLineSpacing();
  const auto widget_size = GetSize();
  if (widget_size.width <= unsigned(padding * 2))
    return;
  const unsigned text_width = widget_size.width - padding * 2;

  const std::size_t n_lines = segmented_lines->size();

  /* Binary search: first line whose bottom is past clip_top. */
  std::size_t first_line = 0;
  {
    std::size_t lo = 0, hi = n_lines;
    while (lo < hi) {
      const std::size_t mid = lo + (hi - lo) / 2;
      if (padding + line_y_offsets[mid] + line_heights[mid]
          <= clip_top)
        lo = mid + 1;
      else
        hi = mid;
    }
    first_line = lo;
  }

  std::size_t last_line = n_lines;
  for (std::size_t i = first_line; i < n_lines; ++i) {
    if (padding + line_y_offsets[i] >= clip_bottom) {
      last_line = i;
      break;
    }
  }

  canvas.SetBackgroundTransparent();

  const char *text_data = parsed.text.c_str();
  const int list_indent = font->TextSize("  ").width;

  for (std::size_t i = first_line; i < last_line; ++i) {
    const SegmentedLine &line = (*segmented_lines)[i];
    const int y = padding + line_y_offsets[i] - y_origin;
    const int cur_line_height = line_heights[i];

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
          canvas.Stretch({img_x, img_y},
                         {target_w, target_h},
                         *bmp, {0, 0}, img_size);
          continue;
        }
      }
    }

    int effective_text_height = text_line_height;
    if (!line.segments.empty()) {
      const TextSegment &first = line.segments.front();
      if (first.IsHeading())
        effective_text_height =
          GetHeadingFont(first.style).GetLineSpacing();
    }

    const int text_y =
      y + (cur_line_height - effective_text_height) / 2;
    int x = padding;
    if (line.is_list_continuation) {
      x += line.list_hang_offset > 0 ? line.list_hang_offset
                                     : list_indent;
    } else if (!line.segments.empty()) {
      const TextSegment &first_seg = line.segments.front();
      if (first_seg.IsCheckbox() || first_seg.IsListItem())
        x += list_indent;
    }

    for (const TextSegment &seg : line.segments) {
      if (RenderInlineImage(canvas, seg, x, y,
                            cur_line_height, text_line_height))
        continue;

      const Font &seg_font = GetStyleFont(seg.style);
      canvas.Select(seg_font);

      if (seg.IsLink())
        /* Hit/focus height must match the painted font, not the
           body line spacing (bold/heading links are taller/wider). */
        RenderLinkSegment(canvas, seg, text_data,
                          x, text_y, seg_font.GetLineSpacing(),
                          y_origin);
      else if (seg.IsCheckbox())
        RenderCheckboxSegment(canvas, seg,
                              x, y, cur_line_height,
                              y_origin);
      else
        RenderPlainSegment(canvas, seg, text_data,
                           x, text_y);
    }
  }
}

void
RichTextWindow::EnsureContentCache(int origin,
                                   int viewport_h) noexcept
{
  if (viewport_h <= 0)
    return;

  const auto widget_size = GetSize();
  if (widget_size.width == 0)
    return;

  const int content_h = static_cast<int>(
    std::max(cached_content_height, widget_size.height));

  /* Three viewports of strip so prefetch has runway; capped for
     GL/memory. */
  static constexpr int MAX_CACHE_HEIGHT = 4096;
  int buf_h = std::max(viewport_h * 3, viewport_h + Layout::Scale(64));
  buf_h = std::min(buf_h, MAX_CACHE_HEIGHT);
  buf_h = std::min(buf_h, content_h);
  buf_h = std::max(buf_h, viewport_h);

  const PixelSize buf_size{widget_size.width,
                           static_cast<unsigned>(buf_h)};

  if (!content_cache.IsDefined() ||
      content_cache.GetSize() != buf_size) {
    content_cache.Create(buf_size);
    content_cache_dirty = true;
  }

  const int max_top = std::max(0, content_h - buf_h);
  const int margin = std::max(1, viewport_h / 4);
  const int cache_bottom = content_cache_top + buf_h;

  const bool outside =
    origin < content_cache_top ||
    origin + viewport_h > cache_bottom;
  /* Prefetch before the hard edge when more content exists that way. */
  const bool near_top =
    content_cache_top > 0 &&
    origin < content_cache_top + margin;
  const bool near_bottom =
    cache_bottom < content_h &&
    origin + viewport_h > cache_bottom - margin;

  if (!content_cache_dirty && !outside && !near_top && !near_bottom)
    return;

  /* Centre the strip on the visible origin when possible. */
  int new_top = origin - (buf_h - viewport_h) / 2;
  if (new_top < 0)
    new_top = 0;
  else if (new_top > max_top)
    new_top = max_top;

  if (!content_cache_dirty && new_top == content_cache_top)
    return;

  content_cache_top = new_top;
  content_hits.clear();

  content_cache.Begin();
  content_cache.Clear(background_color);
  PaintContent(content_cache, content_cache_top,
               content_cache_top, content_cache_top + buf_h);
  content_cache.End();
  content_cache_dirty = false;
}

void
RichTextWindow::PublishWindowHits(int origin,
                                  int viewport_h) noexcept
{
  ClearLinkRects();
  checkbox_rects.clear();

  const int visible_bottom = origin + viewport_h;
  for (const auto &hit : content_hits) {
    if (hit.content_rect.bottom <= origin ||
        hit.content_rect.top >= visible_bottom)
      continue;

    PixelRect window_rc = hit.content_rect;
    window_rc.Offset(0, -origin);
    if (hit.is_checkbox)
      checkbox_rects.push_back({window_rc, hit.index});
    else
      RegisterLinkRect(hit.index, window_rc);
  }
}

void
RichTextWindow::DrawFocusOverlay(Canvas &canvas,
                                 int origin) noexcept
{
  if (!HasFocus())
    return;

  canvas.SetBackgroundTransparent();

  for (const auto &hit : content_hits) {
    const bool focused = hit.is_checkbox
      ? IsCheckboxFocused(hit.index)
      : IsLinkFocused(hit.index);
    if (!focused)
      continue;

    PixelRect window_rc = hit.content_rect;
    window_rc.Offset(0, -origin);

    if (hit.is_checkbox) {
      const int box_size = CheckboxBoxSize(*font);
      /* Shrink touch-expanded hit back toward the box for drawing. */
      const int cb_expand = IsTouchLayout() ? Layout::Scale(6) : 0;
      PixelRect box_rc = window_rc;
      if (cb_expand > 0)
        box_rc.Grow(-cb_expand);
      /* Prefer exact box size if the hit was expanded. */
      if (box_rc.GetWidth() != unsigned(box_size) ||
          box_rc.GetHeight() != unsigned(box_size)) {
        box_rc.left = window_rc.left + cb_expand;
        box_rc.top = window_rc.top + cb_expand;
        box_rc.right = box_rc.left + box_size;
        box_rc.bottom = box_rc.top + box_size;
      }

      const bool checked = IsCheckboxChecked(hit.index);
      if (dialog_look != nullptr) {
        const unsigned focus_w = Layout::ScalePenWidth(2);
        PixelRect focus_rc = box_rc;
        focus_rc.Grow((int)focus_w);
        canvas.Select(Pen(focus_w, COLOR_XCSOAR_LIGHT));
        canvas.SelectHollowBrush();
        canvas.DrawRectangle(focus_rc);
        DrawCheckBox(canvas, *dialog_look, box_rc, checked, true,
                     false, true);
      } else
        DrawSimpleCheckbox(canvas, box_rc, checked, true, dark_mode);
    } else {
      /* Repaint this link segment only (wrapped links have one hit
         per visual line), using the same font as PaintContent. */
      if (font == nullptr || hit.text_length == 0 ||
          hit.text_start + hit.text_length > parsed.text.size())
        continue;

      std::string_view seg_text(parsed.text.c_str() + hit.text_start,
                                hit.text_length);
      const TextStyle style =
        GetStyleAt(parsed.styles, hit.text_start);
      const Font &seg_font = GetStyleFont(style);
      canvas.Select(seg_font);
      if (dark_mode)
        canvas.SetTextColor(COLOR_WHITE);
      else
        canvas.SetTextColor(COLOR_XCSOAR_LIGHT);

      const int touch_expand = IsTouchLayout() ? Layout::Scale(4) : 0;
      const int text_y = window_rc.top + touch_expand;
      canvas.DrawText({window_rc.left, text_y}, seg_text);

      /* Size the ring to the formatted glyph box, not the
         touch-expanded hit (which may use a taller line spacing). */
      const PixelSize text_size = canvas.CalcTextSize(seg_text);
      const int seg_width = static_cast<int>(text_size.width);
      const int seg_height = seg_font.GetLineSpacing();
      PixelRect focus_rc{window_rc.left, text_y,
                         window_rc.left + seg_width,
                         text_y + seg_height};
      focus_rc.Grow(Layout::ScalePenWidth(2));
      canvas.Select(Pen(Layout::ScalePenWidth(1),
                        dark_mode ? COLOR_LIGHT_GRAY
                                  : COLOR_DARK_GRAY));
      canvas.SelectHollowBrush();
      canvas.DrawRectangle(focus_rc);
    }
  }
}

void
RichTextWindow::OnPaint(Canvas &canvas) noexcept
{
  if (parsed.text.empty() || font == nullptr) {
    canvas.Clear(background_color);
    return;
  }

  EnsureSegmentedLines();
  EnsureLineLayout();
  SyncParentScrollHeight();

  int visible_top, visible_bottom, viewport_height;
  GetVisibleArea(visible_top, visible_bottom, viewport_height);
  (void)visible_bottom;

  if (!segmented_lines || segmented_lines->empty() ||
      line_y_offsets.empty()) {
    canvas.Clear(background_color);
    return;
  }

  EnsureContentCache(visible_top, viewport_height);

  if (!content_cache.IsDefined()) {
    canvas.Clear(background_color);
    return;
  }

  /* Pan: blit the visible strip from the content cache. */
  const int src_y = visible_top - content_cache_top;
  const PixelSize view_size = GetSize();
  const unsigned copy_h = std::min(view_size.height,
    content_cache.GetHeight() > unsigned(src_y)
      ? content_cache.GetHeight() - unsigned(src_y)
      : 0u);
  canvas.Clear(background_color);
  if (copy_h == 0)
    return;

  const PixelSize copy_size{view_size.width, copy_h};
  content_cache.CopyTo(canvas,
                       PixelRect{PixelPoint{0, 0}, copy_size},
                       PixelRect{PixelPoint{0, src_y}, copy_size});

  PublishWindowHits(visible_top, viewport_height);
  DrawFocusOverlay(canvas, visible_top);
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
  InvalidateContentCache();
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
RichTextWindow::ScrollToFocusItem(const FocusItem &item) noexcept
{
  if (item.is_checkbox) {
    focused_checkbox_style = item.index;
    focused_link.reset();
  } else {
    focused_link = item.index;
    focused_checkbox_style.reset();
  }

  /* #VScrollPanel::ScrollTo expects parent-client coordinates. */
  ContainerWindow *parent = GetParent();
  if (parent != nullptr) {
    const int parent_height = parent->GetSize().height;
    const int margin = Layout::Scale(20);

    /* Content Y → parent/viewport Y via scroll origin (child stays at
       y=0 when hosted in a viewport-sized VScrollPanel). */
    int origin = 0;
    if (const auto *panel = dynamic_cast<const VScrollPanel *>(parent))
      origin = static_cast<int>(panel->GetOrigin());
    else
      origin = -GetPosition().top;

    const int item_top = item.y_pos - origin;
    const int item_bottom = item.y_pos + item.height - origin;

    if (item_top < margin || item_bottom > parent_height - margin) {
      PixelRect scroll_rc{0, 0, 1, 1};
      if (item_top < margin) {
        scroll_rc.top = item_top - margin;
        scroll_rc.bottom = scroll_rc.top + 1;
      } else {
        scroll_rc.bottom = item_bottom + margin;
        scroll_rc.top = scroll_rc.bottom - 1;
      }
      parent->ScrollTo(scroll_rc);
    }
  }

  Invalidate();
}

[[gnu::pure]]
static bool
FocusItemVisible(const FocusItem &item,
                 int visible_top, int visible_bottom) noexcept
{
  return item.y_pos + item.height > visible_top &&
         item.y_pos < visible_bottom;
}

bool
RichTextWindow::OnKeyDown(unsigned key_code) noexcept
{
  EnsureSegmentedLines();
  EnsureLineLayout();
  SyncParentScrollHeight();

  if (!segmented_lines || segmented_lines->empty() ||
      line_y_offsets.empty())
    return LinkableWindow::OnKeyDown(key_code);

  const int padding = GetContentPadding();
  const auto items = BuildFocusItems(*this, *segmented_lines,
                                     line_y_offsets, line_heights,
                                     padding);
  if (items.empty())
    return LinkableWindow::OnKeyDown(key_code);

  int visible_top, visible_bottom, viewport_height;
  GetVisibleArea(visible_top, visible_bottom, viewport_height);
  (void)viewport_height;

  const auto current_pos =
    FindCurrentFocusIndex(items, focused_checkbox_style, focused_link);

  switch (key_code) {
  case KEY_DOWN:
    /* After PageDown (etc.) focus may sit on an off-screen item.
       Re-anchor to the first visible focusable before advancing. */
    if (!current_pos.has_value() ||
        !FocusItemVisible(items[current_pos.value()],
                          visible_top, visible_bottom)) {
      for (const auto &it : items) {
        if (FocusItemVisible(it, visible_top, visible_bottom)) {
          ScrollToFocusItem(it);
          return true;
        }
      }
      return false;
    }
    if (current_pos.value() + 1 < items.size()) {
      const auto &cur = items[current_pos.value()];
      const auto &next = items[current_pos.value() + 1];
      /* Move when the next item is on screen, or still within
         about one viewport.  If it is farther, return false so
         the parent scroller moves; once it enters the viewport
         the FocusItemVisible branch selects it. */
      if (FocusItemVisible(next, visible_top, visible_bottom) ||
          next.y_pos - cur.y_pos <= visible_bottom - visible_top) {
        ScrollToFocusItem(next);
        return true;
      }
    }
    return false;

  case KEY_UP:
    if (!current_pos.has_value() ||
        !FocusItemVisible(items[current_pos.value()],
                          visible_top, visible_bottom)) {
      for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if (FocusItemVisible(*it, visible_top, visible_bottom)) {
          ScrollToFocusItem(*it);
          return true;
        }
      }
      return false;
    }
    if (current_pos.value() > 0) {
      const auto &cur = items[current_pos.value()];
      const auto &prev = items[current_pos.value() - 1];
      if (FocusItemVisible(prev, visible_top, visible_bottom) ||
          cur.y_pos - prev.y_pos <= visible_bottom - visible_top) {
        ScrollToFocusItem(prev);
        return true;
      }
    }
    return false;

  case KEY_RETURN:
    if (focused_checkbox_style.has_value()) {
      ToggleCheckbox(focused_checkbox_style.value());
      /* Advance like Down when possible. */
      if (current_pos.has_value() &&
          current_pos.value() + 1 < items.size()) {
        const auto &next = items[current_pos.value() + 1];
        if (next.y_pos - items[current_pos.value()].y_pos <=
            visible_bottom - visible_top)
          ScrollToFocusItem(next);
      }
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
