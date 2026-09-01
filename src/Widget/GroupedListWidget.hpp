// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "ResourceId.hpp"

#include <cstdint>
#include <functional>
#include <memory>

struct DialogLook;
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
   * counting only items
   */
  using CursorCallback = std::function<void(unsigned index)>;

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

    /** a short label in a rounded box, e.g. "active" */
    const char *badge = nullptr;

    /** the colors of #badge */
    BadgeStyle badge_style = BadgeStyle::PRIMARY;

    /** an arrow, marking an item which opens another page */
    bool chevron = false;

    /** is this item checked?  (only with a #SelectionMode) */
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
     * The badge of an item which is #disabled; nullptr for the
     * default label.  Use it to say why the item is not available.
     */
    const char *disabled_badge_label = nullptr;
  };

private:
  /** owns the window until Prepare() hands it to #WindowWidget */
  std::unique_ptr<GroupedListControl> pending;

  GroupedListControl &control;

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
   * Remove all elements.  The item the cursor is on and the scroll
   * position survive the next UpdateLayout(), so that a list which
   * refreshes itself does not jump back to the top.
   */
  void Clear() noexcept;

  /**
   * @return the number of items
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
   * @return the index of the item the cursor is on, counting only
   * items; -1 if there is no cursor
   */
  [[gnu::pure]]
  int GetCursorIndex() const noexcept;

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

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};
