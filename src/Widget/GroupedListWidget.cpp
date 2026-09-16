// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GroupedListWidget.hpp"
#include "Asset.hpp"
#include "ui/window/TopWindow.hpp"
#include "Language/Language.hpp"
#include "Look/Colors.hpp"
#include "system/OpenLink.hpp"
#include "util/MarkdownParser.hpp"
#include "ui/canvas/TextWrapper.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Hardware/CPU.hpp"
#include "Screen/Layout.hpp"
#include "UIUtil/KineticManager.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/ColorGlyph.hpp"
#include "ui/canvas/Icon.hpp"
#include "util/UTF8.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/control/ScrollBar.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/Button.hpp"
#include "Renderer/TextButtonRenderer.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
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

/**
 * A view this narrow lets its cards fill it: the margin beside them,
 * and the rounded corners which go with it, are worth more as room
 * for the text where there is little of it.
 *
 * The first number is the one which counts on a display of the usual
 * kind, where a view of 400 pixels is a small device and 640x480 is
 * not.  The second one belongs to a screen which is small in the
 * hand, and it is larger, because such a display often has many dots
 * per inch: XCSoar started with -small on a retina display draws
 * into 640x480 pixels, which is not a lot of room once everything on
 * it is twice as large, while a phone of the same size in the hand
 * has half again as many pixels and can afford the margin.
 *
 * @see Layout::small_screen
 */
static constexpr unsigned NARROW_VIEW_PIXELS = 400;
static constexpr unsigned NARROW_SMALL_SCREEN_PIXELS = 700;

/** Corner radius of the card which holds the items of one group. */
static constexpr unsigned RADIUS_PT = 8;

/**
 * How round is a corner at a cut which a row of buttons has made in
 * the middle of a group, as a factor of the radius of an outer
 * corner?  Much less than one: a round corner is what says that a
 * card ends, and at a cut the group goes on.
 */
static constexpr double CUT_RADIUS_FACTOR = 0.3;

/**
 * The width of the bar which marks a child, in the same unit as the
 * padding: a third of it, which is a mark of its own next to the
 * caption and takes no room from it.
 */
static constexpr unsigned CHILD_BAR_PT = 3;

/** Distance between a group and the group above it. */
static constexpr unsigned GROUP_GAP_PT = 15;

/** Distance between the caption of a group and its card. */
static constexpr unsigned CAPTION_GAP_PT = 4;

/**
 * Horizontal padding of the contents of a card, the distance between
 * the icon and the caption, and the one between the value and the
 * arrow at its right edge.  As much as the card itself keeps to the
 * edges of the list, unless the items are shorter than a touch
 * target asks for.
 *
 * @see GroupedListControl::GetPadding()
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

/**
 * How many lines may the caption or the value of an item use?  What
 * does not fit ends with an ellipsis; a text which was filled by
 * accident must not blow up the card.
 */
static constexpr std::size_t MAX_TEXT_LINES = 4;

/** Distance between a card and the footer which explains it. */
static constexpr unsigned FOOTER_GAP_PT = 4;

/**
 * While the keys move the cursor, the list scrolls a little further
 * than it must, so that the items behind the one under the cursor
 * are on the screen before the cursor gets to them.  How far may it
 * scroll for that?  One entry per item it wants to gain, in percent
 * of the height of the view, and each of them counts the whole way
 * from the item under the cursor: scrolling by half of the view is
 * worth one item ahead - on a screen which shows only a few items
 * that is all the room there is, and the first item ahead is what
 * matters most -, and a second one is worth it only if the two of
 * them together fit into a fifth of the view, which a landscape
 * window rarely affords and a tall one hardly notices.  The end of
 * the table is the end of the look ahead: two items, however small
 * they are.
 */
static constexpr unsigned LOOK_AHEAD_PERCENT[] = {50, 20};

/**
 * The height of the switch which shows a boolean value, and the
 * distance between its track and its thumb.  The width follows from
 * the height: the track of such a switch is much wider than tall.
 */
static constexpr unsigned TOGGLE_HEIGHT_PT = 22;
static constexpr unsigned TOGGLE_INSET_PT = 2;

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
class GroupedListControl final : public ContainerWindow {
public:
  /** A link inside an explanatory text, from "[caption](url)". */
  struct Link {
    /** the byte range of the caption within the text */
    std::size_t start, end;

    std::string url;
  };

  using Callback = GroupedListWidget::Callback;
  using CursorCallback = GroupedListWidget::CursorCallback;
  using SelectionMode = GroupedListWidget::SelectionMode;
  using CheckPosition = GroupedListWidget::CheckPosition;
  using EnterAction = GroupedListWidget::EnterAction;
  using BadgeStyle = GroupedListWidget::BadgeStyle;
  using TextFont = GroupedListWidget::TextFont;
  using TextSize = GroupedListWidget::TextSize;
  using ToggleHitArea = GroupedListWidget::ToggleHitArea;
  using GroupOptions = GroupedListWidget::GroupOptions;
  using ButtonDefinition = GroupedListWidget::ButtonDefinition;
  using ButtonOptions = GroupedListWidget::ButtonOptions;
  using ItemOptions = GroupedListWidget::ItemOptions;
  using ExpandMode = GroupedListWidget::ExpandMode;
  using ExpandTrigger = GroupedListWidget::ExpandTrigger;

private:
  /**
   * One button of a #Element::Type::BUTTONS row.  It draws itself
   * with the renderer of the dialog, so that it looks like every
   * other button of XCSoar without being a window of its own: a
   * window would join the focus chain, where Up and Down belong to
   * the list.
   */
  struct ElementButton {
    Callback callback;

    TextButtonRenderer renderer;

    bool disabled;

    ElementButton(const ButtonLook &look, const char *caption,
                  Callback _callback, bool _disabled) noexcept
      :callback(std::move(_callback)), renderer(look, caption),
       disabled(_disabled) {}
  };

  struct Element {
    enum class Type : uint_least8_t {
      HERO, CAPTION, FOOTER, ITEM, WIDGET,

      /** a row of one or more buttons, below the card of its group */
      BUTTONS,
    };

    Type type;

    std::string text;

    /**
     * only for Type::BUTTONS: the buttons of this row, which share
     * the width of a card
     */
    std::vector<ElementButton> buttons{};

    /** only for Type::ITEM: drawn at the left edge */
    ResourceId icon_id = ResourceId::Null();

    /** only for Type::ITEM: drawn at the left edge if there is no #icon */
    std::string icon_text{};

    /** the icon which #icon_id names, once it has been loaded */
    std::unique_ptr<MaskedIcon> icon{};

    /** #icon_text in its own colors, on a platform which can do that */
    std::unique_ptr<Bitmap> icon_image{};

    /** the icon size for which the icon has been looked for; 0 if never */
    unsigned icon_size = 0;

    /** can the font draw #icon_text? */
    bool icon_glyph = false;

    /** #text broken into lines by the last layout */
    WrappedText wrapped_text{};

    /** the width for which #wrapped_text was broken; -1 if never */
    int wrapped_text_width = -1;

    /** only for Type::ITEM: #value broken into lines */
    WrappedText wrapped_value{};

    /** the width for which #wrapped_value was broken; -1 if never */
    int wrapped_value_width = -1;

    /** only for Type::ITEM: a second line below #text; it may wrap */
    std::string subtitle{};

    /** only for Type::ITEM: the font of #subtitle */
    TextFont subtitle_font = TextFont::DEFAULT;

    /**
     * only for Type::ITEM: replaces the footer of the group while
     * the cursor is on this item
     */
    std::string help{};

    /** the links of #help (Type::ITEM) or of #text (Type::FOOTER) */
    std::vector<Link> links{};

    /**
     * only for Type::ITEM: drawn at the right edge; only for
     * Type::HERO: the description below the title
     */
    std::string value{};

    /** only for Type::ITEM: draw #value below the caption */
    bool value_below = false;

    /** only for Type::ITEM: the font of #value */
    TextFont value_font = TextFont::DEFAULT;

    /** only for Type::ITEM: the size of #value */
    TextSize value_size = TextSize::DEFAULT;

    /** only for Type::ITEM: show every line of #value */
    bool value_all_lines = false;

    /** only for Type::ITEM: how many lines #value may use; 0 for
        #MAX_TEXT_LINES */
    unsigned value_max_lines = 0;

    /** is #value drawn below the caption?  (the option, or too little room) */
    bool value_is_below = false;

    /** the width of the box which holds #value; 0 if there is none */
    unsigned value_width = 0;

    /** the height of #value, which may need more than one line */
    unsigned value_height = 0;

    /** the height of the caption, which may need more than one line */
    unsigned text_height = 0;

    /** only for Type::ITEM: a text over the whole width, between the
        value and #subtitle */
    std::string description{};

    /** only for Type::ITEM: the font of #description */
    TextFont description_font = TextFont::DEFAULT;

    /** only for Type::ITEM: the size of #description */
    TextSize description_size = TextSize::DEFAULT;

    /** only for Type::ITEM: how many lines #description may use; 0
        for all of them */
    unsigned description_max_lines = 0;

    /** the lines of #description, broken for #wrapped_description_width */
    WrappedText wrapped_description{};

    /** the width for which #wrapped_description was broken; -1 if never */
    int wrapped_description_width = -1;

    /** the height of #description, which may need more than one line */
    unsigned description_height = 0;

    /**
     * The height of the first row, which the caption shares with what
     * stands at the right edge; only while HasRows()
     */
    unsigned first_row_height = 0;

    /**
     * Is the item a stack of rows over the whole width, below a
     * first row which holds the caption and the decorations?
     */
    bool HasRows() const noexcept {
      return !description.empty();
    }

    /** only for Type::ITEM: drawn in a rounded box */
    std::string badge{};

    /** only for Type::ITEM: the colors of #badge */
    BadgeStyle badge_style = BadgeStyle::PRIMARY;

    /** only for Type::ITEM: the font of #badge */
    TextFont badge_font = TextFont::DEFAULT;

    /** only for Type::ITEM */
    Callback callback{};

    /** only for Type::ITEM: draw an arrow at the right edge */
    bool chevron = false;

    /** only for Type::ITEM: draw a check mark */
    bool checked = false;

    /** only for Type::ITEM: greyed out, and does nothing */
    bool disabled = false;

    /** only for Type::ITEM: may the cursor rest on it while #disabled? */
    bool selectable_when_disabled = false;

    /** only for Type::ITEM: left out, as if it had not been added */
    bool hidden = false;

    /** only for Type::ITEM: a switch which shows #checked */
    bool toggle = false;

    /** only for Type::ITEM: where a tap flips #toggle */
    ToggleHitArea toggle_hit_area = ToggleHitArea::SWITCH;

    /** only for Type::WIDGET: the view which this group shows */
    std::unique_ptr<Widget> widget{};

    /** only for Type::WIDGET: its height; 0 asks the view itself */
    unsigned widget_height_pt = 0;

    /** has #widget been prepared for the list window? */
    bool widget_prepared = false;

    /** is #widget currently shown? */
    bool widget_visible = false;

    /** only for Type::ITEM: may this item be checked, and how? */
    SelectionMode selection_mode = SelectionMode::NONE;

    /** only for Type::ITEM: the edge which holds the check mark */
    CheckPosition check_position = CheckPosition::RIGHT;

    /** only for Type::ITEM: what Enter does on it */
    EnterAction enter_action = EnterAction::ITEM;

    /** only for Type::ITEM: how many items of its group may be open */
    ExpandMode expand_mode = ExpandMode::MULTIPLE;

    /** only for Type::ITEM: what opens it */
    ExpandTrigger expand_trigger = ExpandTrigger::ACTIVATE;

    /**
     * How deep does this item sit below the one above it?  0 is an
     * item of the card itself, 1 a child of the item above it.  Only
     * one level is built today; the field is a number so that a
     * deeper tree costs nothing but the rules which go with it.
     */
    uint_least8_t depth = 0;

    /** does an item follow this one which is deeper than it? */
    bool has_children = false;

    /** are the children of this item shown? */
    bool expanded = false;

    /** is this item a child of one which is closed? */
    bool collapsed = false;

    /** the first / the last item of its card */
    bool first_in_group = false, last_in_group = false;

    /**
     * Is this edge of the card a cut which a row of buttons has made
     * in the middle of a group, with items on the other side of it?
     * Such an edge is rounded less, so that the two halves still read
     * as one group.
     */
    bool cut_top = false, cut_bottom = false;

    /**
     * Does any item of this group have a check mark at this edge?
     * All items of the group then reserve the room for it, which
     * keeps their captions aligned.
     */
    bool check_left = false, check_right = false;

    /** does any item of this group have an icon? */
    bool icon_column = false;

    /** does this element belong to the group which is open? */
    bool IsGroupContent() const noexcept {
      return IsItem() || IsButtons() || type == Type::WIDGET;
    }

    /** does this element have something to draw in the icon column? */
    bool HasIcon() const noexcept {
      return icon != nullptr || icon_image != nullptr || icon_glyph;
    }

    /** the height of #subtitle, which may need more than one line */
    unsigned subtitle_height = 0;

    /** position and height within the virtual contents */
    int top = 0;
    unsigned height = 0;

    bool IsItem() const noexcept {
      return type == Type::ITEM;
    }

    bool IsButtons() const noexcept {
      return type == Type::BUTTONS;
    }

    /** is this element an item which is drawn? */
    bool IsShownItem() const noexcept {
      return IsItem() && !hidden && !collapsed;
    }

    /** may the cursor be moved to this element? */
    bool IsSelectable() const noexcept {
      if (IsButtons())
        /* a row of buttons which are all disabled is a row the cursor
           has nothing to do on */
        return std::any_of(buttons.begin(), buttons.end(),
                           [](const ElementButton &button){
                             return !button.disabled;
                           });

      return IsShownItem() && (!disabled || selectable_when_disabled);
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

  /** does #saved_cursor count rows of buttons instead of items? */
  bool saved_buttons = false;

  /** the button which was armed when Clear() saved the cursor */
  unsigned saved_column = 0;

  /** the items which were open when Clear() saved them, as item indexes */
  std::vector<unsigned> saved_expanded;

  /** does #saved_expanded wait to be applied? */
  bool restore_expanded = false;

  /**
   * Has the cursor been taken off the list on purpose - a tap beside
   * the items, a page which has none to point at?  Then UpdateLayout()
   * must leave the list without one, instead of putting it on the
   * first item as it does for a list which has just been filled.
   */
  bool cursor_removed = false;

  CursorCallback cursor_callback;

  enum class DragMode : uint_least8_t { NONE, SCROLL, CURSOR };

  DragMode drag_mode = DragMode::NONE;

  /**
   * The item under the finger, which is drawn pressed; -1 if there is
   * none.  The cursor stays where it was until the finger is lifted:
   * a press which becomes a scroll gesture must not select anything.
   */
  int pressed_element = -1;

  /**
   * Which button of a #Element::Type::BUTTONS row is armed, and which
   * one the finger is on.  Left and Right walk along a row the way
   * they walk along the action bar.
   */
  unsigned button_column = 0;
  int pressed_column = -1;

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
  void AddChildItem(const char *caption, Callback callback,
                    const ItemOptions &options) noexcept;
  void AddButtons(std::span<const ButtonDefinition> buttons,
                  const ButtonOptions &options) noexcept;
  void AddWidget(std::unique_ptr<Widget> widget,
                 unsigned height_pt) noexcept;

  /**
   * Remove all elements, but remember where the user was: a list
   * which refreshes itself while it is on the screen (a device list,
   * a WiFi scan) is cleared and filled again, and must not jump back
   * to the top each time.  The next UpdateLayout() restores the item
   * the cursor was on, and the scroll position follows the contents.
   */
  void Clear() noexcept {
    /* a row of buttons is not an item and has no item index; count
       the rows instead, so that a page which fills itself again
       keeps the cursor and the armed button where they were */
    saved_buttons = cursor >= 0 && (std::size_t)cursor < elements.size() &&
      elements[cursor].IsButtons();
    saved_cursor = saved_buttons ? GetButtonsIndex() : GetCursorIndex();
    saved_column = button_column;

    /* a page which fills itself again keeps what the user has opened */
    saved_expanded.clear();
    restore_expanded = true;

    unsigned n = 0;
    for (const auto &element : elements) {
      if (!element.IsItem())
        continue;

      if (element.expanded)
        saved_expanded.push_back(n);

      ++n;
    }

    pressed_element = -1;
    pressed_column = -1;

    /* the views are windows: hide them before their #Widget goes */
    for (auto &element : elements)
      UnprepareWidget(element);

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

  void SetCursorByIndex(unsigned i) noexcept;

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
   * The text of a footer element and its links: the help of the
   * item the cursor is on, or the footer of the group.
   */
  struct Footer {
    const std::string *text;
    const std::vector<Link> *links;
  };

  [[gnu::pure]]
  Footer GetFooter(std::size_t i) const noexcept;

  /** The area which the text of a footer element occupies. */
  [[gnu::pure]]
  PixelRect GetFooterRect(std::size_t i) const noexcept;

  /**
   * Walk the pieces of the text of a footer element: each call gets
   * the area of one piece, its text, and the link it belongs to, or
   * nullptr if it is plain text.
   */
  using FooterCallback =
    std::function<void(PixelRect rc, std::string_view text,
                       const Link *link)>;

  void WalkFooter(std::size_t i, FooterCallback f) const noexcept;

  /**
   * @return the link at the given position, or nullptr if there is
   * none
   */
  [[gnu::pure]]
  const Link *FindLinkAt(PixelPoint p) const noexcept;

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
    return OnKeyCheck(key_code) && OnKeyDown(key_code);
  }

  /**
   * Arm the next button of the row under the cursor.
   *
   * @return false if the cursor is not on a row of buttons, or if the
   * row has none left in this direction; Left and Right then belong
   * to the action bar
   */
  bool MoveButtonColumn(bool forward) noexcept;

  /**
   * Hand one call of the #Widget protocol to the views of the widget
   * groups.  A view which edits something is saved like the widgets
   * around the list, and it may know a key which the list does not.
   */
  bool SaveWidgets(bool &changed) noexcept;
  bool LeaveWidgets() noexcept;
  bool KeyPressWidgets(unsigned key_code) noexcept;

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

  /** How many rows of buttons stand above the cursor? */
  [[gnu::pure]]
  int GetButtonsIndex() const noexcept {
    int n = 0;

    for (int j = 0; j < cursor; ++j)
      if (elements[j].IsButtons())
        ++n;

    return n;
  }

  /** The element which holds the i-th row of buttons; -1 if there is none. */
  [[gnu::pure]]
  int FindButtonsByIndex(unsigned i) const noexcept {
    unsigned n = 0;

    for (std::size_t j = 0; j < elements.size(); ++j) {
      if (!elements[j].IsButtons())
        continue;

      if (n == i)
        return (int)j;

      ++n;
    }

    return -1;
  }

  /** Check the given item and uncheck the others of its group. */
  void CheckOnly(std::size_t i) noexcept;

  /**
   * The width which the decorations at the edges of an item take away
   * from its caption.
   */
  [[gnu::pure]]
  int GetDecorationWidth(const Element &element) const noexcept;

  /**
   * The width which the decorations at the left edge and the inset at
   * the right edge take away from every row of an item: what a row
   * below the first one loses in an item which is a stack of rows.
   */
  [[gnu::pure]]
  int GetRowDecorationWidth(const Element &element) const noexcept;

  /**
   * The height which the decorations at the right edge need when they
   * share the first row with the caption.
   */
  [[gnu::pure]]
  int GetFirstRowDecorationHeight(const Element &element) const noexcept;

  /**
   * Divide the room of an item between the caption and the value:
   * both may wrap, and the one which needs more of it gets more.
   *
   * @param room the width which the caption and the value share
   * @param wide_room the width of a value below the caption; the
   * same as @p room unless the item is a stack of rows
   * @return the width of the box which holds the caption
   */
  int UpdateTextLayout(Element &element, int room,
                       int wide_room) const noexcept;

  /**
   * Draw a text into its box; a text which is too wide wraps, and the
   * last line ends with an ellipsis.
   *
   * @param right align the lines at the right edge of the box
   */
  void DrawWrappedText(Canvas &canvas, const Font &font, const PixelRect &rc,
                       const std::string &text, const WrappedText &cached,
                       int cached_width, bool right,
                       std::size_t max_lines=MAX_TEXT_LINES) const noexcept;

  /** How many lines the value of the given item may use. */
  [[gnu::pure]]
  static std::size_t GetValueMaxLines(const Element &element) noexcept {
    if (element.value_all_lines)
      return std::numeric_limits<std::size_t>::max();

    return element.value_max_lines > 0
      ? element.value_max_lines
      : MAX_TEXT_LINES;
  }

  /** How many lines the description of the given item may use. */
  [[gnu::pure]]
  static std::size_t GetDescriptionMaxLines(const Element &element) noexcept {
    return element.description_max_lines > 0
      ? element.description_max_lines
      : std::numeric_limits<std::size_t>::max();
  }

  /**
   * Does the view have so little room that the cards are better off
   * filling it?  On such a screen the margin costs a tenth of the
   * width, and the text is what it is taken from.
   */
  [[gnu::pure]]
  bool IsNarrowView() const noexcept {
    const unsigned width = GetSize().width;

    return width < NARROW_VIEW_PIXELS ||
      (Layout::small_screen && width < NARROW_SMALL_SCREEN_PIXELS);
  }

  /**
   * The distance which the cards keep to the left and to the right
   * edge of the view.  A narrow view has none: there the cards go
   * from edge to edge.
   */
  [[gnu::pure]]
  int GetCardMargin() const noexcept {
    return IsNarrowView() ? 0 : (int)Layout::VptScale(MARGIN_PT);
  }

  /**
   * The radius of the corners of a card.  It goes with the margin:
   * a corner which is rounded against the edge of the screen is no
   * corner but a bite out of the card.
   */
  [[gnu::pure]]
  int GetCardRadius() const noexcept {
    return IsNarrowView() ? 0 : (int)Layout::VptScale(RADIUS_PT);
  }

  /**
   * The horizontal padding inside a card, which is also the distance
   * between the icon of an item and its caption.  It is at most a
   * quarter of the height of an item: a device without a touch
   * screen makes its rows as tall as one line of text, and the
   * padding a touch target asks for would be wider there than the
   * text is tall.  On a touch screen the rows are tall enough that
   * the padding keeps the full #PADDING_PT.
   */
  [[gnu::pure]]
  int GetPadding() const noexcept {
    return std::min((int)Layout::VptScale(PADDING_PT),
                    (int)GetItemHeight() / 4);
  }

  /**
   * The width and the height of the icon of an item: one and a half
   * lines of text, so that it does not grow with the item when a
   * subtitle makes it taller.
   */
  [[gnu::pure]]
  int GetIconSize() const noexcept {
    return (int)look.list.font->GetHeight() * 3 / 2;
  }

  /**
   * The width of the column which holds the icon, including the
   * padding which separates it from the text.
   */
  [[gnu::pure]]
  int GetIconWidth() const noexcept {
    return GetIconSize() + GetPadding();
  }

  /**
   * The width of the bar which stands at the left edge of a child, in
   * the color of the brand.  A child keeps the width and the columns
   * of the item it belongs to: the bar says what an indent would say
   * and costs the caption nothing.  It is never as thin as the line
   * between two items, which would read as a hairline.
   */
  [[gnu::pure]]
  int GetChildBarWidth() const noexcept {
    return std::max((int)Layout::VptScale(CHILD_BAR_PT),
                    2 * GetSeparatorThickness());
  }

  /** Load the icons, and find out which ones can be drawn. */
  void PrepareIcons() noexcept;

  /** The height of the view which a Type::WIDGET element shows. */
  [[gnu::pure]]
  unsigned GetWidgetHeight(const Element &element) const noexcept;

  [[gnu::pure]]
  PixelRect GetWidgetRect(const Element &element) const noexcept;

  /** Prepare the views which have been added since the last time. */
  void PrepareWidgets() noexcept;

  /** Undo PrepareWidgets() for one element. */
  static void UnprepareWidget(Element &element) noexcept {
    if (!element.widget_prepared)
      return;

    if (element.widget_visible) {
      element.widget->Hide();
      element.widget_visible = false;
    }

    element.widget->Unprepare();
    element.widget_prepared = false;
  }

  /** Move the views to where the list has scrolled them. */
  void MoveWidgets() noexcept;

  /** The font which draws the subtitle of the given item. */
  [[gnu::pure]]
  const Font &GetSubtitleFont(const Element &element) const noexcept {
    return element.subtitle_font == TextFont::MONO &&
      look.small_mono_font.IsDefined()
      ? look.small_mono_font
      : look.small_font;
  }

  /** The font of the given family and size, as far as the look has it. */
  [[gnu::pure]]
  const Font &GetFont(TextFont font, TextSize size) const noexcept {
    const bool mono = font == TextFont::MONO;

    if (size == TextSize::SMALL)
      return mono && look.small_mono_font.IsDefined()
        ? look.small_mono_font
        : look.small_font;

    return mono && look.mono_font.IsDefined()
      ? look.mono_font
      : *look.list.font;
  }

  /** The font which draws the value of the given item. */
  [[gnu::pure]]
  const Font &GetValueFont(const Element &element) const noexcept {
    return GetFont(element.value_font, element.value_size);
  }

  /** The font which draws the description of the given item. */
  [[gnu::pure]]
  const Font &GetDescriptionFont(const Element &element) const noexcept {
    return GetFont(element.description_font, element.description_size);
  }

  /** The font which draws the badge of an item. */
  [[gnu::pure]]
  const Font &GetBadgeFont(const Element &element) const noexcept {
    return element.badge_font == TextFont::MONO && look.mono_font.IsDefined()
      ? look.mono_font
      : *look.list.font;
  }

  /**
   * The height of the box of the badge: as tall as the list font, so
   * that the badges of a group are the same size, and taller where
   * another font needs the room.
   */
  [[gnu::pure]]
  int GetBadgeHeight(const Element &element) const noexcept {
    const int badge_pad_y = Layout::VptScale(BADGE_PADDING_PT) / 2;
    return std::max((int)look.list.font->GetHeight(),
                    (int)GetBadgeFont(element).GetHeight())
      + 2 * badge_pad_y;
  }

  /**
   * The width of the column which holds a check mark, including the
   * padding which separates it from the text.
   */
  [[gnu::pure]]
  int GetCheckWidth() const noexcept {
    return GetCheckSize() + GetPadding();
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
   * The gap above a row of buttons.  A row which follows the card it
   * belongs to keeps the distance of a footer, and one which follows
   * another row stands right below it, the way the buttons of a
   * dialog do.
   */
  [[gnu::pure]]
  int GetButtonsLeadingGap(std::size_t i) const noexcept {
    if (i > 0) {
      const Element &previous = elements[i - 1];

      if (previous.IsButtons())
        /* two rows stand right below each other, like the buttons of
           a dialog; a description between them needs the same air
           below it as it has above it */
        return previous.text.empty()
          ? 0
          : Layout::VptScale(CAPTION_GAP_PT);

      if (previous.IsItem())
        return Layout::VptScale(FOOTER_GAP_PT);

      if (previous.type == Element::Type::CAPTION)
        /* the caption of the group carries the gap above it */
        return 0;
    }

    return GetLeadingGap(i);
  }

  /**
   * How many buttons of a row stand next to each other?  They share
   * the width of a card, and where the widest caption does not fit
   * into its share, they stand below each other instead.
   */
  [[gnu::pure]]
  unsigned GetButtonColumns(const Element &element) const noexcept {
    const unsigned count = element.buttons.size();
    if (count <= 1)
      return 1;

    unsigned widest = 0;
    for (const auto &button : element.buttons)
      widest = std::max(widest, button.renderer.GetMinimumButtonWidth());

    const int room = GetContentWidth() - 2 * GetCardMargin();

    return (int)(widest * count) <= room ? count : 1;
  }

  /** How many rows the buttons of this element need. */
  [[gnu::pure]]
  unsigned GetButtonLines(const Element &element) const noexcept {
    const unsigned columns = GetButtonColumns(element);

    return (element.buttons.size() + columns - 1) / columns;
  }

  /** The place of one button within the card rectangle of its row. */
  [[gnu::pure]]
  PixelRect GetButtonRect(const Element &element, unsigned j,
                          PixelRect rc, int top) const noexcept {
    const unsigned columns = GetButtonColumns(element);
    const int height = (int)GetItemHeight();
    const int width = rc.GetWidth();

    const unsigned line = j / columns, column = j % columns;

    return {rc.left + (int)(width * column / columns),
            top + (int)line * height,
            rc.left + (int)(width * (column + 1) / columns),
            top + (int)(line + 1) * height};
  }

  /** The card rectangle of an element, in the window. */
  [[gnu::pure]]
  PixelRect GetCardRect(const Element &element) const noexcept {
    const int margin = GetCardMargin();

    return {margin, element.top - origin,
            GetContentWidth() - margin, element.GetBottom() - origin};
  }

  /**
   * Which button of a row is at this position; -1 beside all of
   * them.
   */
  [[gnu::pure]]
  int FindButtonAt(std::size_t i, PixelPoint p) const noexcept {
    const Element &element = elements[i];
    const PixelRect rc = GetCardRect(element);
    const int top = rc.top + GetButtonsLeadingGap(i);

    for (std::size_t j = 0; j < element.buttons.size(); ++j)
      if (GetButtonRect(element, j, rc, top).Contains(p))
        return (int)j;

    return -1;
  }

  /** The first button of a row which can be pressed; 0 if there is none. */
  [[gnu::pure]]
  static unsigned GetFirstButton(const Element &element) noexcept {
    for (std::size_t i = 0; i < element.buttons.size(); ++i)
      if (!element.buttons[i].disabled)
        return (unsigned)i;

    return 0;
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

  /** The height of the switch which shows a boolean value. */
  [[gnu::pure]]
  int GetToggleHeight() const noexcept {
    /* it must not touch the edges of the item on a device whose rows
       are shorter than the switch would like to be; the room it
       leaves is the one the text keeps, which is a vertical measure.
       The padding of the card is the horizontal one and much larger:
       on a device without a touch screen, where a row is only as
       tall as one line of text, it would leave nothing at all.  An
       even height lets the two half circles meet the rectangle
       between them without a step */
    return std::max(6, std::min((int)Layout::VptScale(TOGGLE_HEIGHT_PT),
                                (int)GetItemHeight()
                                - 2 * (int)Layout::GetTextPadding())) & ~1;
  }

  /** The width of that switch; the track is much wider than tall. */
  [[gnu::pure]]
  int GetToggleWidth() const noexcept {
    return GetToggleHeight() * 33 / 20;
  }

  static void DrawCheck(Canvas &canvas, PixelRect rc, Color color) noexcept;

  static void DrawToggle(Canvas &canvas, const PixelRect &rc, bool checked,
                         const DialogLook &look,
                         Color background_color, Color text_color) noexcept;

  void SetOrigin(int _origin) noexcept;
  void EnsureVisible(unsigned i) noexcept;

  /**
   * Scroll a little further than #EnsureVisible() would, in the
   * direction the cursor is moving, so that the list shows what the
   * next presses lead to.
   */
  void ScrollAhead(unsigned i, bool forward) noexcept;

  void SetCursor(int i) noexcept;

  /**
   * Take the cursor off the list, e.g. because the user has tapped
   * beside the items.  The help text of the item goes with it.
   */
  void ClearCursor() noexcept;

  /**
   * Where a search for the next item begins.  Without a cursor - the
   * list has just been filled, or a tap beside the items has taken it
   * away - the search starts at the edge it comes from, so that Down
   * finds the first item and Up the last one.
   */
  [[gnu::pure]]
  int GetSearchStart(bool forward) const noexcept {
    if (cursor >= 0)
      return forward ? cursor + 1 : cursor;

    return forward ? 0 : (int)elements.size();
  }

  void MoveCursor(bool forward) noexcept;

  /**
   * Move the cursor by one screen full of items.  A page without a
   * single item the cursor may rest on has nothing to move: it
   * scrolls instead.
   */
  void MoveCursorPage(bool forward) noexcept;

  void ActivateItem() noexcept;

  /**
   * The horizontal range which the switch of an item occupies, wide
   * enough for a finger.  Empty (left == right) if the item has no
   * switch.
   */
  [[gnu::pure]]
  std::pair<int, int> GetToggleHitArea(const Element &element) const noexcept;

  void UpdateGroupFlags() noexcept;

  /**
   * Find out which items have children and hide the children of
   * those which are closed.
   */
  void UpdateExpansion() noexcept;

  /**
   * Open or close the item under the given element index.  The row
   * stays where it is on the screen, and what opens below it is
   * shown as far as it fits.
   */
  void ToggleExpanded(std::size_t i) noexcept;

  /**
   * The item which holds this element: the element itself when it is
   * not a child, and -1 when it is no item at all.
   */
  [[gnu::pure]]
  int FindParentItem(int i) const noexcept {
    if (i < 0 || (std::size_t)i >= elements.size() || !elements[i].IsItem())
      return -1;

    while (i > 0 && elements[i].depth > 0)
      --i;

    return i;
  }

  /**
   * Open what the cursor has arrived at and close what it has left,
   * in a group where the cursor is what opens an item.
   */
  void UpdateCursorExpansion(int previous) noexcept;

  /**
   * Paint the two corners of one edge with the color behind the
   * rectangle, which makes them appear rounded.
   *
   * @param top round the upper corners, else the lower ones
   */
  static void DrawRoundedEdge(Canvas &canvas, const PixelRect &rc,
                              bool top, Color color, int radius) noexcept;

  /**
   * Paint the arc of one rounded corner, just inside the pixels which
   * #DrawRoundedEdge has taken away.
   *
   * @param top the upper corners, else the lower ones
   */
  static void DrawRoundedCorner(Canvas &canvas, const PixelRect &rc,
                                bool top, Color color, int radius,
                                int thickness) noexcept;

  /**
   * Draw the edge of one item of a card: its two sides, the line to
   * the next item, and the rounded upper or lower edge of the card
   * where this item is the first or the last one of it.
   */
  static void DrawCardBorder(Canvas &canvas, const PixelRect &rc,
                             bool first, bool last, Color color,
                             int top_radius, int bottom_radius) noexcept;

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

/**
 * Extract the links from a text which explains an item or a group,
 * and return the text without their markup.
 */
static std::string
ParseLinks(const char *src,
           std::vector<GroupedListControl::Link> &links) noexcept
{
  links.clear();

  if (src == nullptr || *src == '\0')
    return {};

  auto parsed = ParseMarkdown(src);

  for (const auto &link : parsed.links)
    links.push_back({link.start, link.end, link.url});

  return std::move(parsed.text);
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
  if (elements.empty() || !elements.back().IsGroupContent())
    /* no group is open, or its footer has been added already */
    return;

  const char *const footer = group_options.footer != nullptr
    ? group_options.footer
    : "";

  /* an item which explains itself needs a footer to be explained
     in, even if the group has no text of its own */
  bool needed = *footer != '\0';

  for (auto i = elements.rbegin();
       i != elements.rend() && (i->IsItem() || i->IsButtons()); ++i)
    /* the help of a hidden item is never shown: the cursor cannot
       reach it, and an empty footer would be a gap below the card */
    if (i->IsShownItem() && !i->help.empty())
      needed = true;

  if (!needed)
    return;

  Element &element = elements.emplace_back(Element{
    .type = Element::Type::FOOTER,
    .text = {},
  });

  element.text = ParseLinks(footer, element.links);
}

GroupedListControl::Footer
GroupedListControl::GetFooter(std::size_t i) const noexcept
{
  assert(i < elements.size());
  assert(elements[i].type == Element::Type::FOOTER);

  /* the items of this group are the elements right above it */
  if (cursor >= 0 && (std::size_t)cursor < i &&
      !elements[cursor].help.empty()) {
    bool in_group = true;

    for (std::size_t j = i; j-- > (std::size_t)cursor;)
      if (!elements[j].IsItem() && !elements[j].IsButtons()) {
        in_group = false;
        break;
      }

    if (in_group)
      return {&elements[cursor].help, &elements[cursor].links};
  }

  return {&elements[i].text, &elements[i].links};
}

PixelRect
GroupedListControl::GetFooterRect(std::size_t i) const noexcept
{
  const Element &element = elements[i];
  const int margin = GetCardMargin();
  const int padding = GetPadding();

  return {margin + padding,
          element.top - origin + (int)Layout::VptScale(FOOTER_GAP_PT),
          GetContentWidth() - margin - padding,
          element.GetBottom() - origin};
}

void
GroupedListControl::WalkFooter(std::size_t i,
                                 FooterCallback f) const noexcept
{
  const auto footer = GetFooter(i);
  if (footer.text->empty())
    return;

  const std::string &text = *footer.text;
  const Font &font = *look.list.font;
  const int line_spacing = font.GetLineSpacing();

  const PixelRect rc = GetFooterRect(i);
  const int width = std::max((int)rc.GetWidth(), 1);

  /* the layout has broken this text already */
  WrappedText fallback;
  if (elements[i].wrapped_text_width != width)
    fallback = WrapText(font, width, text);

  const WrappedText &wrapped = elements[i].wrapped_text_width == width
    ? elements[i].wrapped_text
    : fallback;

  int y = rc.top;

  for (const auto &line : wrapped.lines) {
    const std::size_t line_end = line.start + line.length;
    std::size_t position = line.start;
    int x = rc.left;

    while (position < line_end) {
      /* the piece which begins here is either one link, or the plain
         text up to the next one */
      const Link *link = nullptr;
      std::size_t end = line_end;

      for (const auto &l : *footer.links) {
        if (l.end <= position || l.start >= line_end)
          continue;

        if (l.start <= position) {
          link = &l;
          end = std::min(l.end, line_end);
          break;
        }

        if (l.start < end)
          end = l.start;
      }

      const std::string_view piece{text.data() + position, end - position};
      const int piece_width = font.TextSize(piece).width;

      f(PixelRect{x, y, x + piece_width, y + line_spacing}, piece, link);

      x += piece_width;
      position = end;
    }

    y += line_spacing;
  }
}

const GroupedListControl::Link *
GroupedListControl::FindLinkAt(PixelPoint p) const noexcept
{
  const int i = FindElementAt(p.y);
  if (i < 0 || elements[i].type != Element::Type::FOOTER)
    return nullptr;

  const Link *result = nullptr;

  WalkFooter(i, [&result, p](PixelRect rc, std::string_view,
                               const Link *link){
    if (link != nullptr && rc.Contains(p))
      result = link;
  });

  return result;
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

  if (IsDithered())
    /* a display which knows two colors would turn all of them into
       the same black; one with gray levels tells them apart */
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
    .icon_id = options.icon,
    .icon_text = options.icon_text != nullptr ? options.icon_text : "",
    .subtitle = options.subtitle != nullptr ? options.subtitle : "",
    .subtitle_font = options.subtitle_font,

    .value = options.value != nullptr ? options.value : "",
    .value_below = options.value_below,
    .value_font = options.value_font,
    .value_size = options.value_size,
    .value_all_lines = options.value_all_lines,
    .value_max_lines = options.value_max_lines,
    .description = options.description != nullptr ? options.description : "",
    .description_font = options.description_font,
    .description_size = options.description_size,
    .description_max_lines = options.description_max_lines,
    .badge = GetBadge(options),
    .badge_style = options.badge_style,
    .badge_font = options.badge_font,
    .callback = std::move(callback),
    .chevron = options.chevron,
    .checked = options.checked,
    .disabled = options.disabled,
    .selectable_when_disabled = options.selectable_when_disabled,
    .hidden = options.hidden,
    .toggle = options.toggle,
    .toggle_hit_area = options.toggle_hit_area,
    .selection_mode = group_options.selection_mode,
    .check_position = group_options.check_position,
    .enter_action = group_options.enter_action,
    .expand_mode = group_options.expand_mode,
    .expand_trigger = group_options.expand_trigger,
  });

  Element &element = elements.back();
  element.help = ParseLinks(options.help, element.links);
}

void
GroupedListControl::AddChildItem(const char *caption, Callback callback,
                                 const ItemOptions &options) noexcept
{
  assert(!elements.empty());
  assert(elements.back().IsItem());

  /* the item above is the parent, or a sibling which has been added
     to it already */
  const uint_least8_t depth = elements.back().depth == 0
    ? 1
    : elements.back().depth;

  AddItem(caption, std::move(callback), options);

  Element &element = elements.back();
  element.depth = depth;

  /* a child is not on the screen before its parent has been opened;
     UpdateLayout() decides that again from the parent */
  element.collapsed = true;
}

void
GroupedListControl::AddButtons(std::span<const ButtonDefinition> definitions,
                               const ButtonOptions &options) noexcept
{
  assert(!definitions.empty());

  elements.push_back(Element{
    .type = Element::Type::BUTTONS,
    .text = options.description != nullptr ? options.description : "",
  });

  Element &element = elements.back();
  element.buttons.reserve(definitions.size());

  for (const auto &definition : definitions) {
    assert(definition.caption != nullptr);

    element.buttons.emplace_back(look.button, definition.caption,
                                 definition.callback,
                                 definition.disabled || options.disabled);
  }
}

void
GroupedListControl::AddWidget(std::unique_ptr<Widget> widget,
                              unsigned height_pt) noexcept
{
  assert(widget != nullptr);

  Element &element = elements.emplace_back(Element{
    .type = Element::Type::WIDGET,
    .text = {},
  });

  element.widget = std::move(widget);
  element.widget_height_pt = height_pt;
}

unsigned
GroupedListControl::GetWidgetHeight(const Element &element) const noexcept
{
  assert(element.type == Element::Type::WIDGET);

  if (element.widget_height_pt > 0)
    return Layout::VptScale(element.widget_height_pt);

  /* the view knows best how tall it needs to be, and other than the
     widget below the list it may be taller than the window: the list
     scrolls */
  unsigned height = element.widget->GetMaximumSize().height;

  if (height == 0)
    height = element.widget->GetMinimumSize().height;

  return std::max(height, Layout::GetMinimumControlHeight());
}

/**
 * The rectangle of a view inside the list window: it uses the width
 * of a card, and it scrolls with the list.
 */
PixelRect
GroupedListControl::GetWidgetRect(const Element &element) const noexcept
{
  const int margin = GetCardMargin();
  const int top = element.GetBottom() - (int)GetWidgetHeight(element) - origin;

  return PixelRect{margin, top, GetContentWidth() - margin,
                   top + (int)GetWidgetHeight(element)};
}

void
GroupedListControl::PrepareWidgets() noexcept
{
  if (!IsDefined())
    return;

  for (auto &element : elements) {
    if (element.type != Element::Type::WIDGET || element.widget_prepared)
      continue;

    const PixelRect rc{0, 0, 1, 1};

    element.widget->Initialise(*this, rc);
    element.widget->Prepare(*this, rc);
    element.widget_prepared = true;
  }
}

bool
GroupedListControl::SaveWidgets(bool &changed) noexcept
{
  for (auto &element : elements)
    if (element.widget_prepared && !element.widget->Save(changed))
      return false;

  return true;
}

bool
GroupedListControl::LeaveWidgets() noexcept
{
  for (auto &element : elements)
    if (element.widget_prepared && !element.widget->Leave())
      return false;

  return true;
}

bool
GroupedListControl::KeyPressWidgets(unsigned key_code) noexcept
{
  for (auto &element : elements)
    if (element.widget_prepared && element.widget->KeyPress(key_code))
      return true;

  return false;
}

void
GroupedListControl::MoveWidgets() noexcept
{
  if (!IsDefined())
    return;

  const int bottom = GetViewHeight();

  for (auto &element : elements) {
    if (element.type != Element::Type::WIDGET || !element.widget_prepared)
      continue;

    const PixelRect rc = GetWidgetRect(element);

    if (rc.bottom <= 0 || rc.top >= bottom) {
      /* scrolled out of the window */
      if (element.widget_visible) {
        element.widget->Hide();
        element.widget_visible = false;
      }

      continue;
    }

    if (element.widget_visible)
      element.widget->Move(rc);
    else {
      element.widget->Show(rc);
      element.widget_visible = true;
    }
  }
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
GroupedListControl::PrepareIcons() noexcept
{
  const unsigned size = GetIconSize();

  for (auto &element : elements) {
    if (!element.IsItem())
      continue;

    if (element.icon_size == size)
      /* the icon of an item does not change; looking for one which
         is not there costs a font lookup for every character, and on
         a platform with color fonts a good deal more */
      continue;

    element.icon_size = size;

    if (element.icon == nullptr && element.icon_id.IsDefined()) {
      auto icon = std::make_unique<MaskedIcon>();
      icon->LoadResource(element.icon_id);

      if (icon->IsDefined())
        element.icon = std::move(icon);
    }

    if (element.icon == nullptr && element.icon_image == nullptr &&
        !element.icon_text.empty()) {
      /* an emoji is a color glyph, and the text stack can only draw a
         one-color mask; where the platform has color fonts, render it
         into an image instead */
      auto image = std::make_unique<Bitmap>();

      if (RenderColorGlyph(element.icon_text.c_str(), size, *image))
        element.icon_image = std::move(image);
    }

    /* a character is only an icon if the font can draw it; on a
       display which has no glyph for it, the item keeps no room for
       one */
    element.icon_glyph = element.icon == nullptr &&
      element.icon_image == nullptr &&
      !element.icon_text.empty() &&
      look.heading1_font.HasGlyph(NextUTF8(element.icon_text.c_str()).first);
  }
}

void
GroupedListControl::UpdateExpansion() noexcept
{
  for (std::size_t i = 0; i < elements.size(); ++i) {
    Element &element = elements[i];
    if (!element.IsItem() || element.depth > 0)
      continue;

    /* the items below it which are deeper are its children */
    std::size_t j = i + 1;

    for (; j < elements.size() && elements[j].IsItem() &&
           elements[j].depth > element.depth; ++j)
      elements[j].collapsed = !element.expanded;

    element.has_children = j > i + 1;

    if (!element.has_children)
      element.expanded = false;
  }
}

void
GroupedListControl::ToggleExpanded(std::size_t i) noexcept
{
  assert(i < elements.size());

  Element &element = elements[i];
  if (!element.has_children)
    return;

  const bool expand = !element.expanded;
  const unsigned depth = element.depth;

  if (expand && element.expand_mode == ExpandMode::SINGLE) {
    /* like an accordion: the group shows the children of one item */
    for (std::size_t j = 0; j < elements.size(); ++j) {
      if (!elements[j].IsItem())
        continue;

      if (elements[j].depth == 0 && j != i)
        elements[j].expanded = false;
    }
  }

  element.expanded = expand;

  if (!expand && cursor > (int)i && (std::size_t)cursor < elements.size() &&
      elements[cursor].depth > depth)
    /* the cursor was on a child which is about to go: it follows the
       item it belonged to */
    cursor = (int)i;

  /* the row under the cursor keeps its place on the screen, whatever
     opens or closes above it */
  const int anchor = cursor >= 0 && (std::size_t)cursor < elements.size()
    ? elements[cursor].top - origin
    : -1;

  UpdateLayout();

  if (anchor >= 0 && cursor >= 0 && (std::size_t)cursor < elements.size())
    SetOrigin(elements[cursor].top - anchor);

  if (!expand)
    return;

  /* show as much of what has opened as fits, without pushing the item
     itself off the top of the view */
  std::size_t last = i;
  while (last + 1 < elements.size() && elements[last + 1].IsItem() &&
         elements[last + 1].depth > depth)
    ++last;

  const int bottom = elements[last].GetBottom();
  const int room = GetViewHeight();

  if (bottom > origin + room)
    SetOrigin(std::min(bottom - room, elements[i].top));
}

void
GroupedListControl::UpdateCursorExpansion(int previous) noexcept
{
  const int here = FindParentItem(cursor);

  /* what the cursor has left closes again: in such a group, what is
     open is what the cursor is on, and nothing else */
  const int left = FindParentItem(previous);

  if (left >= 0 && left != here && elements[left].expanded &&
      elements[left].expand_trigger == ExpandTrigger::CURSOR)
    ToggleExpanded((std::size_t)left);

  /* and what it has arrived at opens */
  if (here >= 0 && here == cursor && elements[here].has_children &&
      !elements[here].expanded &&
      elements[here].expand_trigger == ExpandTrigger::CURSOR)
    ToggleExpanded((std::size_t)here);
}

void
GroupedListControl::UpdateGroupFlags() noexcept
{
  for (std::size_t i = 0; i < elements.size(); ++i) {
    Element &element = elements[i];
    if (!element.IsShownItem())
      continue;

    /* a hidden item does not separate the two items around it: they
       are drawn as neighbours of the same group */
    std::size_t j = i;
    while (j > 0 && elements[j - 1].IsItem() && !elements[j - 1].IsShownItem())
      --j;

    element.first_in_group = j == 0 || !elements[j - 1].IsItem();

    j = i;
    while (j + 1 < elements.size() && elements[j + 1].IsItem() &&
           !elements[j + 1].IsShownItem())
      ++j;

    element.last_in_group = j + 1 == elements.size() ||
      !elements[j + 1].IsItem();
  }

  /* a row of buttons cuts a group in two; the edges at the cut are
     rounded less than the outer ones, which keeps the halves
     together.  A group which ends with a row of buttons keeps the
     full rounding: nothing follows which the card belongs to */
  for (std::size_t i = 0; i < elements.size(); ++i) {
    Element &element = elements[i];
    if (!element.IsShownItem())
      continue;

    element.cut_top = element.cut_bottom = false;

    if (element.first_in_group) {
      std::size_t j = i;
      while (j > 0 && (elements[j - 1].IsButtons() ||
                       (elements[j - 1].IsItem() &&
                        !elements[j - 1].IsShownItem())))
        --j;

      element.cut_top = j > 0 && j < i && elements[j - 1].IsShownItem();
    }

    if (element.last_in_group) {
      std::size_t j = i;
      while (j + 1 < elements.size() && (elements[j + 1].IsButtons() ||
                                         (elements[j + 1].IsItem() &&
                                          !elements[j + 1].IsShownItem())))
        ++j;

      element.cut_bottom = j > i && j + 1 < elements.size() &&
        elements[j + 1].IsShownItem();
    }
  }

  /* find the groups which have check marks; all their items reserve
     the room for one, even those which cannot be checked */
  for (std::size_t i = 0; i < elements.size();) {
    if (!elements[i].IsItem()) {
      ++i;
      continue;
    }

    std::size_t end = i;
    bool left = false, right = false, icon = false;

    do {
      const Element &element = elements[end];

      if (element.IsShownItem()) {
        if (element.selection_mode != SelectionMode::NONE) {
          if (element.check_position == CheckPosition::LEFT)
            left = true;
          else
            right = true;
        }

        if (element.HasIcon())
          icon = true;
      }

      ++end;
    } while (end < elements.size() && elements[end].IsItem());

    for (std::size_t j = i; j < end; ++j) {
      elements[j].check_left = left;
      elements[j].check_right = right;
      elements[j].icon_column = icon;
    }

    i = end;
  }
}

int
GroupedListControl::GetRowDecorationWidth(const Element &element)
  const noexcept
{
  /* this mirrors DrawElement(), which lays the decorations out while
     it draws them */

  int width = Layout::VptScale(EDGE_INSET_PT);

  if (element.icon_column)
    width += GetIconWidth();

  if (element.check_left)
    width += Layout::VptScale(EDGE_INSET_PT) + GetCheckWidth();

  return width;
}

int
GroupedListControl::GetDecorationWidth(const Element &element) const noexcept
{
  const int padding = GetPadding();
  const Font &font = *look.list.font;

  int width = GetRowDecorationWidth(element);

  if (element.check_right && !element.has_children)
    /* an item which opens and closes carries no check mark, and its
       arrow moves into the column which the mark would have */
    width += GetCheckWidth();

  if (element.toggle && !element.disabled)
    width += GetToggleWidth() + padding;

  if (element.has_children && !element.disabled)
    /* the arrow which points down or up is as wide as it is tall
       twice over */
    width += 2 * std::max(2, (int)font.GetHeight() / 4) + padding;
  else if (element.chevron && !element.disabled)
    width += std::max(2, (int)font.GetHeight() / 4) + padding;

  if (!element.badge.empty())
    width += (int)GetBadgeFont(element).TextSize(element.badge).width
      + 2 * (int)Layout::VptScale(BADGE_PADDING_PT) + padding;

  return width;
}

int
GroupedListControl::GetFirstRowDecorationHeight(const Element &element)
  const noexcept
{
  int height = 0;

  if (!element.badge.empty())
    height = GetBadgeHeight(element);

  if (element.toggle && !element.disabled)
    height = std::max(height, GetToggleHeight());

  if (element.check_right && !element.has_children)
    height = std::max(height, GetCheckSize());

  return height;
}

/**
 * Break a text into lines, unless the last break was for the same
 * width.  Breaking measures every word of the text, and outside
 * OpenGL it builds a canvas of its own for that; the layout and the
 * paint both need the lines and must not do this twice.
 */
static const WrappedText &
WrapCached(const Font &font, int width, const std::string &text,
           WrappedText &cache, int &cache_width) noexcept
{
  if (cache_width != width) {
    cache = WrapText(font, std::max(width, 1), text);
    cache_width = width;
  }

  return cache;
}

/**
 * The height which a text needs inside a box of the given width: one
 * line is as tall as the font itself, more lines use its line
 * spacing.
 */
static unsigned
GetTextHeight(const Font &font, int width, const std::string &text,
              WrappedText &cache, int &cache_width,
              std::size_t max_lines=MAX_TEXT_LINES) noexcept
{
  const auto &wrapped = WrapCached(font, width, text, cache, cache_width);
  const std::size_t lines = std::min(wrapped.lines.size(), max_lines);

  return lines <= 1
    ? font.GetHeight()
    : (unsigned)lines * font.GetLineSpacing();
}

int
GroupedListControl::UpdateTextLayout(Element &element, int room,
                                     int wide_room) const noexcept
{
  const Font &font = *look.list.font;
  const Font &value_font = GetValueFont(element);
  const int padding = GetPadding();

  element.value_is_below = false;
  element.value_width = 0;
  element.value_height = 0;

  if (element.value.empty() || element.disabled) {
    element.text_height = GetTextHeight(font, room, element.text,
                                        element.wrapped_text,
                                        element.wrapped_text_width);
    return room;
  }

  /* the caption and the value are two columns which share the room,
     with nothing but the padding between them */
  const int available = std::max(room - padding, 2);
  const int caption_natural = (int)font.TextSize(element.text).width;
  const int value_natural = (int)value_font.TextSize(element.value).width;

  /* a value which leaves too little room for the caption moves below
     it, where it has the whole width: two narrow columns which both
     break their words are worse than a taller item */
  const bool below = element.value_below ||
    (caption_natural + value_natural > available &&
     caption_natural > available / 2 && value_natural > available / 2);

  if (below) {
    element.value_width = wide_room;
    element.value_height = GetTextHeight(value_font, wide_room,
                                         element.value,
                                         element.wrapped_value,
                                         element.wrapped_value_width,
                                         GetValueMaxLines(element));
    element.value_is_below = true;
    element.text_height = GetTextHeight(font, room, element.text,
                                        element.wrapped_text,
                                        element.wrapped_text_width);
    return room;
  }

  int value_width;

  if (caption_natural + value_natural <= available ||
      value_natural <= available / 2)
    /* both fit, or the value is the short one: it keeps its width and
       the caption takes the rest */
    value_width = value_natural;
  else
    /* the caption is the short one */
    value_width = available - caption_natural;

  const int caption_width = available - value_width;

  element.value_width = value_width;
  element.value_height = GetTextHeight(value_font, value_width,
                                       element.value,
                                       element.wrapped_value,
                                       element.wrapped_value_width,
                                       GetValueMaxLines(element));
  element.text_height = GetTextHeight(font, caption_width, element.text,
                                      element.wrapped_text,
                                      element.wrapped_text_width);

  return caption_width;
}

void
GroupedListControl::DrawWrappedText(Canvas &canvas, const Font &font,
                                    const PixelRect &rc,
                                    const std::string &text,
                                    const WrappedText &cached,
                                    int cached_width,
                                    bool right,
                                    std::size_t max_lines) const noexcept
{
  canvas.Select(font);

  const int width = std::max((int)rc.GetWidth(), 1);

  /* the layout has broken this text already; break it again only if
     the box has become another one since */
  WrappedText fallback;
  if (cached_width != width)
    fallback = WrapText(font, width, text);

  const WrappedText &wrapped = cached_width == width ? cached : fallback;

  int y = rc.top;

  for (std::size_t i = 0; i < wrapped.lines.size(); ++i) {
    if (i + 1 == max_lines && wrapped.lines.size() > max_lines) {
      /* the last line says that the text goes on */
      const std::string_view rest =
        std::string_view{text}.substr(wrapped.lines[i].start);
      const int ellipsis_width = (int)font.TextSize("…").width;
      const auto tail = WrapText(font, std::max(width - ellipsis_width, 1),
                                 rest);

      std::string last{tail.lines.empty()
                       ? rest
                       : tail.lines.front().GetText(rest)};
      last += "…";

      const int x = right
        ? rc.right - (int)font.TextSize(last).width
        : rc.left;

      canvas.DrawClippedText({x, y}, rc, last);
      return;
    }

    const std::string_view line = wrapped.lines[i].GetText(text);
    const int x = right
      ? rc.right - (int)font.TextSize(line).width
      : rc.left;

    canvas.DrawClippedText({x, y}, rc, line);

    y += (int)font.GetLineSpacing();
  }
}

void
GroupedListControl::UpdateLayout() noexcept
{
  FinishGroup();
  PrepareIcons();
  PrepareWidgets();

  if (restore_expanded) {
    /* the page has been filled again: open the items which were open
       before, counted in item indexes, which do not move when
       something opens or closes */
    restore_expanded = false;

    unsigned n = 0;
    for (auto &element : elements) {
      if (!element.IsItem())
        continue;

      if (element.depth == 0)
        element.expanded = std::find(saved_expanded.begin(),
                                     saved_expanded.end(),
                                     n) != saved_expanded.end();

      ++n;
    }
  }

  UpdateExpansion();
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
    cursor = saved_buttons
      ? FindButtonsByIndex(saved_cursor)
      : FindItemByIndex(saved_cursor);

    if (cursor < 0)
      cursor = FindItem(elements.size(), false);
    else if (!elements[cursor].IsSelectable())
      cursor = FindItem(cursor, true);

    if (cursor >= 0 && elements[cursor].IsButtons())
      /* and the same button of that row, as long as it is still
         there and can be pressed */
      button_column = saved_column < elements[cursor].buttons.size() &&
        !elements[cursor].buttons[saved_column].disabled
        ? saved_column
        : GetFirstButton(elements[cursor]);
  }

  saved_cursor = -1;
  saved_buttons = false;

  if (cursor >= 0 && (std::size_t)cursor < elements.size() &&
      !elements[cursor].IsSelectable())
    /* the item under the cursor has been hidden or disabled */
    cursor = FindItem(cursor, true);

  if (cursor < 0 && !cursor_removed)
    cursor = FindItem(0, true);

  const int margin = GetCardMargin();
  const int caption_gap = Layout::VptScale(CAPTION_GAP_PT);
  const int footer_gap = Layout::VptScale(FOOTER_GAP_PT);

  const int padding = GetPadding();

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
      case Element::Type::ITEM: {
        if (!element.IsShownItem()) {
          /* hidden by the page, or a child whose parent is closed */
          element.height = 0;
          break;
        }

        const unsigned font_height = look.list.font->GetHeight();

        /* the room which the caption, the value and the second line
           share, once the decorations have taken theirs */
        const int room = std::max(text_width
                                  - GetDecorationWidth(element), 1);

        /* the rows below the first one of an item which is a stack
           of rows: only the decorations at the left edge take from
           their width */
        const bool rows = element.HasRows();
        const int wide_room = rows
          ? std::max(text_width - GetRowDecorationWidth(element), 1)
          : room;

        const int caption_width = UpdateTextLayout(element, room,
                                                   wide_room);

        element.subtitle_height = element.subtitle.empty()
          ? 0
          : text_renderer.GetHeight(GetSubtitleFont(element),
                                    std::max(rows ? wide_room
                                             : caption_width, 1),
                                    element.subtitle.c_str());

        element.description_height = rows
          ? GetTextHeight(GetDescriptionFont(element), wide_room,
                          element.description,
                          element.wrapped_description,
                          element.wrapped_description_width,
                          GetDescriptionMaxLines(element))
          : 0;

        const unsigned subtitle_lines = element.subtitle_height
          / GetSubtitleFont(element).GetLineSpacing();
        const unsigned value_lines = element.value_height
          / GetValueFont(element).GetLineSpacing();

        /* the caption, its second line and a value below them are one
           block; a value beside them is a block of its own */
        unsigned block = element.text_height;
        unsigned lines = std::max(1u, element.text_height
                                  / look.list.font->GetLineSpacing());

        element.first_row_height = 0;

        if (rows) {
          /* what stands at the right edge shares the first row with
             the caption; a value below it, the description and the
             subtitle are rows of their own, over the whole width */
          unsigned row = element.text_height;

          if (!element.value_is_below && element.value_height > row) {
            row = element.value_height;
            lines = value_lines;
          }

          row = std::max(row,
                         (unsigned)GetFirstRowDecorationHeight(element));

          element.first_row_height = row;
          block = row;

          if (element.value_is_below) {
            block += subtitle_gap + element.value_height;
            lines += value_lines;
          }

          block += subtitle_gap + element.description_height;
          lines += element.description_height
            / GetDescriptionFont(element).GetLineSpacing();

          if (element.subtitle_height > 0) {
            block += subtitle_gap + element.subtitle_height;
            lines += subtitle_lines;
          }
        } else {
          if (element.subtitle_height > 0) {
            block += subtitle_gap + element.subtitle_height;
            lines += subtitle_lines;
          }

          if (element.value_is_below) {
            block += subtitle_gap + element.value_height;
            lines += value_lines;
          } else if (element.value_height > block) {
            block = element.value_height;
            lines = value_lines;
          }
        }

        if (block == font_height) {
          /* one line, like most items */
          element.height = item_height;
          break;
        }

        /* the room above and below grows with the number of lines:
           with the padding of a one-line item, a tall item would sit
           cramped between the two separators.  It never grows beyond
           the room which a one-line item has, where the minimum
           height of a touch target is what pads the caption */
        const unsigned vertical_padding =
          std::min(Layout::GetTextPadding() * lines,
                   (item_height - font_height) / 2);

        element.height = std::max(block + 2 * vertical_padding,
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

      case Element::Type::BUTTONS:
        element.height = GetButtonsLeadingGap(i)
          + GetButtonLines(element) * item_height;

        /* the description belongs to the buttons and is always on the
           screen, so that a button which is greyed out can say why */
        if (!element.text.empty())
          element.height += caption_gap +
            text_renderer.GetHeight(*look.list.font,
                                    std::max(text_width, 1),
                                    element.text.c_str());

        /* the group goes on below this row: the card which continues
           it keeps the distance which the row keeps to the card
           above it */
        if (i + 1 < elements.size() && elements[i + 1].IsItem())
          element.height += footer_gap;

        break;

      case Element::Type::WIDGET:
        element.height = GetWidgetHeight(element);

        /* the caption of the group carries the gap above it; without
           one, the view keeps the distance itself */
        if (i == 0 || elements[i - 1].type != Element::Type::CAPTION)
          element.height += GetLeadingGap(i);

        break;

      case Element::Type::FOOTER: {
        const auto footer = GetFooter(i);

        /* the cursor decides which text this is, so it may be another
           one than the last time: break it again in any case */
        element.wrapped_text = WrapText(*look.list.font,
                                        std::max(text_width, 1),
                                        *footer.text);
        element.wrapped_text_width = text_width;

        element.height = footer_gap +
          element.wrapped_text.lines.size() *
          look.list.font->GetLineSpacing();
      }
        break;
      }

      element.top = y;
      y += (int)element.height;
    }

    content_height = elements.empty()
      ? 0
      : (unsigned)(y + (int)Layout::VptScale(MARGIN_PT));

    if (scroll_bar.IsDefined() ||
        content_height <= (unsigned)GetViewHeight())
      break;

    scroll_bar.SetSize(GetSize());
  }

  SetOrigin(origin);

  MoveWidgets();

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

  MoveWidgets();

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
GroupedListControl::ScrollAhead(unsigned i, bool forward) noexcept
{
  assert(i < elements.size());

  const Element &element = elements[i];
  const int room = GetViewHeight();

  /* the items which follow are worth a look, and with them whatever
     lies between them: the caption of the next group belongs to what
     comes.  How many of them fit is not a question of their number
     but of the room they ask for */
  const int edge = forward ? element.GetBottom() : element.top;

  int ahead = edge;
  unsigned items_ahead = 0;

  for (int j = (int)i; items_ahead < std::size(LOOK_AHEAD_PERCENT);) {
    j += forward ? 1 : -1;
    if (j < 0 || (std::size_t)j >= elements.size())
      break;

    const Element &next = elements[j];
    const int next_edge = forward ? next.GetBottom() : next.top;

    /* how far the view would have to scroll to show this element */
    const int scroll = forward ? next_edge - edge : edge - next_edge;

    if (scroll > room * (int)LOOK_AHEAD_PERCENT[items_ahead] / 100)
      break;

    ahead = next_edge;

    if (next.IsShownItem() || next.IsButtons())
      ++items_ahead;
  }

  /* the item under the cursor stays on the screen as a whole */
  if (forward) {
    const int target = std::min(ahead - room, element.top);
    if (target > origin)
      SetOrigin(target);
  } else {
    const int target = std::max(ahead, element.GetBottom() - room);
    if (target < origin)
      SetOrigin(target);
  }
}

void
GroupedListControl::SetCursor(int i) noexcept
{
  if (i < 0 || i == cursor)
    return;

  const int previous = cursor;

  cursor = i;
  cursor_removed = false;

  if (elements[i].IsButtons())
    /* the cursor arrives on the button it would reach first, from
       wherever it comes */
    button_column = GetFirstButton(elements[i]);

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

  /* a group whose items open under the cursor does it now, after the
     cursor has arrived and before anybody is told about it */
  UpdateCursorExpansion(previous);

  if (cursor_callback)
    cursor_callback(GetCursorIndex());
}

void
GroupedListControl::SetCursorByIndex(unsigned i) noexcept
{
  const int j = FindItemByIndex(i);
  if (j >= 0)
    SetCursor(j);
  else {
    /* there is no such item yet: the list is being filled, and the
       next UpdateLayout() moves the cursor there */
    saved_cursor = i;
    cursor_removed = false;
  }
}

void
GroupedListControl::ClearCursor() noexcept
{
  if (cursor < 0)
    return;

  cursor = -1;

  /* the list is not being rebuilt: the cursor is meant to be gone,
     and UpdateLayout() must not bring it back */
  saved_cursor = -1;
  cursor_removed = true;

  /* the footer of the group showed the help of that item; the
     elements below it move up */
  UpdateLayout();

  if (cursor_callback)
    cursor_callback(GetCursorIndex());
}

void
GroupedListControl::MoveCursor(bool forward) noexcept
{
  const int next = FindItem(GetSearchStart(forward), forward);
  if (next < 0)
    return;

  SetCursor(next);

  /* the view follows the cursor with a margin, instead of waiting
     until it is at the edge: this way the list shows where the next
     presses lead before the cursor gets there */
  ScrollAhead(next, forward);
}

bool
GroupedListControl::MoveButtonColumn(bool forward) noexcept
{
  if (cursor < 0 || (std::size_t)cursor >= elements.size())
    return false;

  const Element &element = elements[cursor];
  if (!element.IsButtons())
    return false;

  for (int i = (int)button_column + (forward ? 1 : -1);
       i >= 0 && i < (int)element.buttons.size();
       i += forward ? 1 : -1) {
    if (element.buttons[i].disabled)
      continue;

    button_column = (unsigned)i;
    Invalidate();
    return true;
  }

  return false;
}

void
GroupedListControl::MoveCursorPage(bool forward) noexcept
{
  const int room = GetViewHeight();

  /* the view moves by exactly one screen, whatever is on it: this is
     what makes the two keys a reliable way through a long page, and
     the cursor is what follows, not the other way round */
  SetOrigin(forward ? origin + room : origin - room);

  /* the item the next press starts from: at the lower edge of the new
     view when going down, at the upper one when going up, so that the
     next press carries on where this one has stopped */
  int next = -1;

  for (std::size_t i = 0; i < elements.size(); ++i) {
    const Element &element = elements[i];
    if (!element.IsSelectable())
      continue;

    if (element.top < origin || element.GetBottom() > origin + room)
      /* only an item which is there as a whole; half of one at the
         edge of the view is not where a cursor belongs */
      continue;

    next = (int)i;

    if (!forward)
      break;
  }

  if (next < 0)
    /* a screen full of an explanation, a hero card or a view: there
       is nothing to point at */
    ClearCursor();
  else if (next != cursor)
    SetCursor(next);
}

std::pair<int, int>
GroupedListControl::GetToggleHitArea(const Element &element) const noexcept
{
  if (!element.toggle || element.disabled)
    return {0, 0};

  /* the switch sits at the right edge of the item, inside the margin
     of the card and its padding; only the check mark of a group is
     further out */
  const int padding = GetPadding();

  int right = GetContentWidth() - GetCardMargin() - padding;
  if (element.check_right)
    right -= GetCheckWidth();

  const int left = right - GetToggleWidth();

  /* a finger is wider than the switch: let the padding around it
     count, so that a tap next to it is not lost */
  return {left - padding, right + padding};
}

void
GroupedListControl::ActivateItem() noexcept
{
  if (cursor < 0 || (std::size_t)cursor >= elements.size())
    return;

  Element &element = elements[cursor];
  if (element.disabled)
    return;

  if (element.has_children) {
    /* the item is a parent: it opens and closes instead of acting,
       and its callback is never called */
    ToggleExpanded((std::size_t)cursor);
    return;
  }

  if (element.IsButtons()) {
    if (button_column >= element.buttons.size())
      return;

    const ElementButton &button = element.buttons[button_column];
    if (button.disabled)
      return;

    /* the button is drawn pressed and the release does not reach the
       screen before the next repaint: show it now, the callback may
       take a while */
    if (auto *top = dynamic_cast<UI::TopWindow *>(GetRootOwner()))
      top->Refresh();

    /* a copy, because the callback may rebuild the list */
    if (auto callback = button.callback)
      callback();

    return;
  }

  if (element.toggle) {
    /* the switch is the state of this item; the check mark of the
       group does not apply to it */
    element.checked = !element.checked;
    Invalidate();
  } else switch (element.selection_mode) {
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
  if (pressed && !IsDithered())
    /* zinc-200 and zinc-700 of the Tailwind palette; a phone dims the
       row under the finger instead of coloring it.  A dithered
       display has no dim gray and takes the color of the look */
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
GroupedListControl::DrawRoundedCorner(Canvas &canvas, const PixelRect &rc,
                                      bool top, Color color, int radius,
                                      int thickness) noexcept
{
  /* the arc is the ring between two circles, the one which
     DrawRoundedEdge() has cut and one which is thinner by the
     thickness of the line.  Drawing it row by row from the outer
     circle towards the inner one keeps the line the same width all
     the way around; following the outer circle alone would make it
     bulge wherever two neighbouring rows begin at the same pixel */
  const int inner_radius = std::max(0, radius - thickness);

  for (int i = 0; i < radius; ++i) {
    /* the distance of the middle of this pixel row from the center of
       the circles */
    const double dy = radius - i - 0.5;

    const int outer = (int)std::lround(radius -
                                       std::sqrt(radius * radius - dy * dy));

    /* a row above the inner circle belongs to the line as a whole,
       from the outer circle to where the straight edge begins */
    const int inner = dy < inner_radius
      ? (int)std::lround(radius -
                         std::sqrt(inner_radius * inner_radius - dy * dy))
      : radius;

    if (inner <= outer)
      continue;

    const int y = top ? rc.top + i : rc.bottom - 1 - i;

    canvas.DrawFilledRectangle({rc.left + outer, y, rc.left + inner, y + 1},
                               color);
    canvas.DrawFilledRectangle({rc.right - inner, y, rc.right - outer, y + 1},
                               color);
  }
}

void
GroupedListControl::DrawCardBorder(Canvas &canvas, const PixelRect &rc,
                                   bool first, bool last, Color color,
                                   int top_radius,
                                   int bottom_radius) noexcept
{
  const int thickness = GetSeparatorThickness();

  /* the two sides; the rows of a rounded corner get their pixels from
     DrawRoundedCorner() instead */
  const int top = rc.top + (first ? top_radius : 0);
  const int bottom = rc.bottom - (last ? bottom_radius : 0);

  canvas.DrawFilledRectangle({rc.left, top, rc.left + thickness, bottom},
                             color);
  canvas.DrawFilledRectangle({rc.right - thickness, top, rc.right, bottom},
                             color);

  if (first)
    canvas.DrawFilledRectangle({rc.left + top_radius, rc.top,
                                rc.right - top_radius, rc.top + thickness},
                               color);

  /* the lower edge of the card, or the line which separates this item
     from the next one */
  const int inset = last ? bottom_radius : 0;
  canvas.DrawFilledRectangle({rc.left + inset, rc.bottom - thickness,
                              rc.right - inset, rc.bottom},
                             color);

  if (first)
    DrawRoundedCorner(canvas, rc, true, color, top_radius, thickness);

  if (last)
    DrawRoundedCorner(canvas, rc, false, color, bottom_radius, thickness);
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

/** The colors of the switch which shows a boolean value. */
struct ToggleColors {
  /** the pill behind the thumb */
  Color track_color;

  /** the thumb which sits at one of its ends */
  Color thumb_color;

  /** the outline of the pill; the same as #track_color draws none */
  Color outline_color;
};

[[gnu::pure]]
static ToggleColors
GetToggleColors(const DialogLook &look, bool checked,
                Color background_color, Color text_color) noexcept
{
  if (IsDithered())
    /* a display which knows two colors has nothing but the two colors
       of the row it sits on: the switch which is on is filled with
       the color of the text, the one which is off shows the row
       through it and draws its outline instead.  Both of them turn
       around with the item under the cursor, as its icon does.  Gray
       levels are enough for the colors below */
    return {checked ? text_color : background_color,
            checked ? background_color : text_color,
            text_color};

  if (checked)
    /* green-500 of the Tailwind palette: it is much lighter and more
       saturated than the accent blue which the item under the cursor
       wears, and it keeps its distance from that blue in hue as well.
       The darker green of a badge sits too close to it: on the item
       under the cursor the two read as one muddy color */
    return {Color(0x22, 0xc5, 0x5e), COLOR_WHITE, Color(0x22, 0xc5, 0x5e)};

  /* zinc-600 and zinc-300 of the same palette */
  const Color track = look.dark_mode
    ? Color(0x52, 0x52, 0x5b)
    : Color(0xd4, 0xd4, 0xd8);

  return {track, COLOR_WHITE, track};
}

void
GroupedListControl::DrawToggle(Canvas &canvas, const PixelRect &rc,
                               bool checked, const DialogLook &look,
                               Color background_color,
                               Color text_color) noexcept
{
  const auto colors = GetToggleColors(look, checked,
                                      background_color, text_color);

  /* the pill is a rectangle between two half circles; other than a
     rounded rectangle whose corners are painted over, this touches no
     pixel outside the switch, and the background may be anything.

     Only the circles take their color from the brush; it and the pen
     have a name because a temporary would be gone at the semicolon,
     and a canvas which selects the object itself rather than a copy
     of it would draw with a deleted one.  The pen paints the rim of
     the circle: give it the color of the brush, because a null pen is
     black and the rim would be a frayed dark edge */
  const auto draw_pill = [&canvas](const PixelRect &r, Color color){
    const int radius = (int)r.GetHeight() / 2;
    const int centre_y = r.top + radius;

    canvas.DrawFilledRectangle({r.left + radius, r.top,
                                r.right - radius, r.bottom},
                               color);

    const Brush brush(color);
    const Pen pen(0, color);
    canvas.Select(brush);
    canvas.Select(pen);

    canvas.DrawCircle({r.left + radius, centre_y}, radius);
    canvas.DrawCircle({r.right - radius, centre_y}, radius);
  };

  if (colors.outline_color == colors.track_color)
    draw_pill(rc, colors.track_color);
  else {
    /* the switch which is off has the color of the row behind it:
       draw it one line larger in the color of the outline, and let
       the track cover all but that line */
    draw_pill(rc, colors.outline_color);

    PixelRect inner = rc;
    inner.Grow(-GetSeparatorThickness());
    draw_pill(inner, colors.track_color);
  }

  const int radius = (int)rc.GetHeight() / 2;
  const int inset = std::max(1, (int)Layout::VptScale(TOGGLE_INSET_PT));
  const int thumb_radius = radius - inset;

  const Brush thumb_brush(colors.thumb_color);
  const Pen thumb_pen(0, colors.thumb_color);
  canvas.Select(thumb_brush);
  canvas.Select(thumb_pen);
  canvas.DrawCircle({checked
                     ? rc.right - radius
                     : rc.left + radius,
                     rc.top + radius}, thumb_radius);
}

void
GroupedListControl::DrawElement(Canvas &canvas, std::size_t i,
                                PixelRect rc) const noexcept
{
  const Element &element = elements[i];

  const int margin = GetCardMargin();
  const int padding = GetPadding();

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

    /* the item under the finger is drawn pressed while the one the
       cursor is on keeps its color: the finger has not chosen
       anything yet, and a scroll gesture chooses nothing at all */
    const bool pressed = (int)i == pressed_element;
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

    /* a child wears a bar at the left edge of the card instead of an
       indent; the bars of the children of one item stand below each
       other and become one.  The corner which DrawRoundedEdge() takes
       away below takes the end of the bar with it, so that it cannot
       reach out of the card */
    if (element.depth > 0 && !selected) {
      /* the item under the cursor wears no bar: its background is the
         accent color itself, and a bar would only cut an edge into
         it.  A display which knows two colors has no accent color and
         takes the color of the text */
      const Color bar_color = IsDithered()
        ? row_colors.text_color
        : (look.dark_mode ? COLOR_XCSOAR_LIGHT : COLOR_XCSOAR);

      canvas.DrawFilledRectangle({background_rc.left, background_rc.top,
                                  background_rc.left + GetChildBarWidth(),
                                  background_rc.bottom},
                                 bar_color);
    }

    const int radius = GetCardRadius();

    /* an edge which a row of buttons has cut into the group is
       barely rounded: the group goes on beyond it */
    const int cut_radius = (int)std::lround(radius * CUT_RADIUS_FACTOR);
    const int top_radius = element.cut_top ? cut_radius : radius;
    const int bottom_radius = element.cut_bottom ? cut_radius : radius;

    if (element.first_in_group)
      DrawRoundedEdge(canvas, rc, true, look.background_color, top_radius);

    if (element.last_in_group)
      DrawRoundedEdge(canvas, rc, false, look.background_color,
                      bottom_radius);

    /* a display which knows two colors paints the card in the same
       white as the page behind it: draw its edge, or there would be
       no card at all.  The line to the next item is part of it */
    if (IsDithered())
      DrawCardBorder(canvas, rc, element.first_in_group,
                     element.last_in_group, look.list.text_color,
                     top_radius, bottom_radius);

    /* a thin line separates the items of a card; like the gap between
       two cards, it shows the page behind them.  The selected item
       needs no line, its background already separates it from its
       neighbours */
    if (!element.last_in_group && !selected && !IsDithered()) {
      /* the line begins where the caption does, like the lists of a
         phone; the check mark is a state of the whole item and stays
         outside */
      const int left = text_rc.left +
        (element.icon_column ? GetIconWidth() : 0);

      canvas.DrawFilledRectangle({left, rc.bottom - separator,
                                  text_rc.right, rc.bottom},
                                 look.background_color);
    }

    canvas.Select(*look.list.font);

    const Color plain_text_color = row_colors.text_color;

    /* an item which is not available keeps its background, but all of
       its contents fade towards it.  A display which knows two colors
       has no faded color: it would dither the text into a pattern
       which is hard to read, and the badge of the item says clearly
       enough that it cannot be reached */
    const Color text_color = element.disabled && !IsDithered()
      ? (look.dark_mode
         ? DarkColor(plain_text_color)
         : LightColor(plain_text_color))
      : plain_text_color;

    canvas.SetTextColor(text_color);

    const int font_height = look.list.font->GetHeight();
    const int centre_y = text_rc.top + (int)text_rc.GetHeight() / 2;

    /* the caption, its second line and a value below them are
       centred as one block */
    const int gap = Layout::VptScale(SUBTITLE_GAP_PT);

    /* an item which is a stack of rows keeps what stands at the right
       edge in its first row, so that the rows below it have the whole
       width */
    const bool rows = element.HasRows();
    const int row_height = rows
      ? (int)element.first_row_height
      : (int)element.text_height;

    int block_height = row_height;

    if (element.value_is_below)
      block_height += gap + (int)element.value_height;

    if (rows)
      block_height += gap + (int)element.description_height;

    if (!element.subtitle.empty())
      block_height += gap + (int)element.subtitle_height;

    const int text_y = text_rc.top
      + ((int)text_rc.GetHeight() - block_height) / 2;

    /* the caption sits in the middle of the first row, which a badge
       may make taller than the caption is */
    const int caption_y = text_y
      + (row_height - (int)element.text_height) / 2;

    /* the value belongs to the caption and comes right after it; the
       description follows, and the second line, which explains the
       item, closes the block */
    const int value_y = text_y + row_height + gap;
    const int description_y = element.value_is_below
      ? value_y + (int)element.value_height + gap
      : value_y;
    const int subtitle_y = rows
      ? description_y + (int)element.description_height + gap
      : description_y;

    /* the middle of the row which holds what stands at the right
       edge: the item itself, or its first row */
    const int row_centre_y = rows ? text_y + row_height / 2 : centre_y;

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

      /* an item which carries a switch keeps the room of the column,
         so that all captions of the group stay aligned, but the
         switch says its state, not a check mark */
      if (element.checked && !element.disabled && !element.toggle &&
          !element.has_children) {
        PixelRect check_rc;
        check_rc.left = left
          ? caption_rc.left
          : caption_rc.right - size;
        check_rc.right = check_rc.left + size;
        check_rc.top = (left ? centre_y : row_centre_y) - size / 2;
        check_rc.bottom = check_rc.top + size;

        DrawCheck(canvas, check_rc, text_color);
      }

      /* the column at the left edge is what the captions of the group
         line up behind, so every item keeps it; the one at the right
         edge is only room for a mark, and an item which opens and
         closes has none to show */
      if (element.check_left)
        caption_rc.left += GetCheckWidth();

      if (element.check_right && !element.has_children)
        caption_rc.right -= GetCheckWidth();
    }

    if (element.toggle && !element.disabled) {
      const int width = GetToggleWidth();
      const int height = GetToggleHeight();

      PixelRect toggle_rc;
      toggle_rc.right = caption_rc.right;
      toggle_rc.left = toggle_rc.right - width;
      toggle_rc.top = row_centre_y - height / 2;
      toggle_rc.bottom = toggle_rc.top + height;

      DrawToggle(canvas, toggle_rc, element.checked, look,
                 background, plain_text_color);

      caption_rc.right = toggle_rc.left - padding;
    }

    /* an item which is not available shows neither the arrow nor its
       value: the arrow would promise a page which does not open, and
       the state is not in effect anyway.  What is left is the caption
       and the badge which says why */
    if (element.icon_column) {
      const int size = GetIconSize();

      const PixelPoint centre{caption_rc.left + size / 2, centre_y};

      if (element.icon != nullptr) {
        const PixelSize icon_size = element.icon->GetSize();

        if ((int)icon_size.width <= size && (int)icon_size.height <= size) {
          /* the icon keeps the size which it was loaded for: scaling
             it a second time, to a size which is not the one the icon
             system has chosen for this display, is what makes it
             blurry.  This is also the overload which turns a dark
             icon around on a dark background */
          const PixelRect icon_rc{centre.x - size / 2, centre.y - size / 2,
                                  centre.x + size / 2, centre.y + size / 2};

          element.icon->Draw(canvas, icon_rc, false);
        } else {
          /* it does not fit: a wide icon gets a smaller height, so
             that it does not grow out of the column */
          const unsigned height = icon_size.width > icon_size.height
            ? size * icon_size.height / icon_size.width
            : size;

          element.icon->Draw(canvas, centre, height);
        }
      } else if (element.icon_image != nullptr) {
#ifdef ENABLE_OPENGL
        /* the image is transparent around the glyph */
        const ScopeAlphaBlend alpha_blend;
#endif

        canvas.Stretch({centre.x - size / 2, centre.y - size / 2},
                       {size, size}, *element.icon_image);
      } else if (element.icon_glyph) {
        /* a character which stands in for an icon is drawn as large
           as the column, not as large as the text beside it: the
           heading font is the 3/2 of the list font which #GetIconSize
           asks for */
        canvas.Select(look.heading1_font);

        const int width = canvas.CalcTextWidth(element.icon_text.c_str());
        const int glyph_height = (int)look.heading1_font.GetHeight();

        canvas.DrawText({centre.x - width / 2, centre_y - glyph_height / 2},
                        element.icon_text.c_str());

        canvas.Select(*look.list.font);
      }

      caption_rc.left += GetIconWidth();
    }

    if ((element.chevron || element.has_children) && !element.disabled) {
      const int size = std::max(2, font_height / 4);

      const Pen pen(Layout::ScalePenWidth(1), text_color);
      canvas.Select(pen);

      if (element.has_children) {
        /* an arrow which points down or up, never to the right: the
           arrow to the right belongs to an item which opens another
           page, and these two must not look alike.  Down says that
           the children come out below, up that they go back in */
        const int x = caption_rc.right - size;
        const int y = row_centre_y
          + (element.expanded ? size / 2 : -size / 2);
        const int dy = element.expanded ? -size : size;

        canvas.DrawLine({x - size, y}, {x, y + dy});
        canvas.DrawLine({x, y + dy}, {x + size, y});

        caption_rc.right -= 2 * size + padding;
      } else {
        canvas.DrawLine({caption_rc.right - size, row_centre_y - size},
                        {caption_rc.right, row_centre_y});
        canvas.DrawLine({caption_rc.right, row_centre_y},
                        {caption_rc.right - size, row_centre_y + size});

        caption_rc.right -= size + padding;
      }
    }

    /* the value keeps a box of its own, which begins where the
       caption ends; it never covers the caption */
    PixelRect value_rc{};

    if (element.value_width > 0 && !element.value_is_below) {
      value_rc = caption_rc;
      value_rc.left = value_rc.right - (int)element.value_width;

      /* it sits in the middle of the item, like the badge and the
         arrow, even when the caption has been pushed up by a second
         line; in the middle of the first row of a stack of rows */
      value_rc.top = row_centre_y - (int)element.value_height / 2;
      value_rc.bottom = value_rc.top + (int)element.value_height;

      caption_rc.right = value_rc.left - padding;
    }

    if (!element.badge.empty()) {
      /* a short label on a filled rounded box; on the selected item
         the colors are swapped, where the accent color is the
         background of the item itself.  On an item which is not
         available, the box is grey, like its text */
      const int badge_pad_x = Layout::VptScale(BADGE_PADDING_PT);

      const Font &badge_font = GetBadgeFont(element);
      canvas.Select(badge_font);

      const int badge_height = GetBadgeHeight(element);

      PixelRect badge_rc;
      badge_rc.right = caption_rc.right;
      badge_rc.left = badge_rc.right
        - canvas.CalcTextWidth(element.badge.c_str()) - 2 * badge_pad_x;
      badge_rc.top = row_centre_y - badge_height / 2;
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

      /* the text is centered by its box, which is what the font
         renders into.  Aiming at the capitals instead would look
         better for a short label, but it needs the distance from the
         top of that box to the baseline, and a font whose ascent does
         not include its leading - Courier on macOS - does not tell
         it: the label would sit low by that leading */
      const int badge_text_y = badge_rc.top
        + (badge_height - (int)badge_font.GetHeight()) / 2;

      canvas.DrawClippedText({badge_rc.left + badge_pad_x, badge_text_y},
                             badge_rc, element.badge.c_str());

      caption_rc.right = badge_rc.left - padding;

      canvas.Select(*look.list.font);
      canvas.SetTextColor(text_color);
    }

    /* the rows below the first one run up to the right edge, below
       what stands there */
    PixelRect wide_rc = caption_rc;
    wide_rc.right = text_rc.right - edge_inset;

    if (element.value_width > 0) {
      if (element.value_is_below) {
        /* below the caption, the value has the whole width */
        value_rc = rows ? wide_rc : caption_rc;
        value_rc.top = value_y;
        value_rc.bottom = value_y + (int)element.value_height;
      }

      DrawWrappedText(canvas, GetValueFont(element), value_rc,
                      element.value, element.wrapped_value,
                      element.wrapped_value_width, true,
                      GetValueMaxLines(element));
    }

    PixelRect text_box = caption_rc;
    text_box.top = caption_y;
    text_box.bottom = caption_y + (int)element.text_height;

    DrawWrappedText(canvas, *look.list.font, text_box, element.text,
                    element.wrapped_text, element.wrapped_text_width, false);

    if (rows) {
      PixelRect description_rc = wide_rc;
      description_rc.top = description_y;
      description_rc.bottom = description_y
        + (int)element.description_height;

      DrawWrappedText(canvas, GetDescriptionFont(element), description_rc,
                      element.description, element.wrapped_description,
                      element.wrapped_description_width, false,
                      GetDescriptionMaxLines(element));
    }

    if (!element.subtitle.empty()) {
      canvas.Select(GetSubtitleFont(element));

      PixelRect subtitle_rc = rows ? wide_rc : caption_rc;
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

    const int radius = GetCardRadius();
    DrawRoundedEdge(canvas, card_rc, true, look.background_color, radius);
    DrawRoundedEdge(canvas, card_rc, false, look.background_color, radius);

    /* this card is as white as the page, too */
    if (IsDithered())
      DrawCardBorder(canvas, card_rc, true, true, look.list.text_color,
                     radius, radius);

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

  case Element::Type::BUTTONS: {
    const int top = rc.top + GetButtonsLeadingGap(i);

    for (std::size_t j = 0; j < element.buttons.size(); ++j) {
      const ElementButton &button = element.buttons[j];

      /* the buttons share the width of the card; the renderer keeps
         the gap between two of them and towards the edges */
      /* the cursor is shown on a button only where keys move it, as
         #Button does: on a touch screen, a button which stays marked
         after the tap would look pressed */
      const ButtonState state = button.disabled
        ? ButtonState::DISABLED
        : (int)i == pressed_element && (int)j == pressed_column
        ? ButtonState::PRESSED
        : (int)i == cursor && j == button_column && HasCursorKeys()
        ? (HasFocus() ? ButtonState::FOCUSED : ButtonState::SELECTED)
        : ButtonState::ENABLED;

      button.renderer.DrawButton(canvas, GetButtonRect(element, j, rc, top),
                                 state);
    }

    if (!element.text.empty()) {
      PixelRect description_rc = text_rc;
      description_rc.top = top
        + (int)(GetButtonLines(element) * GetItemHeight())
        + (int)Layout::VptScale(CAPTION_GAP_PT);

      canvas.Select(*look.list.font);
      canvas.SetTextColor(look.text_color);
      canvas.SetBackgroundTransparent();
      text_renderer.Draw(canvas, description_rc, element.text.c_str());
    }
  }
    break;

  case Element::Type::FOOTER: {
    canvas.Select(*look.list.font);

    /* the same color which the rich text of the manual uses for its
       links */
    const Color link_color = look.dark_mode
      ? COLOR_XCSOAR_LIGHT
      : COLOR_XCSOAR;

    WalkFooter(i, [this, &canvas, link_color](PixelRect piece_rc,
                                                std::string_view piece,
                                                const Link *link){
      canvas.SetTextColor(link != nullptr ? link_color : look.text_color);
      canvas.DrawText(piece_rc.GetTopLeft(), piece);

      if (link != nullptr)
        canvas.DrawHLine(piece_rc.left, piece_rc.right,
                         piece_rc.top + look.list.font->GetAscentHeight() + 1,
                         link_color);
    });
  }
    break;

  case Element::Type::WIDGET:
    /* the view is a child window: it paints itself */
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
    if (element.height == 0)
      /* a hidden item */
      continue;

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

  {
#ifdef ENABLE_OPENGL
    /* a view which is half scrolled out must not reach beyond the
       list: its window is not clipped by this one, and it would be
       drawn over the dialog above it and over the widget below it */
    const GLCanvasScissor scissor(PixelRect{PixelPoint{0, 0},
                                            canvas.GetSize()});
#endif

    /* the views of the widget groups are child windows */
    ContainerWindow::OnPaint(canvas);
  }

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

  /* the views are child windows of this one */
  for (auto &element : elements)
    UnprepareWidget(element);

  ContainerWindow::OnDestroy();
}

void
GroupedListControl::OnResize(PixelSize new_size) noexcept
{
  ContainerWindow::OnResize(new_size);

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
    pressed_element = -1;
    ReleaseCapture();
    Invalidate();
  }

  scroll_bar.DragEnd(this);

  PaintWindow::OnCancelMode();
}

bool
GroupedListControl::OnMouseDown(PixelPoint p) noexcept
{
  /* the view of a widget group is a child window: it gets the press
     before the list turns it into a drag */
  if (ContainerWindow::OnMouseDown(p))
    return true;

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
  const int column = i >= 0 && elements[i].IsButtons()
    ? FindButtonAt(i, p)
    : -1;

  if (i >= 0 && elements[i].IsButtons()
      ? column >= 0 && !elements[i].buttons[column].disabled
      : i >= 0 && elements[i].IsSelectable()) {
    /* the item is drawn pressed right away, so that the finger gets
       an answer; the cursor follows only when the finger is lifted
       without having wandered off.  Moving it here would select an
       item on every scroll gesture, and the explanation below the
       group would jump while the list is still moving */
    pressed_element = i;
    pressed_column = column;
    drag_mode = DragMode::CURSOR;
    Invalidate();
  } else
    drag_mode = DragMode::SCROLL;

  SetCapture();
  return true;
}

bool
GroupedListControl::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  if (drag_mode == DragMode::NONE && !scroll_bar.IsDragging() &&
      ContainerWindow::OnMouseMove(p, keys))
    /* the press began on a view of a widget group */
    return true;

  if (scroll_bar.IsDragging()) {
    SetOrigin(scroll_bar.DragMove(content_height, GetViewHeight(), p.y));
    return true;
  }

  if (drag_mode == DragMode::NONE)
    return false;

  if (drag_mode == DragMode::CURSOR &&
      std::abs(p.y - drag_start_y) > Layout::Scale(8)) {
    /* the finger has moved too far: this is a scroll gesture, not a
       tap on an item.  The item loses its pressed color, and the
       item which the cursor was on all the while keeps it */
    drag_mode = DragMode::SCROLL;
    pressed_element = -1;
    pressed_column = -1;
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
  if (drag_mode == DragMode::NONE && !scroll_bar.IsDragging() &&
      ContainerWindow::OnMouseUp(p))
    return true;

  if (scroll_bar.IsDragging()) {
    scroll_bar.DragEnd(this);
    return true;
  }

  /* the press has remembered the item under the finger; asking which
     element is there now would miss it, because the footer of another
     group may have grown or shrunk in between and moved the item
     away.  A tap which has not wandered selects the item the press
     has chosen, and activates it */
  const bool tapped = std::abs(p.y - drag_start_y) < Layout::Scale(8);

  const int press = pressed_element >= 0 &&
    (std::size_t)pressed_element < elements.size()
    ? pressed_element
    : -1;

  const int press_column = pressed_column;

  pressed_element = -1;
  pressed_column = -1;

  bool activate = drag_mode == DragMode::CURSOR && tapped && press >= 0;

  if (activate && elements[press].has_children) {
    /* an item which opens and closes is a control like the switch: it
       does what it says on the first tap, wherever the cursor was.
       Where the cursor is what opens it, the first tap only carries
       the cursor there, because the item opens with it; a second tap
       on the same item closes it again */
    const bool arrived = press != cursor &&
      elements[press].expand_trigger == ExpandTrigger::CURSOR;

    SetCursor(press);

    if (!arrived)
      ActivateItem();

    activate = false;
  } else if (activate && elements[press].IsButtons()) {
    /* a button is a control, not a choice: it acts on the first tap,
       wherever the cursor was, like the switch of an item.  The
       cursor comes first, it arms the button the finger is on */
    SetCursor(press);

    if (press_column >= 0)
      button_column = (unsigned)press_column;
  } else if (activate) {
    const Element &element = elements[press];

    /* the switch is a control of its own: a tap on it does what it
       says, wherever the cursor was */
    const auto [toggle_left, toggle_right] = GetToggleHitArea(element);
    const bool on_switch = element.toggle &&
      p.x >= toggle_left && p.x < toggle_right;

    if (element.toggle && !on_switch &&
        element.toggle_hit_area == ToggleHitArea::SWITCH) {
      /* the rest of the item is only the label of that switch: a tap
         beside it selects the item, which shows what this setting
         does, and that is all it does */
      SetCursor(press);
      activate = false;
    } else if (!on_switch && press != cursor &&
               element.enter_action == EnterAction::ACTION_BAR) {
      /* the items of this group are a choice, and choosing one is
         what a tap on it means: the buttons of the dialog act on the
         item under the cursor, and the finger must be able to point
         at it without setting anything off.  A tap on the item which
         the cursor is on already is the second one, and that one
         activates it */
      SetCursor(press);
      activate = false;
    }
  }

  /* a tap on a link in an explanatory text opens it, as long as the
     finger has not moved and the tap has become a scroll gesture */
  const Link *const link =
    drag_mode == DragMode::SCROLL && tapped
    ? FindLinkAt(p)
    : nullptr;

  /* a tap beside the items - on the gap between two cards, on a
     caption or on an explanation - means "none of them", with a
     finger as with a mouse; the cursor of a list nobody is using is
     noise, and the buttons which act on it must not aim at an item
     the user has left behind */
  const bool clear_cursor = drag_mode == DragMode::SCROLL && tapped;

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

  if (link != nullptr)
    OpenLink(link->url.c_str());
  else if (activate) {
    /* the cursor follows the finger only now: a press which became a
       scroll gesture leaves it where it was */
    SetCursor(press);
    ActivateItem();
  } else if (clear_cursor)
    ClearCursor();

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
    /* an item which leaves Enter to the action bar does not take it
       here; the button which is marked there gets it instead, and
       the list keeps the cursor which that button acts on.  Should
       no button be marked, the key comes back to #OnKeyDown() and
       activates the item after all.  A button of the page takes
       Enter in any case: it is what the cursor is on */
    return cursor >= 0 &&
      (elements[cursor].IsButtons() ||
       elements[cursor].enter_action == EnterAction::ITEM);

  case KEY_SPACE:
    /* the item itself, whoever owns Enter.  A control stick has no
       such key: there, the action of an item belongs into the action
       bar as a button of its own */
    return cursor >= 0;

  case KEY_UP:
    return FindItem(GetSearchStart(false), false) >= 0;

  case KEY_DOWN:
    return FindItem(GetSearchStart(true), true) >= 0;

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
  case KEY_SPACE:
    ActivateItem();
    return true;

  case KEY_UP:
    if (FindItem(GetSearchStart(false), false) < 0)
      break;

    MoveCursor(false);
    return true;

  case KEY_DOWN:
    if (FindItem(GetSearchStart(true), true) < 0)
      break;

    MoveCursor(true);
    return true;

  case KEY_HOME:
    SetCursor(FindItem(0, true));

    /* the beginning of the page, not only the first item: what is
       above it - a hero card, the caption of the first group - is
       part of it, and Home is the way back to it */
    SetOrigin(0);
    return true;

  case KEY_END:
    SetCursor(FindItem(elements.size(), false));

    /* and the end of it, including the explanation below the last
       group */
    SetOrigin(content_height);
    return true;

  case KEY_PRIOR:
    MoveCursorPage(false);
    return true;

  case KEY_NEXT:
    MoveCursorPage(true);
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
GroupedListWidget::AddChildItem(const char *caption, Callback callback) noexcept
{
  control.AddChildItem(caption, std::move(callback), ItemOptions{});
}

void
GroupedListWidget::AddChildItem(const char *caption, Callback callback,
                                const ItemOptions &options) noexcept
{
  control.AddChildItem(caption, std::move(callback), options);
}

void
GroupedListWidget::AddChildItem(const char *caption,
                                const ItemOptions &options) noexcept
{
  control.AddChildItem(caption, {}, options);
}

void
GroupedListWidget::AddChildItems(std::initializer_list<ChildDefinition>
                                 children) noexcept
{
  for (const auto &child : children)
    control.AddChildItem(child.caption, child.callback, child.options);
}

void
GroupedListWidget::AddButton(const char *caption, Callback callback) noexcept
{
  AddButton(caption, std::move(callback), ButtonOptions{});
}

void
GroupedListWidget::AddButton(const char *caption, Callback callback,
                             const ButtonOptions &options) noexcept
{
  const ButtonDefinition button{caption, std::move(callback)};
  control.AddButtons({&button, 1}, options);
}

void
GroupedListWidget::AddButtonRow(std::initializer_list<ButtonDefinition>
                                buttons) noexcept
{
  AddButtonRow(buttons, ButtonOptions{});
}

void
GroupedListWidget::AddButtonRow(std::initializer_list<ButtonDefinition>
                                buttons,
                                const ButtonOptions &options) noexcept
{
  control.AddButtons({buttons.begin(), buttons.size()}, options);
}

void
GroupedListWidget::AddItem(const char *caption,
                           const ItemOptions &options) noexcept
{
  control.AddItem(caption, Callback{}, options);
}

void
GroupedListWidget::AddWidgetGroup(const char *caption,
                                  std::unique_ptr<Widget> widget,
                                  unsigned height_pt) noexcept
{
  AddWidgetGroup(caption, std::move(widget), GroupOptions{}, height_pt);
}

void
GroupedListWidget::AddWidgetGroup(const char *caption,
                                  std::unique_ptr<Widget> widget,
                                  const GroupOptions &options,
                                  unsigned height_pt) noexcept
{
  control.AddGroup(caption, options);
  control.AddWidget(std::move(widget), height_pt);
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

void
GroupedListWidget::SetActionBar(ButtonPanel &buttons) noexcept
{
  action_bar = &buttons;
}

bool
GroupedListWidget::MoveFocus(bool forward) noexcept
{
  auto *parent = control.GetParent();
  if (parent == nullptr)
    return false;

  if (action_bar != nullptr && control.HasFocus()) {
    /* on the way out of the list, both keys lead to the button which
       was used last: which of the two the user pressed says nothing
       about which button is meant, and one of them landing on the
       far end of the bar every time is a detour */
    if (auto *button = action_bar->GetSelectedButton()) {
      button->SetFocus();
      return true;
    }
  }

  /* the dialog knows the order of its controls, and it skips the
     buttons which are disabled; the list is one of them, so the focus
     comes back to it after the last button */
  return forward
    ? parent->FocusNextControl()
    : parent->FocusPreviousControl();
}

int
GroupedListWidget::GetCursorIndex() const noexcept
{
  return control.GetCursorIndex();
}

void
GroupedListWidget::SetCursorIndex(unsigned i) noexcept
{
  control.SetCursorByIndex(i);
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

void
GroupedListWidget::SetTopWidget(std::unique_ptr<Widget> _widget,
                                unsigned height_pt) noexcept
{
  assert(pending);

  top_widget = std::move(_widget);
  top_widget_height_pt = height_pt;
}

void
GroupedListWidget::SetBottomWidget(std::unique_ptr<Widget> _widget,
                                   unsigned height_pt) noexcept
{
  assert(pending);

  bottom_widget = std::move(_widget);
  bottom_widget_height_pt = height_pt;
}

unsigned
GroupedListWidget::GetWidgetHeight(const Widget *widget,
                                   unsigned height_pt) noexcept
{
  if (widget == nullptr)
    return 0;

  unsigned height = height_pt > 0
    ? Layout::VptScale(height_pt)
    : widget->GetMaximumSize().height;

  if (height == 0)
    /* a view which does not say how tall it would like to be */
    height = widget->GetMinimumSize().height;

  return height;
}

std::tuple<PixelRect, PixelRect, PixelRect>
GroupedListWidget::SplitRect(const PixelRect &rc) const noexcept
{
  unsigned top_height = GetWidgetHeight(top_widget.get(),
                                        top_widget_height_pt);
  unsigned bottom_height = GetWidgetHeight(bottom_widget.get(),
                                           bottom_widget_height_pt);

  /* the list keeps half of the room at least: it is what the page is
     about, and the views around it only explain it */
  const unsigned room = rc.GetHeight() / 2;

  if (top_height + bottom_height > room) {
    /* the widget above the list is a control which needs its row;
       what is left of the room goes to the one below */
    top_height = std::min(top_height, room);
    bottom_height = room - top_height;
  }

  PixelRect top_rc = rc, list_rc = rc, bottom_rc = rc;

  top_rc.bottom = list_rc.top = rc.top + (int)top_height;
  list_rc.bottom = bottom_rc.top = rc.bottom - (int)bottom_height;

  return {top_rc, list_rc, bottom_rc};
}

/**
 * A layout for Initialise() and Prepare(), which run before a view
 * may be asked how tall it wants to be.  Show() and Move() follow
 * with the real one.
 */
[[gnu::pure]]
static std::tuple<PixelRect, PixelRect, PixelRect>
DummySplitRect(const PixelRect &rc) noexcept
{
  PixelRect top_rc = rc, list_rc = rc, bottom_rc = rc;
  const int third = (int)rc.GetHeight() / 3;

  top_rc.bottom = list_rc.top = rc.top + third;
  list_rc.bottom = bottom_rc.top = rc.bottom - third;

  return {top_rc, list_rc, bottom_rc};
}

PixelSize
GroupedListWidget::GetMinimumSize() const noexcept
{
  PixelSize size{Layout::Scale(200u), 2u * control.GetItemHeight()};

  if (top_widget != nullptr)
    size.height += top_widget->GetMinimumSize().height;

  if (bottom_widget != nullptr)
    size.height += bottom_widget->GetMinimumSize().height;

  return size;
}

PixelSize
GroupedListWidget::GetMaximumSize() const noexcept
{
  return { 4096, 4096 };
}

void
GroupedListWidget::Initialise(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  const auto [top_rc, list_rc, bottom_rc] = DummySplitRect(rc);

  if (top_widget != nullptr)
    top_widget->Initialise(parent, top_rc);

  if (bottom_widget != nullptr)
    bottom_widget->Initialise(parent, bottom_rc);
}

void
GroupedListWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  assert(pending);

  const auto [top_rc, list_rc, bottom_rc] = DummySplitRect(rc);

  pending->Create(parent, top_widget == nullptr && bottom_widget == nullptr
                  ? rc : list_rc);
  SetWindow(std::move(pending));

  if (top_widget != nullptr)
    top_widget->Prepare(parent, top_rc);

  if (bottom_widget != nullptr)
    bottom_widget->Prepare(parent, bottom_rc);

  UpdateLayout();
}

void
GroupedListWidget::Unprepare() noexcept
{
  if (top_widget != nullptr)
    top_widget->Unprepare();

  if (bottom_widget != nullptr)
    bottom_widget->Unprepare();
}

bool
GroupedListWidget::Save(bool &changed) noexcept
{
  return (top_widget == nullptr || top_widget->Save(changed)) &&
    control.SaveWidgets(changed) &&
    (bottom_widget == nullptr || bottom_widget->Save(changed));
}

bool
GroupedListWidget::Leave() noexcept
{
  return (top_widget == nullptr || top_widget->Leave()) &&
    control.LeaveWidgets() &&
    (bottom_widget == nullptr || bottom_widget->Leave());
}

void
GroupedListWidget::Show(const PixelRect &rc) noexcept
{
  const auto [top_rc, list_rc, bottom_rc] = SplitRect(rc);

  if (top_widget != nullptr)
    top_widget->Show(top_rc);

  WindowWidget::Show(list_rc);

  if (bottom_widget != nullptr)
    bottom_widget->Show(bottom_rc);
}

void
GroupedListWidget::Hide() noexcept
{
  if (top_widget != nullptr)
    top_widget->Hide();

  if (bottom_widget != nullptr)
    bottom_widget->Hide();

  WindowWidget::Hide();
}

void
GroupedListWidget::Move(const PixelRect &rc) noexcept
{
  const auto [top_rc, list_rc, bottom_rc] = SplitRect(rc);

  if (top_widget != nullptr)
    top_widget->Move(top_rc);

  WindowWidget::Move(list_rc);

  if (bottom_widget != nullptr)
    bottom_widget->Move(bottom_rc);
}

bool
GroupedListWidget::KeyPress(unsigned key_code) noexcept
{
  /* the keys are routed here, before #WndForm maps Up and Down to
     focus movement and before a #ButtonPanel which has armed one of
     its buttons takes the Enter away from the item under the cursor.
     Only while the list or the action bar has the focus: Up and Down
     must reach a filter row otherwise, and Left and Right are what
     another control, a row of tabs for example, walks through what it
     holds with */
  if (IsDefined() && control.HasFocus()) {
    switch (key_code) {
    case KEY_UP:
    case KEY_DOWN:
      if (control.KeyFromWidget(key_code))
        return true;

      /* the list has no item left in this direction.  With an action
         bar, Up and Down stay in the list all the same: Left and
         Right are the way to the buttons, and a focus which slips
         away at the end of the list would be a second one */
      if (action_bar != nullptr)
        return true;

      break;

    case KEY_RETURN:
    case KEY_SPACE:
      if (control.KeyFromWidget(key_code))
        return true;

      break;

    case KEY_LEFT:
    case KEY_RIGHT:
      /* a row of buttons on the page is walked along first: it is
         what the cursor is on, and only its last button hands the
         key on */
      if (control.MoveButtonColumn(key_code == KEY_RIGHT))
        return true;

      /* out of the list and onto the buttons; Left reaches the last
         one of them, which is where a dialog puts Close */
      if (action_bar != nullptr && MoveFocus(key_code == KEY_RIGHT))
        return true;

      break;
    }
  } else if (action_bar != nullptr && IsDefined() &&
             action_bar->HasFocus()) {
    switch (key_code) {
    case KEY_UP:
    case KEY_DOWN:
      /* Up and Down belong to the list wherever the focus is: they
         bring it back instead of walking along the buttons, which
         Left and Right already do */
      control.SetFocus();
      return true;

    case KEY_LEFT:
    case KEY_RIGHT:
      if (MoveFocus(key_code == KEY_RIGHT))
        return true;

      break;
    }
  }

  /* a view of this page may know the key */
  return (top_widget != nullptr && top_widget->KeyPress(key_code)) ||
    control.KeyPressWidgets(key_code) ||
    (bottom_widget != nullptr && bottom_widget->KeyPress(key_code));
}
