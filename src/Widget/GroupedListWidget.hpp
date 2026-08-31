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

  /** The contents and the behaviour of a group. */
  struct GroupOptions {
    /**
     * An explanatory text below the card of the group.  It is
     * word-wrapped and gets as much room as it needs.
     */
    const char *footer = nullptr;
  };

private:
  /** owns the window until Prepare() hands it to #WindowWidget */
  std::unique_ptr<GroupedListControl> pending;

  GroupedListControl &control;

public:
  explicit GroupedListWidget(const DialogLook &look) noexcept;
  ~GroupedListWidget() noexcept override;

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
