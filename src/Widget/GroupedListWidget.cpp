// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GroupedListWidget.hpp"
#include "Asset.hpp"
#include "ui/window/TopWindow.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Hardware/CPU.hpp"
#include "Screen/Layout.hpp"
#include "UIUtil/KineticManager.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/control/ScrollBar.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/window/PaintWindow.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

/**
 * Can the user scroll with pixel precision?  Fast displays get
 * kinetic scrolling; e-paper and slow CPUs snap.
 */
[[gnu::pure]]
static bool
UseKineticScrolling() noexcept
{
  return !HasEPaper() && !IsSlowCPU();
}

/** Distance between the cards and the edges of the widget area. */
static constexpr unsigned MARGIN_PT = 10;

/** Corner radius of the card which holds the items of one group. */
static constexpr unsigned RADIUS_PT = 8;

/** Distance between a group and the group above it. */
static constexpr unsigned GROUP_GAP_PT = 15;

/** Distance between the caption of a group and its card. */
static constexpr unsigned CAPTION_GAP_PT = 4;

/**
 * Horizontal padding of the contents of a card, and the distance
 * between the value and the arrow at its right edge.  The same as the
 * distance the card itself keeps to the edges of the list.
 */
static constexpr unsigned PADDING_PT = MARGIN_PT;

/**
 * Additional distance which the arrow, the check mark, the badge and
 * the value keep to the edge of a card.  They are drawn to the pixel,
 * while a glyph carries some white space of its own; with the same
 * distance as the caption on the other side, they look closer to the
 * edge than they are.
 */
static constexpr unsigned EDGE_INSET_PT = 4;

/** Horizontal padding inside a badge; vertically, half of it. */
static constexpr unsigned BADGE_PADDING_PT = 4;

/** Corner radius of a badge. */
static constexpr unsigned BADGE_RADIUS_PT = 3;

/** Distance between the caption of an item and its second line. */
static constexpr unsigned SUBTITLE_GAP_PT = 2;

/** Distance between a card and the footer which explains it. */
static constexpr unsigned FOOTER_GAP_PT = 4;

/**
 * The renderer of the texts which may take more than one line, the
 * second line of an item and the description of a hero card.  It is
 * never configured, all of its settings are the default ones, and it
 * therefore needs no state of its own.
 */
static constexpr TextRenderer text_renderer;

/**
 * The scrollable contents of a #GroupedListWidget.
 *
 * Other than #ListControl, this gives every element its own height:
 * a caption or a footer is only as tall as its text plus the gaps
 * around it, while an item is as tall as a touch target.
 */
class GroupedListControl final : public PaintWindow {
public:
  using Callback = GroupedListWidget::Callback;
  using CursorCallback = GroupedListWidget::CursorCallback;
  using SelectionMode = GroupedListWidget::SelectionMode;
  using CheckPosition = GroupedListWidget::CheckPosition;
  using BadgeStyle = GroupedListWidget::BadgeStyle;
  using GroupOptions = GroupedListWidget::GroupOptions;

private:
  struct Element {
    enum class Type : uint_least8_t { HERO, CAPTION, FOOTER, ITEM };

    Type type;

    std::string text;

    /** only for Type::ITEM: a second line below #text; it may wrap */
    std::string subtitle{};

    /**
     * only for Type::ITEM: replaces the footer of the group while
     * the cursor is on this item
     */
    std::string help{};

    /**
     * only for Type::ITEM: drawn at the right edge; only for
     * Type::HERO: the description below the title
     */
    std::string value{};

    /** only for Type::ITEM: drawn in a rounded box */
    std::string badge{};

    /** only for Type::ITEM: the colors of #badge */
    BadgeStyle badge_style = BadgeStyle::PRIMARY;

    /** only for Type::ITEM */
    Callback callback{};

    /** only for Type::ITEM: draw an arrow at the right edge */
    bool chevron = false;

    /** only for Type::ITEM: draw a check mark */
    bool checked = false;

    /** only for Type::ITEM: greyed out, and the cursor skips it */
    bool disabled = false;

    /** only for Type::ITEM: may this item be checked, and how? */
    SelectionMode selection_mode = SelectionMode::NONE;

    /** only for Type::ITEM: the edge which holds the check mark */
    CheckPosition check_position = CheckPosition::RIGHT;

    /** the first / the last item of its group */
    bool first_in_group = false, last_in_group = false;

    /**
     * Does any item of this group have a check mark at this edge?
     * All items of the group then reserve the room for it, which
     * keeps their captions aligned.
     */
    bool check_left = false, check_right = false;

    /** the height of #subtitle, which may need more than one line */
    unsigned subtitle_height = 0;

    /** position and height within the virtual contents */
    int top = 0;
    unsigned height = 0;

    bool IsItem() const noexcept {
      return type == Type::ITEM;
    }

    /** may the cursor be moved to this element? */
    bool IsSelectable() const noexcept {
      return IsItem() && !disabled;
    }

    int GetBottom() const noexcept {
      return top + (int)height;
    }
  };

  const DialogLook &look;

  std::vector<Element> elements;

  /** the options of the group which the last AddGroup() call opened */
  GroupOptions group_options;

  ScrollBar scroll_bar;

  /** the height of all elements, including the margin below them */
  unsigned content_height = 0;

  /** the first visible pixel row of the virtual contents */
  int origin = 0;

  /** the selected item, or -1 if there is none */
  int cursor = -1;

  /**
   * The item index which Clear() has saved, to be restored by the
   * next UpdateLayout(); -1 if there is nothing to restore.
   */
  int saved_cursor = -1;

  CursorCallback cursor_callback;

  enum class DragMode : uint_least8_t { NONE, SCROLL, CURSOR };

  DragMode drag_mode = DragMode::NONE;

  /** the virtual pixel row which was grabbed */
  int drag_y = 0;

  /** the window pixel row where the drag started */
  int drag_start_y = 0;

  KineticManager kinetic;

  UI::PeriodicTimer kinetic_timer{[this]{ OnKineticTimer(); }};

public:
  explicit GroupedListControl(const DialogLook &_look) noexcept
    :look(_look), scroll_bar(_look.button) {}

  void Create(ContainerWindow &parent, const PixelRect &rc) noexcept;

  void AddHero(const char *title, const char *description) noexcept;
  void AddGroup(const char *caption, const GroupOptions &options) noexcept;
  void AddItem(const char *caption, Callback callback,
               const GroupedListWidget::ItemOptions &options) noexcept;

  /**
   * Remove all elements, but remember where the user was: a list
   * which refreshes itself while it is on the screen (a device list,
   * a WiFi scan) is cleared and filled again, and must not jump back
   * to the top each time.  The next UpdateLayout() restores the item
   * the cursor was on, and the scroll position follows the contents.
   */
  void Clear() noexcept {
    saved_cursor = GetCursorIndex();

    /* the options of the group which is open must not leak into the
       items of the next build */
    group_options = {};

    elements.clear();
    cursor = -1;
  }

  [[gnu::pure]]
  unsigned GetItemCount() const noexcept;

  void SetCursorCallback(CursorCallback callback) noexcept {
    cursor_callback = std::move(callback);
  }

  [[gnu::pure]]
  int GetCursorIndex() const noexcept;

  void SetItemChecked(unsigned i, bool checked) noexcept;

  [[gnu::pure]]
  bool IsItemChecked(unsigned i) const noexcept;

  /**
   * Append the footer of the group which is currently open, if it
   * needs one.  Called when the next group begins and before the
   * layout is calculated.
   */
  void FinishGroup() noexcept;

  /**
   * The text of a footer element: the help of the item the cursor
   * is on, or the footer of the group.
   */
  [[gnu::pure]]
  const char *GetFooterText(std::size_t i) const noexcept;

  /** Recalculate the layout and repaint. */
  void UpdateLayout() noexcept;

  [[gnu::pure]]
  unsigned GetItemHeight() const noexcept {
    return std::max(look.list.font->GetHeight() + 2 * Layout::GetTextPadding(),
                    Layout::GetMaximumControlHeight());
  }

  /**
   * Forward a key from #GroupedListWidget::KeyPress(), before the
   * dialog's normal key dispatch.
   */
  bool KeyFromWidget(unsigned key_code) noexcept {
    return OnKeyDown(key_code);
  }

private:
  [[gnu::pure]]
  int GetViewHeight() const noexcept {
    return (int)GetSize().height;
  }

  /** The width available for the cards, without the scroll bar. */
  [[gnu::pure]]
  int GetContentWidth() const noexcept {
    return (int)scroll_bar.GetLeft(GetSize());
  }

  /**
   * Look for an item, starting at @a i.
   *
   * @return the index, or -1 if there is none
   */
  [[gnu::pure]]
  int FindItem(int i, bool forward) const noexcept;

  [[gnu::pure]]
  int FindElementAt(int y) const noexcept;

  /**
   * Look up an item by its index, counting only items.
   *
   * @return the element index, or -1 if there is no such item
   */
  [[gnu::pure]]
  int FindItemByIndex(unsigned i) const noexcept;

  /** Check the given item and uncheck the others of its group. */
  void CheckOnly(std::size_t i) noexcept;

  /**
   * The width which the decorations at the edges of an item take away
   * from its caption.
   */
  [[gnu::pure]]
  int GetDecorationWidth(const Element &element) const noexcept;

  /**
   * The width of the column which holds a check mark, including the
   * padding which separates it from the text.
   */
  [[gnu::pure]]
  int GetCheckWidth() const noexcept {
    return GetCheckSize() + (int)Layout::VptScale(PADDING_PT);
  }

  /**
   * The gap above an element which begins a new group: the first
   * element keeps the same distance to the upper edge as the cards
   * keep to both sides.
   */
  [[gnu::pure]]
  static int GetLeadingGap(std::size_t i) noexcept {
    return Layout::VptScale(i == 0 ? MARGIN_PT : GROUP_GAP_PT);
  }

  /**
   * The thickness of the line which separates two items.  It is a
   * hair line: half of the thinnest pen, but at least one pixel.
   */
  [[gnu::pure]]
  static int GetSeparatorThickness() noexcept {
    return std::max(1u, Layout::ScalePenWidth(1) / 2);
  }

  /** The width and the height of the check mark itself. */
  [[gnu::pure]]
  int GetCheckSize() const noexcept {
    return std::max(4, (int)look.list.font->GetHeight() * 2 / 3);
  }

  static void DrawCheck(Canvas &canvas, PixelRect rc, Color color) noexcept;

  void SetOrigin(int _origin) noexcept;
  void EnsureVisible(unsigned i) noexcept;
  void SetCursor(int i) noexcept;
  void MoveCursor(bool forward) noexcept;
  void ActivateItem() noexcept;

  void UpdateGroupFlags() noexcept;

  /**
   * Paint the two corners of one edge with the color behind the
   * rectangle, which makes them appear rounded.
   *
   * @param top round the upper corners, else the lower ones
   */
  static void DrawRoundedEdge(Canvas &canvas, const PixelRect &rc,
                              bool top, Color color, int radius) noexcept;

  void DrawElement(Canvas &canvas, std::size_t i,
                   PixelRect rc) const noexcept;

  void DrawElements(Canvas &canvas) noexcept;
  void DrawScrollBar(Canvas &canvas) noexcept;

  void OnKineticTimer() noexcept;

protected:
  /* virtual methods from class Window */
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;
  void OnSetFocus() noexcept override;
  void OnKillFocus() noexcept override;
  void OnCancelMode() noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseWheel(PixelPoint p, int delta) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};

void
GroupedListControl::Create(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  PaintWindow::Create(parent, rc, style);
}

void
GroupedListControl::AddHero(const char *title,
                              const char *description) noexcept
{
  assert(title != nullptr);

  FinishGroup();

  elements.push_back(Element{
    .type = Element::Type::HERO,
    .text = title,
    .value = description != nullptr ? description : "",
  });
}

void
GroupedListControl::AddGroup(const char *caption,
                             const GroupOptions &options) noexcept
{
  FinishGroup();

  group_options = options;

  elements.push_back(Element{
    .type = Element::Type::CAPTION,
    .text = caption != nullptr ? caption : "",
  });
}

void
GroupedListControl::FinishGroup() noexcept
{
  if (elements.empty() || !elements.back().IsItem())
    /* no group is open, or its footer has been added already */
    return;

  const char *const footer = group_options.footer != nullptr
    ? group_options.footer
    : "";

  /* an item which explains itself needs a footer to be explained
     in, even if the group has no text of its own */
  bool needed = *footer != '\0';

  for (auto i = elements.rbegin(); i != elements.rend() && i->IsItem(); ++i)
    if (!i->help.empty())
      needed = true;

  if (!needed)
    return;

  elements.push_back(Element{
    .type = Element::Type::FOOTER,
    .text = footer,
  });
}

const char *
GroupedListControl::GetFooterText(std::size_t i) const noexcept
{
  assert(i < elements.size());
  assert(elements[i].type == Element::Type::FOOTER);

  /* the items of this group are the elements right above it */
  if (cursor >= 0 && (std::size_t)cursor < i &&
      !elements[cursor].help.empty()) {
    bool in_group = true;

    for (std::size_t j = i; j-- > (std::size_t)cursor;)
      if (!elements[j].IsItem()) {
        in_group = false;
        break;
      }

    if (in_group)
      return elements[cursor].help.c_str();
  }

  return elements[i].text.c_str();
}

/**
 * The two colors of a badge.  They are always defined as a pair: the
 * label sits on the filled box, and deriving one from the other would
 * break the contrast in one of the two themes.
 */
struct BadgeColors {
  Color background_color, text_color;
};

[[gnu::pure]]
static BadgeColors
GetBadgeColors(const DialogLook &look,
               GroupedListWidget::BadgeStyle style) noexcept
{
  const BadgeColors accent{look.list.focused.background_color,
                           look.list.focused.text_color};

  if (!HasColors())
    /* a monochrome display would turn all of them into the same grey */
    return accent;

  switch (style) {
  case GroupedListWidget::BadgeStyle::PRIMARY:
    break;

    /* the three shades below are yellow-500, red-600 and green-700
       of the Tailwind palette */

  case GroupedListWidget::BadgeStyle::WARNING:
    return {Color(0xea, 0xb3, 0x08), COLOR_WHITE};

  case GroupedListWidget::BadgeStyle::DANGER:
    return {Color(0xdc, 0x26, 0x26), COLOR_WHITE};

  case GroupedListWidget::BadgeStyle::SUCCESS:
    return {Color(0x15, 0x80, 0x3d), COLOR_WHITE};
  }

  return accent;
}

/**
 * An item which is not available always says so on its badge, and
 * replaces the badge which the caller has set: that a setting cannot
 * be reached at all matters more than the state it is in.
 */
[[gnu::pure]]
static const char *
GetBadge(const GroupedListWidget::ItemOptions &options) noexcept
{
  if (options.disabled)
    return options.disabled_badge_label != nullptr
      ? options.disabled_badge_label
      : _("Disabled");

  return options.badge != nullptr ? options.badge : "";
}

void
GroupedListControl::AddItem(const char *caption, Callback callback,
                            const GroupedListWidget::ItemOptions &options) noexcept
{
  assert(caption != nullptr);

  elements.push_back(Element{
    .type = Element::Type::ITEM,
    .text = caption,
    .subtitle = options.subtitle != nullptr ? options.subtitle : "",

    .value = options.value != nullptr ? options.value : "",
    .badge = GetBadge(options),
    .badge_style = options.badge_style,
    .callback = std::move(callback),
    .chevron = options.chevron,
    .checked = options.checked,
    .disabled = options.disabled,
    .selection_mode = group_options.selection_mode,
    .check_position = group_options.check_position,
  });

  elements.back().help = options.help != nullptr ? options.help : "";

}

unsigned
GroupedListControl::GetItemCount() const noexcept
{
  unsigned n = 0;
  for (const auto &element : elements)
    if (element.IsItem())
      ++n;
  return n;
}

int
GroupedListControl::GetCursorIndex() const noexcept
{
  if (cursor < 0)
    return -1;

  int n = 0;

  for (int j = 0; j < cursor; ++j)
    if (elements[j].IsItem())
      ++n;

  return n;
}

int
GroupedListControl::FindItemByIndex(unsigned i) const noexcept
{
  unsigned n = 0;

  for (std::size_t j = 0; j < elements.size(); ++j) {
    if (!elements[j].IsItem())
      continue;

    if (n == i)
      return j;

    ++n;
  }

  return -1;
}

void
GroupedListControl::CheckOnly(std::size_t i) noexcept
{
  assert(i < elements.size());
  assert(elements[i].IsItem());

  /* the items of a group are a run of neighbours; the flags of the
     group say the same, but they are only maintained for the items
     which are drawn */
  std::size_t begin = i;
  while (begin > 0 && elements[begin - 1].IsItem())
    --begin;

  for (std::size_t j = begin; j < elements.size() && elements[j].IsItem(); ++j)
    elements[j].checked = j == i;
}

void
GroupedListControl::SetItemChecked(unsigned i, bool checked) noexcept
{
  const int j = FindItemByIndex(i);
  if (j < 0)
    return;

  if (checked && elements[j].selection_mode == SelectionMode::SINGLE)
    CheckOnly(j);
  else
    elements[j].checked = checked;

  Invalidate();
}

bool
GroupedListControl::IsItemChecked(unsigned i) const noexcept
{
  const int j = FindItemByIndex(i);
  return j >= 0 && elements[j].checked;
}

int
GroupedListControl::FindItem(int i, bool forward) const noexcept
{
  const int n = elements.size();

  if (forward) {
    for (; i < n; ++i)
      if (i >= 0 && elements[i].IsSelectable())
        return i;
  } else {
    if (i > n)
      i = n;

    while (i-- > 0)
      if (elements[i].IsSelectable())
        return i;
  }

  return -1;
}

int
GroupedListControl::FindElementAt(int y) const noexcept
{
  const int virtual_y = y + origin;

  for (std::size_t i = 0; i < elements.size(); ++i)
    if (virtual_y >= elements[i].top && virtual_y < elements[i].GetBottom())
      return i;

  return -1;
}

void
GroupedListControl::UpdateGroupFlags() noexcept
{
  for (std::size_t i = 0; i < elements.size(); ++i) {
    Element &element = elements[i];
    if (!element.IsItem())
      continue;

    element.first_in_group = i == 0 || !elements[i - 1].IsItem();
    element.last_in_group = i + 1 == elements.size() ||
      !elements[i + 1].IsItem();
  }

  /* find the groups which have check marks; all their items reserve
     the room for one, even those which cannot be checked */
  for (std::size_t i = 0; i < elements.size();) {
    if (!elements[i].IsItem()) {
      ++i;
      continue;
    }

    std::size_t end = i;
    bool left = false, right = false;

    do {
      const Element &element = elements[end];

      if (element.selection_mode != SelectionMode::NONE) {
        if (element.check_position == CheckPosition::LEFT)
          left = true;
        else
          right = true;
      }

      ++end;
    } while (end < elements.size() && elements[end].IsItem());

    for (std::size_t j = i; j < end; ++j) {
      elements[j].check_left = left;
      elements[j].check_right = right;
    }

    i = end;
  }
}

int
GroupedListControl::GetDecorationWidth(const Element &element) const noexcept
{
  /* this mirrors DrawElement(), which lays the decorations out while
     it draws them */

  const int padding = Layout::VptScale(PADDING_PT);
  const Font &font = *look.list.font;

  int width = Layout::VptScale(EDGE_INSET_PT);

  if (element.check_left)
    width += Layout::VptScale(EDGE_INSET_PT) + GetCheckWidth();

  if (element.check_right)
    width += GetCheckWidth();

  if (element.chevron && !element.disabled)
    width += std::max(2, (int)font.GetHeight() / 4) + padding;

  if (!element.value.empty() && !element.disabled)
    width += (int)font.TextSize(element.value).width + padding;

  if (!element.badge.empty())
    width += (int)font.TextSize(element.badge).width
      + 2 * (int)Layout::VptScale(BADGE_PADDING_PT) + padding;

  return width;
}

void
GroupedListControl::UpdateLayout() noexcept
{
  FinishGroup();
  UpdateGroupFlags();

  if (!IsDefined())
    return;

  /* the footer of a group shows the help of the item under the
     cursor, and the height of the footer depends on that text:
     restore the cursor before the elements are measured */
  if (cursor < 0 && saved_cursor >= 0) {
    /* the same item as before the list was rebuilt, or its neighbour
       if the list has become shorter or the item cannot be selected
       any more */
    cursor = FindItemByIndex(saved_cursor);

    if (cursor < 0)
      cursor = FindItem(elements.size(), false);
    else if (!elements[cursor].IsSelectable())
      cursor = FindItem(cursor, true);
  }

  saved_cursor = -1;

  if (cursor < 0)
    cursor = FindItem(0, true);

  const int margin = Layout::VptScale(MARGIN_PT);
  const int caption_gap = Layout::VptScale(CAPTION_GAP_PT);
  const int footer_gap = Layout::VptScale(FOOTER_GAP_PT);

  const int padding = Layout::VptScale(PADDING_PT);

  const unsigned item_height = GetItemHeight();
  const unsigned subtitle_gap = Layout::VptScale(SUBTITLE_GAP_PT);
  const unsigned caption_height = look.list.font_bold->GetHeight();

  /* the width available for a footer depends on the scroll bar, and
     whether the scroll bar is needed depends on the height of the
     footers; lay out without it first, and once more with it */
  scroll_bar.Reset();

  for (unsigned pass = 0; pass < 2; ++pass) {
    const int text_width = GetContentWidth() - 2 * margin - 2 * padding;

    int y = 0;

    for (std::size_t i = 0; i < elements.size(); ++i) {
      Element &element = elements[i];

      switch (element.type) {
      case Element::Type::ITEM:
        if (element.subtitle.empty()) {
          element.height = item_height;
          break;
        }

        /* the second line wraps inside the same column as the
           caption, and the item grows with it */
        element.subtitle_height =
          text_renderer.GetHeight(look.small_font,
                                      std::max(text_width
                                               - GetDecorationWidth(element),
                                               1),
                                      element.subtitle.c_str());

        {
          const unsigned subtitle_lines =
            element.subtitle_height / look.small_font.GetLineSpacing();

          /* the room above and below grows with the number of lines:
             with the padding of a one-line item, a tall item would
             sit cramped between the two separators.  It never grows
             beyond the room which a one-line item has, where the
             minimum height of a touch target is what pads the
             caption */
          const unsigned vertical_padding =
            std::min(Layout::GetTextPadding() * (1 + subtitle_lines),
                     (item_height - look.list.font->GetHeight()) / 2);

          element.height = std::max(look.list.font->GetHeight()
                                    + subtitle_gap + element.subtitle_height
                                    + 2 * vertical_padding,
                                    Layout::GetMaximumControlHeight());
        }

        break;

      case Element::Type::HERO:
        element.height = GetLeadingGap(i)
          + 2 * padding + look.heading2_font.GetHeight();

        if (!element.value.empty())
          element.height += caption_gap +
            text_renderer.GetHeight(*look.list.font,
                                        std::max(text_width, 1),
                                        element.value.c_str());
        break;

      case Element::Type::CAPTION:
        element.height = GetLeadingGap(i)
          + (element.text.empty() ? 0u : caption_height + caption_gap);
        break;

      case Element::Type::FOOTER:
        element.height = footer_gap +
          text_renderer.GetHeight(*look.list.font,
                                      std::max(text_width, 1),
                                      GetFooterText(i));
        break;
      }

      element.top = y;
      y += (int)element.height;
    }

    content_height = elements.empty() ? 0 : (unsigned)(y + margin);

    if (scroll_bar.IsDefined() ||
        content_height <= (unsigned)GetViewHeight())
      break;

    scroll_bar.SetSize(GetSize());
  }

  SetOrigin(origin);

  Invalidate();
}

void
GroupedListControl::SetOrigin(int _origin) noexcept
{
  const int max_origin = std::max(0, (int)content_height - GetViewHeight());
  _origin = std::clamp(_origin, 0, max_origin);

  if (_origin == origin)
    return;

  origin = _origin;
  Invalidate();
}

void
GroupedListControl::EnsureVisible(unsigned i) noexcept
{
  assert(i < elements.size());

  const Element &element = elements[i];

  if (element.top < origin)
    SetOrigin(element.top);
  else if (element.GetBottom() > origin + GetViewHeight())
    SetOrigin(element.GetBottom() - GetViewHeight());
}

void
GroupedListControl::SetCursor(int i) noexcept
{
  if (i < 0 || i == cursor)
    return;

  const int previous = cursor;

  cursor = i;

  /* the footer of a group shows the help of the item under the
     cursor, and its height changes with that text; measuring the
     whole list again is only needed when such a text comes or goes */
  if ((previous >= 0 && (std::size_t)previous < elements.size() &&
       !elements[previous].help.empty()) ||
      !elements[i].help.empty())
    UpdateLayout();
  else
    Invalidate();

  EnsureVisible(i);

  if (cursor_callback)
    cursor_callback(GetCursorIndex());
}

void
GroupedListControl::MoveCursor(bool forward) noexcept
{
  const int next = FindItem(forward ? cursor + 1 : cursor, forward);
  if (next >= 0)
    SetCursor(next);
}

void
GroupedListControl::ActivateItem() noexcept
{
  if (cursor < 0 || (std::size_t)cursor >= elements.size())
    return;

  Element &element = elements[cursor];
  if (element.disabled)
    return;

  switch (element.selection_mode) {
  case SelectionMode::NONE:
    break;

  case SelectionMode::SINGLE:
    /* like a radio button: tapping the checked item keeps it checked */
    CheckOnly(cursor);
    Invalidate();
    break;

  case SelectionMode::MULTIPLE:
    element.checked = !element.checked;
    Invalidate();
    break;
  }

  /* the item under the finger is drawn pressed, and the release
     does not reach the screen before the next repaint: show it now,
     because the callback may take a while.  The window this list
     lives in knows the way up; a list outside a top window simply
     does not repaint early */
  if (auto *top = dynamic_cast<UI::TopWindow *>(GetRootOwner()))
    top->Refresh();

  /* the callback may open another dialog; invoke it after the check
     mark has been updated; it is a copy because the callback may
     rebuild the list and destroy the element */
  if (auto callback = element.callback)
    callback();
}

/**
 * The two colors of an item.
 */
struct RowColors {
  Color background_color, text_color;
};

/**
 * The colors of an item, with one exception: the pressed state does
 * not use the yellow of #DialogLook, which was chosen for the lists
 * of the other dialogs and is much too loud between the quiet cards
 * of this one.
 */
[[gnu::pure]]
static RowColors
GetRowColors(const DialogLook &look, bool selected, bool focused,
             bool pressed) noexcept
{
  if (pressed && HasColors())
    /* zinc-200 and zinc-700 of the Tailwind palette; a phone dims the
       row under the finger instead of coloring it */
    return {look.dark_mode
            ? Color(0x3f, 0x3f, 0x46)
            : Color(0xe4, 0xe4, 0xe7),
            look.list.text_color};

  return {look.list.GetBackgroundColor(selected, focused, pressed),
          look.list.GetTextColor(selected, focused, pressed)};
}

void
GroupedListControl::DrawRoundedEdge(Canvas &canvas, const PixelRect &rc,
                                    bool top, Color color,
                                    int radius) noexcept
{
  for (int i = 0; i < radius; ++i) {
    /* the horizontal distance between the corner of the rectangle and
       the arc, in the middle of this pixel row */
    const double dy = radius - i - 0.5;
    const int dx = (int)std::lround(radius -
                                    std::sqrt(radius * radius - dy * dy));
    if (dx <= 0)
      continue;

    const int y = top ? rc.top + i : rc.bottom - 1 - i;

    canvas.DrawFilledRectangle({rc.left, y, rc.left + dx, y + 1}, color);
    canvas.DrawFilledRectangle({rc.right - dx, y, rc.right, y + 1}, color);
  }
}

void
GroupedListControl::DrawCheck(Canvas &canvas, PixelRect rc,
                              Color color) noexcept
{
  /* a check mark, drawn as two strokes: a short one down to the lower
     left corner, and a long one up to the upper right corner */
  const Pen pen(std::max(2u, Layout::ScalePenWidth(2)), color);
  canvas.Select(pen);

  const int left = rc.left;
  const int right = rc.right - 1;
  const int bottom = rc.bottom - 1;

  /* the corner where the two strokes meet */
  const PixelPoint corner{left + (int)rc.GetWidth() / 3, bottom};

  canvas.DrawLine({left, bottom - (int)rc.GetHeight() / 2}, corner);
  canvas.DrawLine(corner, {right, rc.top});
}

void
GroupedListControl::DrawElement(Canvas &canvas, std::size_t i,
                                PixelRect rc) const noexcept
{
  const Element &element = elements[i];

  const int margin = Layout::VptScale(MARGIN_PT);
  const int padding = Layout::VptScale(PADDING_PT);

  rc.left += margin;
  rc.right -= margin;

  /* captions, footers and items all put their text into the same
     column */
  PixelRect text_rc = rc;
  text_rc.left += padding;
  text_rc.right -= padding;

  switch (element.type) {
  case Element::Type::ITEM: {
    const bool selected = (int)i == cursor;
    const bool pressed = selected && drag_mode == DragMode::CURSOR;
    const bool focused = !HasCursorKeys() || HasFocus();

    const RowColors row_colors = GetRowColors(look, selected, focused,
                                              pressed);
    const Color background = row_colors.background_color;

    /* the thin line above this item belongs to the item above it, and
       it must not cut into the selected item: swallow it, so that the
       background covers it.  The elements are drawn from top to
       bottom, and therefore the line is already there */
    const int separator = GetSeparatorThickness();

    PixelRect background_rc = rc;
    if (selected && !element.first_in_group)
      background_rc.top -= separator;

    canvas.DrawFilledRectangle(background_rc, background);

    const int radius = Layout::VptScale(RADIUS_PT);

    if (element.first_in_group)
      DrawRoundedEdge(canvas, rc, true, look.background_color, radius);

    if (element.last_in_group)
      DrawRoundedEdge(canvas, rc, false, look.background_color, radius);

    /* a thin line separates the items of a card; like the gap between
       two cards, it shows the page behind them.  The selected item
       needs no line, its background already separates it from its
       neighbours */
    if (!element.last_in_group && !selected)
      canvas.DrawFilledRectangle({text_rc.left, rc.bottom - separator,
                                  text_rc.right, rc.bottom},
                                 look.background_color);

    canvas.Select(*look.list.font);

    const Color plain_text_color = row_colors.text_color;

    /* an item which is not available keeps its background, but all of
       its contents fade towards it */
    const Color text_color = element.disabled
      ? (look.dark_mode
         ? DarkColor(plain_text_color)
         : LightColor(plain_text_color))
      : plain_text_color;

    canvas.SetTextColor(text_color);

    const int font_height = look.list.font->GetHeight();
    const int centre_y = text_rc.top + (int)text_rc.GetHeight() / 2;

    int text_y = text_rc.top
      + ((int)text_rc.GetHeight() - font_height) / 2;
    int subtitle_y = 0;

    if (!element.subtitle.empty()) {
      /* the caption and its second line are centred as one block */
      const int gap = Layout::VptScale(SUBTITLE_GAP_PT);
      const int block_height = font_height + gap
        + (int)element.subtitle_height;

      text_y = text_rc.top + ((int)text_rc.GetHeight() - block_height) / 2;
      subtitle_y = text_y + font_height + gap;
    }

    /* the caption uses the room which is left of the check mark, the
       arrow, the value and the badge */
    PixelRect caption_rc = text_rc;

    const int edge_inset = Layout::VptScale(EDGE_INSET_PT);

    caption_rc.right -= edge_inset;

    if (element.check_left)
      caption_rc.left += edge_inset;

    if (element.check_left || element.check_right) {
      const int size = GetCheckSize();
      const bool left = element.check_position == CheckPosition::LEFT;

      if (element.checked && !element.disabled) {
        PixelRect check_rc;
        check_rc.left = left
          ? caption_rc.left
          : caption_rc.right - size;
        check_rc.right = check_rc.left + size;
        check_rc.top = centre_y - size / 2;
        check_rc.bottom = check_rc.top + size;

        DrawCheck(canvas, check_rc, text_color);
      }

      if (element.check_left)
        caption_rc.left += GetCheckWidth();

      if (element.check_right)
        caption_rc.right -= GetCheckWidth();
    }

    /* an item which is not available shows neither the arrow nor its
       value: the arrow would promise a page which does not open, and
       the state is not in effect anyway.  What is left is the caption
       and the badge which says why */
    if (element.chevron && !element.disabled) {
      const int size = std::max(2, font_height / 4);

      const Pen pen(Layout::ScalePenWidth(1), text_color);
      canvas.Select(pen);
      canvas.DrawLine({caption_rc.right - size, centre_y - size},
                      {caption_rc.right, centre_y});
      canvas.DrawLine({caption_rc.right, centre_y},
                      {caption_rc.right - size, centre_y + size});

      caption_rc.right -= size + padding;
    }

    if (!element.value.empty() && !element.disabled) {
      const int width = canvas.CalcTextWidth(element.value.c_str());

      /* the value sits in the middle of the item, like the badge and
         the arrow, even when the caption has been pushed up by a
         second line */
      canvas.DrawClippedText({caption_rc.right - width,
                              centre_y - font_height / 2},
                             caption_rc, element.value.c_str());

      caption_rc.right -= width + padding;
    }

    if (!element.badge.empty()) {
      /* a short label on a filled rounded box; on the selected item
         the colors are swapped, where the accent color is the
         background of the item itself.  On an item which is not
         available, the box is grey, like its text */
      const int badge_pad_x = Layout::VptScale(BADGE_PADDING_PT);
      const int badge_pad_y = badge_pad_x / 2;
      const int badge_height = font_height + 2 * badge_pad_y;

      PixelRect badge_rc;
      badge_rc.right = caption_rc.right;
      badge_rc.left = badge_rc.right
        - canvas.CalcTextWidth(element.badge.c_str()) - 2 * badge_pad_x;
      badge_rc.top = centre_y - badge_height / 2;
      badge_rc.bottom = badge_rc.top + badge_height;

      const BadgeColors badge_colors = GetBadgeColors(look,
                                                      element.badge_style);

      canvas.DrawFilledRectangle(badge_rc, selected || element.disabled
                                 ? text_color
                                 : badge_colors.background_color);

      const int badge_radius = std::min((int)Layout::VptScale(BADGE_RADIUS_PT),
                                        badge_height / 2);
      DrawRoundedEdge(canvas, badge_rc, true, background, badge_radius);
      DrawRoundedEdge(canvas, badge_rc, false, background, badge_radius);

      canvas.SetTextColor(selected || element.disabled
                          ? background
                          : badge_colors.text_color);

      /* center the capitals within the box, not the font box: its
         descent is empty for a short label and would push the text
         down */
      const int badge_text_y = centre_y
        - (int)look.list.font->GetAscentHeight()
        + (int)look.list.font->GetCapitalHeight() / 2;

      canvas.DrawClippedText({badge_rc.left + badge_pad_x, badge_text_y},
                             badge_rc, element.badge.c_str());

      caption_rc.right = badge_rc.left - padding;

      canvas.SetTextColor(text_color);
    }

    canvas.DrawClippedText({caption_rc.left, text_y}, caption_rc,
                           element.text.c_str());

    if (!element.subtitle.empty()) {
      canvas.Select(look.small_font);

      PixelRect subtitle_rc = caption_rc;
      subtitle_rc.top = subtitle_y;
      subtitle_rc.bottom = subtitle_y + (int)element.subtitle_height;

      text_renderer.Draw(canvas, subtitle_rc, element.subtitle.c_str());
    }

    break;
  }

  case Element::Type::HERO: {
    /* a hero card which introduces the page or a part of it, with a
       title and an optional description below it */
    PixelRect card_rc = rc;
    card_rc.top += GetLeadingGap(i);

    canvas.DrawFilledRectangle(card_rc,
                               look.list.GetBackgroundColor(false, false,
                                                            false));

    const int radius = Layout::VptScale(RADIUS_PT);
    DrawRoundedEdge(canvas, card_rc, true, look.background_color, radius);
    DrawRoundedEdge(canvas, card_rc, false, look.background_color, radius);

    text_rc.top = card_rc.top + padding;
    text_rc.bottom = card_rc.bottom - padding;

    const Color title_color = look.list.GetTextColor(false, false, false);

    canvas.Select(look.heading2_font);
    canvas.SetTextColor(title_color);
    canvas.DrawClippedText({text_rc.left, text_rc.top}, text_rc,
                           element.text.c_str());

    if (!element.value.empty()) {
      text_rc.top += (int)look.heading2_font.GetHeight()
        + (int)Layout::VptScale(CAPTION_GAP_PT);

      canvas.Select(*look.list.font);
      canvas.SetTextColor(title_color);

      text_renderer.Draw(canvas, text_rc, element.value.c_str());
    }

    break;
  }

  case Element::Type::CAPTION:
    if (element.text.empty())
      break;

    canvas.Select(*look.list.font_bold);
    canvas.SetTextColor(look.text_color);
    canvas.DrawClippedText({text_rc.left,
                            text_rc.bottom
                            - (int)Layout::VptScale(CAPTION_GAP_PT)
                            - (int)look.list.font_bold->GetHeight()},
                           text_rc, element.text.c_str());
    break;

  case Element::Type::FOOTER:
    canvas.Select(*look.list.font);
    canvas.SetTextColor(look.text_color);

    text_rc.top += Layout::VptScale(FOOTER_GAP_PT);
    text_renderer.Draw(canvas, text_rc, GetFooterText(i));
    break;
  }
}

void
GroupedListControl::DrawScrollBar(Canvas &canvas) noexcept
{
  if (!scroll_bar.IsDefined())
    return;

  scroll_bar.SetSlider(content_height, GetViewHeight(), origin);
  scroll_bar.Paint(canvas);
}

void
GroupedListControl::DrawElements(Canvas &canvas) noexcept
{
  const int right = GetContentWidth();
  const int bottom = (int)canvas.GetHeight();

#ifdef ENABLE_OPENGL
  /* the first and the last visible element reach beyond this window;
     without clipping they would be drawn onto the dialog */
  const GLCanvasScissor scissor(PixelRect{0, 0, right, bottom});
#endif

  for (std::size_t i = 0; i < elements.size(); ++i) {
    const Element &element = elements[i];
    const int top = element.top - origin;
    if (top >= bottom)
      break;

    if (element.GetBottom() - origin <= 0)
      continue;

    DrawElement(canvas, i,
                PixelRect{0, top, right, top + (int)element.height});
  }
}

void
GroupedListControl::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(look.background_color);
  canvas.SetBackgroundTransparent();

  /* the scissor of DrawElements() must be gone before the scroll bar
     is painted, it reaches beyond the elements */
  DrawElements(canvas);

  DrawScrollBar(canvas);
}

void
GroupedListControl::OnKineticTimer() noexcept
{
  if (kinetic.IsSteady()) {
    kinetic_timer.Cancel();
    return;
  }

  const int position = kinetic.GetPosition();
  SetOrigin(position);

  if (origin != position)
    /* the end of the list has been reached */
    kinetic_timer.Cancel();
}

void
GroupedListControl::OnDestroy() noexcept
{
  kinetic_timer.Cancel();

  PaintWindow::OnDestroy();
}

void
GroupedListControl::OnResize(PixelSize new_size) noexcept
{
  PaintWindow::OnResize(new_size);

  if (scroll_bar.IsDefined())
    scroll_bar.SetSize(new_size);

  UpdateLayout();
}

void
GroupedListControl::OnSetFocus() noexcept
{
  PaintWindow::OnSetFocus();
  Invalidate();
}

void
GroupedListControl::OnKillFocus() noexcept
{
  PaintWindow::OnKillFocus();
  Invalidate();
}

void
GroupedListControl::OnCancelMode() noexcept
{
  kinetic_timer.Cancel();

  if (drag_mode != DragMode::NONE) {
    drag_mode = DragMode::NONE;
    ReleaseCapture();
    Invalidate();
  }

  scroll_bar.DragEnd(this);

  PaintWindow::OnCancelMode();
}

bool
GroupedListControl::OnMouseDown(PixelPoint p) noexcept
{
  SetFocus();

  if (scroll_bar.IsInside(p)) {
    if (scroll_bar.IsInsideSlider(p)) {
      scroll_bar.DragBegin(this, p.y);
    } else if (scroll_bar.IsInsideUpArrow(p.y))
      SetOrigin(origin - (int)GetItemHeight());
    else if (scroll_bar.IsInsideDownArrow(p.y))
      SetOrigin(origin + (int)GetItemHeight());
    else if (scroll_bar.IsAboveSlider(p.y))
      SetOrigin(origin - GetViewHeight());
    else if (scroll_bar.IsBelowSlider(p.y))
      SetOrigin(origin + GetViewHeight());

    return true;
  }

  kinetic_timer.Cancel();

  drag_y = origin + p.y;
  drag_start_y = p.y;

  if (UseKineticScrolling())
    kinetic.MouseDown(origin);

  const int i = FindElementAt(p.y);
  if (i >= 0 && elements[i].IsSelectable()) {
    SetCursor(i);
    drag_mode = DragMode::CURSOR;
    Invalidate();
  } else
    drag_mode = DragMode::SCROLL;

  SetCapture();
  return true;
}

bool
GroupedListControl::OnMouseMove(PixelPoint p,
                                [[maybe_unused]] unsigned keys) noexcept
{
  if (scroll_bar.IsDragging()) {
    SetOrigin(scroll_bar.DragMove(content_height, GetViewHeight(), p.y));
    return true;
  }

  if (drag_mode == DragMode::NONE)
    return false;

  if (drag_mode == DragMode::CURSOR &&
      std::abs(p.y - drag_start_y) > Layout::Scale(8)) {
    /* the finger has moved too far: this is a scroll gesture, not a
       tap on an item */
    drag_mode = DragMode::SCROLL;
    Invalidate();
  }

  if (drag_mode == DragMode::SCROLL) {
    SetOrigin(drag_y - p.y);

    if (UseKineticScrolling())
      kinetic.MouseMove(origin);
  }

  return true;
}

bool
GroupedListControl::OnMouseUp(PixelPoint p) noexcept
{
  if (scroll_bar.IsDragging()) {
    scroll_bar.DragEnd(this);
    return true;
  }

  /* the press has moved the cursor to the item; asking which element
     is under the finger now would miss it, because the footer of
     another group may have grown or shrunk in between and moved the
     item away.  A tap which has not wandered activates the item the
     press has chosen */
  const bool activate = drag_mode == DragMode::CURSOR &&
    std::abs(p.y - drag_start_y) < Layout::Scale(8);

  const bool coast = drag_mode == DragMode::SCROLL && UseKineticScrolling();

  if (drag_mode != DragMode::NONE) {
    drag_mode = DragMode::NONE;
    ReleaseCapture();
    Invalidate();
  }

  if (coast) {
    kinetic.MouseUp(origin);
    kinetic_timer.Schedule(std::chrono::milliseconds(30));
  }

  if (activate)
    ActivateItem();

  return true;
}

bool
GroupedListControl::OnMouseWheel([[maybe_unused]] PixelPoint p,
                                 int delta) noexcept
{
  kinetic_timer.Cancel();

  SetOrigin(origin - delta * (int)GetItemHeight());
  return true;
}

bool
GroupedListControl::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_RETURN:
    return cursor >= 0;

  case KEY_UP:
    return FindItem(cursor, false) >= 0;

  case KEY_DOWN:
    return FindItem(cursor + 1, true) >= 0;

  default:
    return false;
  }
}

bool
GroupedListControl::OnKeyDown(unsigned key_code) noexcept
{
  kinetic_timer.Cancel();

  switch (key_code) {
  case KEY_RETURN:
    ActivateItem();
    return true;

  case KEY_UP:
    if (FindItem(cursor, false) < 0)
      break;

    MoveCursor(false);
    return true;

  case KEY_DOWN:
    if (FindItem(cursor + 1, true) < 0)
      break;

    MoveCursor(true);
    return true;

  case KEY_HOME:
    SetCursor(FindItem(0, true));
    return true;

  case KEY_END:
    SetCursor(FindItem(elements.size(), false));
    return true;

  case KEY_PRIOR:
    SetOrigin(origin - GetViewHeight());
    return true;

  case KEY_NEXT:
    SetOrigin(origin + GetViewHeight());
    return true;
  }

  return PaintWindow::OnKeyDown(key_code);
}

GroupedListWidget::GroupedListWidget(const DialogLook &look) noexcept
  :pending(std::make_unique<GroupedListControl>(look)),
   control(*pending) {}

GroupedListWidget::~GroupedListWidget() noexcept = default;

void
GroupedListWidget::AddHero(const char *title,
                             const char *description) noexcept
{
  control.AddHero(title, description);
}

void
GroupedListWidget::AddGroup(const char *caption) noexcept
{
  control.AddGroup(caption, GroupOptions{});
}

void
GroupedListWidget::AddGroup(const char *caption,
                            const GroupOptions &options) noexcept
{
  control.AddGroup(caption, options);
}

void
GroupedListWidget::AddItem(const char *caption, Callback callback) noexcept
{
  control.AddItem(caption, std::move(callback), ItemOptions{});
}

void
GroupedListWidget::AddItem(const char *caption, Callback callback,
                           const ItemOptions &options) noexcept
{
  control.AddItem(caption, std::move(callback), options);
}

void
GroupedListWidget::AddItem(const char *caption,
                           const ItemOptions &options) noexcept
{
  control.AddItem(caption, Callback{}, options);
}

void
GroupedListWidget::Clear() noexcept
{
  control.Clear();
}

unsigned
GroupedListWidget::GetItemCount() const noexcept
{
  return control.GetItemCount();
}

void
GroupedListWidget::SetCursorCallback(CursorCallback callback) noexcept
{
  control.SetCursorCallback(std::move(callback));
}

int
GroupedListWidget::GetCursorIndex() const noexcept
{
  return control.GetCursorIndex();
}

void
GroupedListWidget::SetItemChecked(unsigned i, bool checked) noexcept
{
  control.SetItemChecked(i, checked);
}

bool
GroupedListWidget::IsItemChecked(unsigned i) const noexcept
{
  return control.IsItemChecked(i);
}

void
GroupedListWidget::UpdateLayout() noexcept
{
  control.UpdateLayout();
}

PixelSize
GroupedListWidget::GetMinimumSize() const noexcept
{
  return { Layout::Scale(200u), 2u * control.GetItemHeight() };
}

PixelSize
GroupedListWidget::GetMaximumSize() const noexcept
{
  return { 4096, 4096 };
}

void
GroupedListWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  assert(pending);

  pending->Create(parent, rc);
  SetWindow(std::move(pending));

  UpdateLayout();
}

bool
GroupedListWidget::KeyPress(unsigned key_code) noexcept
{
  if (key_code != KEY_UP && key_code != KEY_DOWN)
    return false;

  /* only when the list has the focus: Up/Down must reach a filter row
     or another focused control otherwise */
  if (!IsDefined() || !control.HasFocus())
    return false;

  /* route this before #WndForm maps Up/Down to focus movement; when
     no item is left, return false so that the dialog can move the
     focus to another control */
  return control.KeyFromWidget(key_code);
}
