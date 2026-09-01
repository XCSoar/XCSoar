// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

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

  /** The colors of a badge. */
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
     * word-wrapped and gets as much room as it needs.
     */
    const char *footer = nullptr;
  };

  /** The contents and the decorations of an item. */
  struct ItemOptions {
    /** a text at the right edge, e.g. the current value of a setting */
    const char *value = nullptr;

    /** a short label in a rounded box, e.g. "active" */
    const char *badge = nullptr;

    /** the colors of #badge */
    BadgeStyle badge_style = BadgeStyle::PRIMARY;

    /** an arrow, marking an item which opens another page */
    bool chevron = false;
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

  /** Begin a new group with the given options. */
  void AddGroup(const char *caption, const GroupOptions &options) noexcept;

  /**
   * Append a selectable item to the group which was opened by the
   * last AddGroup() call.
   */
  void AddItem(const char *caption, Callback callback) noexcept;

  void AddItem(const char *caption, Callback callback,
               const ItemOptions &options) noexcept;

  /** Append an item which has no callback. */
  void AddItem(const char *caption, const ItemOptions &options) noexcept;

  void Clear() noexcept;

  /**
   * @return the number of items
   */
  [[gnu::pure]]
  unsigned GetItemCount() const noexcept;

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
