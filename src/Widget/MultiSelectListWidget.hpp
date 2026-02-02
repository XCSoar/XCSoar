// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ListWidget.hpp"

#include <vector>
#include <algorithm>
#include <tchar.h>

class Canvas;
struct PixelRect;

/**
 * A thin extension of ListWidget that supports multi-selection.
 * Clicking/tapping a row (or pressing RETURN) toggles its selection state
 * with immediate feedback (single-click activation).
 */
class MultiSelectListWidget : public ListWidget {
  std::vector<uint8_t> selected_;

public:
  MultiSelectListWidget() noexcept = default;

  /** Set list length and reset selection state */
  void SetLengthWithSelection(unsigned n) noexcept {
    selected_.assign(n, false);
    GetList().SetLength(n);
  }

  void ClearSelection() noexcept {
    if (selected_.empty())
      return;

    std::fill(selected_.begin(), selected_.end(), false);
    GetList().Invalidate();
    OnSelectionChanged();
  }

  /**
   * @return true if index is in-range and selected.
   */
  [[gnu::pure]]
  bool IsSelected(unsigned idx) const noexcept {
    return idx < selected_.size() && selected_[idx];
  }

  /**
   * Toggle selection and invalidate the item for repaint.
   */
  void ToggleSelection(unsigned idx) noexcept {
    if (idx >= selected_.size())
      return;
    selected_[idx] = !selected_[idx];
    GetList().Invalidate();
    OnSelectionChanged();
  }

  /**
   * Collect selected indices into a vector.
   */
  [[gnu::pure]]
  std::vector<unsigned> GetSelectedIndices() const {
    std::vector<unsigned> out;
    out.reserve(selected_.size());
    for (unsigned i = 0; i < selected_.size(); ++i)
      if (selected_[i])
        out.push_back(i);
    return out;
  }

  [[gnu::pure]]
  unsigned GetSelectedCount() const noexcept {
    return std::count_if(selected_.begin(), selected_.end(),
                         [](uint8_t v) { return v != 0; });
  }

  /** Select or deselect all items. */
  void SetAllSelected(bool value) noexcept {
    if (selected_.empty())
      return;
    std::fill(selected_.begin(), selected_.end(), value);
    GetList().Invalidate();
    OnSelectionChanged();
  }

  /** Set selection for a single item (no toggle). */
  void SetSelected(unsigned idx, bool value) noexcept {
    if (idx >= selected_.size())
      return;
    if (selected_[idx] == value)
      return;
    selected_[idx] = value;
    GetList().Invalidate();
    OnSelectionChanged();
  }

  /**
   * Enable single-click activation mode for multi-select lists.
   * Called from Prepare() after the window is created.
   */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    ListWidget::Prepare(parent, rc);
    GetList().SetActivateOnFirstClick(true);
  }

protected:
  /* ListCursorHandler hooks: activate toggles selection */
  bool CanActivateItem(unsigned index) const noexcept override {
    return index < selected_.size();
  }

  void OnActivateItem(unsigned index) noexcept override {
    ToggleSelection(index);
  }

  /**
   * Draw a checkbox at the left of the row and clipped text to the right.
   */
  void DrawCheckboxText(Canvas &canvas, const PixelRect &rc,
                        const TCHAR *text, bool selected) noexcept;

  /** Optional hook for derived classes to react to selection changes. */
  virtual void OnSelectionChanged() noexcept {}
};
