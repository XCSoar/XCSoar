// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <functional>

/**
 * A compact map-bottom control strip: one or more rows of
 * &lt; prev | centre label | next &gt; steppers.
 *
 * The centre label is tappable when a #LabelClickCallback is set.
 * Layout matches touch-sized map weather cursor bars (XCTherm, etc.).
 */
class CursorBarWidget : public WindowWidget {
public:
  using StepCallback = std::function<void(unsigned row, int delta)>;
  using LabelClickCallback = std::function<void(unsigned row)>;

  static constexpr unsigned MAX_ROWS = 4;

  /**
   * @param row_count Number of stepper rows (1..#MAX_ROWS).
   */
  explicit CursorBarWidget(unsigned row_count=2) noexcept;

  void SetStepCallback(StepCallback cb) noexcept {
    step_callback = std::move(cb);
  }

  void SetLabelClickCallback(LabelClickCallback cb) noexcept {
    label_click_callback = std::move(cb);
  }

  /**
   * Update the centred label on @p row.
   */
  void SetRowText(unsigned row, const char *text,
                  bool available=true) noexcept;

  /**
   * Preferred height for @p row_count rows (includes inter-row separators).
   */
  [[gnu::const]]
  static unsigned DefaultHeight(unsigned row_count=2) noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Move(const PixelRect &rc) noexcept override;

protected:
  void InvokeStep(unsigned row, int delta) const noexcept;
  void InvokeLabelClick(unsigned row) const noexcept;
  void RelayoutBar() noexcept;

private:
  class BarWindow;

  const unsigned row_count;
  StepCallback step_callback;
  LabelClickCallback label_click_callback;
};
