// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "ResourceId.hpp"
#include "ui/dim/Rect.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>

struct DialogLook;
class ButtonPanel;
class GroupedListControl;

/**
 * A #Widget which divides one page into several captioned groups.
 *
 * The items of a group are drawn as a card with rounded corners; the
 * caption of a group sits above its card, an optional footer below
 * it.  Unlike #ListWidget, every element has its own height: captions
 * and footers are only as tall as their text, while the items keep
 * the height of a comfortable touch target.
 *
 * Use this where a page has more entries than fit into one flat list,
 * but splitting them over several pages (as #TabWidget or the
 * configuration menu do) would hide the structure from the user.
 */
class GroupedListWidget final : public WindowWidget {
public:
  using Callback = std::function<void()>;

  /**
   * @param index the index of the item the cursor has moved to,
   * counting only items; -1 if the cursor has been taken off the
   * list, which happens when the user taps beside the items
   */
  using CursorCallback = std::function<void(int index)>;

  /** How many items the user may check. */
  enum class SelectionMode : uint_least8_t {
    /** no check marks; an item only invokes its callback */
    NONE,

    /**
     * At most one item of each group carries a check mark, like a
     * group of radio buttons.  Checking an item unchecks the item
     * which was checked before, but only within the same group.
     */
    SINGLE,

    /** any number of items carries a check mark */
    MULTIPLE,
  };

  /** The edge of the item where the check mark is drawn. */
  enum class CheckPosition : uint_least8_t { LEFT, RIGHT };

  /**
   * The part of an item where a tap flips its
   * #ItemOptions::toggle.  It says nothing about the keyboard:
   * Enter always flips the switch of the item under the cursor.
   */
  enum class ToggleHitArea : uint_least8_t {
    /**
     * The switch itself.  A tap on the rest of the item only moves
     * the cursor there, which shows the #ItemOptions::help of the
     * item; this lets the user read what a setting does before
     * changing it.
     */
    SWITCH,

    /** anywhere on the item, which is the larger target */
    ROW,
  };

  /** The font which draws a short text of an item. */
  enum class TextFont : uint_least8_t {
    /** the font of the list, like the caption */
    DEFAULT,

    /**
     * Fixed width, for a text whose characters shall line up, e.g. a
     * path, a serial number, a checksum or a code of four letters.
     */
    MONO,
  };

  /**
   * The colors of a badge.  An item which is #ItemOptions::disabled is
   * grey no matter which style it carries, and grey is reserved for
   * it: it must stay the one thing which means "not available".
   */
  enum class BadgeStyle : uint_least8_t {
    /** the accent color of the dialog; the state which shall be seen */
    PRIMARY,

    /** something needs attention, but works, e.g. "outdated" */
    WARNING,

    /** something is broken, e.g. "no fix" */
    DANGER,

    /** something has succeeded, e.g. "connected" */
    SUCCESS,
  };

  /** The contents and the behaviour of a group. */
  struct GroupOptions {
    /**
     * An explanatory text below the card of the group.  It is
     * word-wrapped and gets as much room as it needs; an item with
     * an #ItemOptions::help replaces it while the cursor is on it.
     */
    const char *footer = nullptr;

    /** how many items of this group may be checked at the same time */
    SelectionMode selection_mode = SelectionMode::NONE;

    /** the edge which holds the check mark */
    CheckPosition check_position = CheckPosition::RIGHT;
  };

  /** The contents and the decorations of an item. */
  struct ItemOptions {
    /**
     * An icon at the left edge, e.g. IDB_TEAMMATE_POS.  It is scaled
     * into a square which is as tall as one and a half lines of text,
     * no matter how tall the item is.
     */
    ResourceId icon = ResourceId::Null();

    /**
     * A character which is drawn where #icon would be, e.g. an emoji.
     * It is left out on a display whose font does not have it, and
     * the column is then not reserved either.
     */
    const char *icon_text = nullptr;

    /**
     * A second line below the caption, in a smaller font; the same
     * shape the device list and the WiFi list use today.  It makes
     * the item taller.
     */
    const char *subtitle = nullptr;

    /** a text at the right edge, e.g. the current value of a setting */
    const char *value = nullptr;

    /**
     * Draw #value below the caption instead of beside it, over the
     * whole width of the item.  A value which leaves too little room
     * for the caption moves there by itself.
     */
    bool value_below = false;

    /** the font of #value */
    TextFont value_font = TextFont::DEFAULT;

    /** a short label in a rounded box, e.g. "active" */
    const char *badge = nullptr;

    /** the colors of #badge */
    BadgeStyle badge_style = BadgeStyle::PRIMARY;

    /**
     * The font of #badge.  A code which is read letter by letter, a
     * frequency or an identifier is easier to compare from one item
     * to the next when its characters line up.
     */
    TextFont badge_font = TextFont::DEFAULT;

    /** an arrow, marking an item which opens another page */
    bool chevron = false;

    /**
     * A switch at the right edge which shows whether the setting is
     * on, for an item which is nothing but a boolean.  Tapping the
     * item flips the switch and then calls the callback, which reads
     * the new state with GroupedListWidget::IsItemChecked().  The
     * switch takes the place of the value, and of the check mark of a
     * group which has a #SelectionMode: an item with a switch keeps
     * the room of that column, but neither shows a check mark nor
     * takes part in the selection of its group.
     */
    bool toggle = false;

    /** the part of the item where a tap flips #toggle */
    ToggleHitArea toggle_hit_area = ToggleHitArea::SWITCH;

    /**
     * Is this item checked?  It is the state of the check mark (with
     * a #SelectionMode) or of the #toggle.
     */
    bool checked = false;

    /**
     * An explanation of this item, shown below the card of its group
     * while the cursor is on this item; it replaces
     * #GroupOptions::footer.
     */
    const char *help = nullptr;

    /**
     * Is this item currently not available?  It is drawn greyed out,
     * the cursor skips it, and it carries a badge which says so,
     * replacing #badge.
     */
    bool disabled = false;

    /**
     * May the cursor rest on this item while it is #disabled?
     * Activating it still does nothing, but its help text can be
     * read, and a button of the dialog can act on it - a button
     * which makes the item available again could not be aimed at it
     * otherwise.
     */
    bool selectable_when_disabled = false;

    /**
     * The badge of an item which is #disabled; nullptr for the
     * default label.  Use it to say why the item is not available.
     */
    const char *disabled_badge_label = nullptr;

    /**
     * Leave this item out.  It is not drawn, it occupies no room and
     * the cursor skips it, but it keeps its index: the indices of the
     * items behind it do not move while it is hidden.
     */
    bool hidden = false;
  };

private:
  /** owns the window until Prepare() hands it to #WindowWidget */
  std::unique_ptr<GroupedListControl> pending;

  GroupedListControl &control;

  /** the view above the list, or nullptr */
  std::unique_ptr<Widget> top_widget;

  /** the height of #top_widget; 0 asks the view itself */
  unsigned top_widget_height_pt = 0;

  /** the view below the list, or nullptr */
  std::unique_ptr<Widget> bottom_widget;

  /** the height of #bottom_widget; 0 asks the view itself */
  unsigned bottom_widget_height_pt = 0;

  /** the buttons which Left and Right reach, or nullptr */
  ButtonPanel *action_bar = nullptr;

public:
  explicit GroupedListWidget(const DialogLook &look) noexcept;
  ~GroupedListWidget() noexcept override;

  /**
   * Add a hero card which introduces the page or a part of it, with a
   * title in a larger font and an optional description below it.
   */
  void AddHero(const char *title, const char *description=nullptr) noexcept;

  /**
   * Begin a new group.  All items added afterwards belong to it.
   *
   * @param caption the group caption; nullptr for a group which is
   * only separated from the previous one, without a caption
   */
  void AddGroup(const char *caption=nullptr) noexcept;

  /**
   * Begin a new group which lets the user check its items.  Inside
   * one group, items with and without a check mark may be mixed;
   * those without one still reserve the room for it, which keeps
   * their captions aligned.
   */
  void AddGroup(const char *caption, const GroupOptions &options) noexcept;

  /**
   * Append a selectable item to the group which was opened by the
   * last AddGroup() call.
   */
  void AddItem(const char *caption, Callback callback) noexcept;

  void AddItem(const char *caption, Callback callback,
               const ItemOptions &options) noexcept;

  /**
   * Append an item which has no callback; useful for a list which is
   * only there to be checked.
   */
  void AddItem(const char *caption, const ItemOptions &options) noexcept;

  /**
   * Add a group which shows a view instead of items, e.g. a button or
   * a preview of what the page changes.  Its caption and its footer
   * work as they do for a group of items, and the view keeps the
   * margins of a card and scrolls with the list.
   *
   * The view may edit something: Save(), Leave() and KeyPress() reach
   * it like they reach the widgets above and below the list.  Click()
   * and ReClick() do not, they belong to the activation area of a tab
   * container, and the focus of this widget stays on the list.
   *
   * @param caption the group caption; nullptr for a group which shows
   * nothing but the view
   * @param height_pt the height of the view; 0 asks the view itself,
   * which means its maximum size, or its minimum size if it has no
   * maximum.  Other than the widget below the list, it may be taller
   * than the window.
   */
  void AddWidgetGroup(const char *caption, std::unique_ptr<Widget> widget,
                      unsigned height_pt=0) noexcept;

  void AddWidgetGroup(const char *caption, std::unique_ptr<Widget> widget,
                      const GroupOptions &options,
                      unsigned height_pt=0) noexcept;

  /**
   * Remove all elements.  The item the cursor is on and the scroll
   * position survive the next UpdateLayout(), so that a list which
   * refreshes itself does not jump back to the top.
   */
  void Clear() noexcept;

  /**
   * @return the number of items; a hidden item counts too, because it
   * keeps its index
   */
  [[gnu::pure]]
  unsigned GetItemCount() const noexcept;

  /**
   * Install a function which is called whenever the cursor moves to
   * another item; a dialog uses it to enable and disable the buttons
   * which act on the current item.  It is not called while the list
   * is being built.
   */
  void SetCursorCallback(CursorCallback callback) noexcept;

  /**
   * Let Left and Right move the keyboard focus between the list and
   * the buttons of the dialog, in the order in which the dialog has
   * created them.  Without this, both keys stay free.
   *
   * A control stick has four directions and one button, and Up and
   * Down belong to the list: this gives the user of such a device a
   * way to reach a button which acts on the item under the cursor
   * without scrolling to the end of the list first.  The focus is
   * what decides where Enter goes, so there is no second, invisible
   * state to keep in mind.  Combine it with
   * WidgetDialog::EnableCursorSelection(), which marks the button
   * which the focus is on: both keys then lead out of the list to
   * the button which was used last, and only walk along the bar once
   * the focus is there.
   */
  void SetActionBar(ButtonPanel &buttons) noexcept;

  /**
   * Move the keyboard focus to the next or the previous control of
   * the dialog, which is a button of the action bar or the list
   * itself.
   */
  bool MoveFocus(bool forward) noexcept;

  /**
   * @return the index of the item the cursor is on, counting only
   * items; -1 if there is no cursor
   */
  [[gnu::pure]]
  int GetCursorIndex() const noexcept;

  /**
   * Move the cursor to one item, counting only items.  A dialog which
   * rearranges the list calls it to let the cursor follow the item
   * the user has moved.  It may be called while the list is being
   * filled: the cursor moves as soon as the list has been laid out.
   */
  void SetCursorIndex(unsigned i) noexcept;

  /**
   * Check or uncheck one item.  In #SelectionMode::SINGLE, checking
   * an item unchecks the other items of its group.
   *
   * @param i the index of the item, counting only items
   */
  void SetItemChecked(unsigned i, bool checked=true) noexcept;

  /**
   * @param i the index of the item, counting only items
   */
  [[gnu::pure]]
  bool IsItemChecked(unsigned i) const noexcept;

  /**
   * Lay out the elements again.  Called by Prepare(); call it again
   * after the contents of a prepared widget have been changed.
   */
  void UpdateLayout() noexcept;

  /**
   * Show another view above the list, e.g. the row which names the
   * directory a file list is in.  It sits between the title of the
   * dialog and the list; it does not scroll with the list, and it
   * does not cover it either, it takes its room from it.  Call this
   * before Prepare().
   *
   * @param height_pt the height of the view; 0 asks the view itself,
   * which means its maximum size, or its minimum size if it has no
   * maximum.  The list always keeps half of the room.
   */
  void SetTopWidget(std::unique_ptr<Widget> widget,
                    unsigned height_pt=0) noexcept;

  /**
   * Show another view below the list, e.g. a preview of what the
   * items above it change.  It sits between the list and the buttons
   * of the dialog; it does not scroll with the list, and it does not
   * cover it either, it takes its room from it.  Call this before
   * Prepare().
   *
   * @param height_pt the height of the view; 0 asks the view itself,
   * which means its maximum size, or its minimum size if it has no
   * maximum.  The list always keeps half of the room.
   */
  void SetBottomWidget(std::unique_ptr<Widget> widget,
                       unsigned height_pt=0) noexcept;

private:
  /**
   * The room which one of the two views takes; 0 if there is none.
   */
  [[gnu::pure]]
  static unsigned GetWidgetHeight(const Widget *widget,
                                  unsigned height_pt) noexcept;

  /** @return the rectangles of #top_widget, of the list and of
      #bottom_widget */
  [[gnu::pure]]
  std::tuple<PixelRect, PixelRect, PixelRect>
  SplitRect(const PixelRect &rc) const noexcept;

public:
  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool Save(bool &changed) noexcept override;
  bool Leave() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};
