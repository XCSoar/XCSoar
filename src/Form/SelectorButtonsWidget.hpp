// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"
#include "Widget/Widget.hpp"

#include <memory>

class DataFieldEnum;
class ContainerWindow;
class SelectorFieldWidget;
class FixedWidthButtonWidget;

/**
 * A compact form widget that combines one #WndProperty selector with
 * horizontal "-" / "+" step buttons on its right side.
 */
class SelectorButtonsWidget final : public NullWidget {
public:
  /**
   * Supplies the selector caption, enum choices and step behavior for
   * #SelectorButtonsWidget.
   */
  class Handler {
  public:
    virtual ~Handler() noexcept = default;

    /**
     * Return the selector caption shown on the left side.
     */
    virtual const char *GetLabel() const noexcept = 0;


    virtual void FillChoices(DataFieldEnum &field) noexcept = 0;

    /**
     * Called when the selector value has changed interactively.
     */
    virtual void OnModified(unsigned value) noexcept = 0;

    /**
     * Step the current value by one unit in the requested direction.
     */
    virtual void Step(int delta) noexcept = 0;
  };

private:
  struct Layout {
    PixelRect minus, selector, plus;
  };

  std::unique_ptr<FixedWidthButtonWidget> minus_button, plus_button;
  std::unique_ptr<SelectorFieldWidget> selector_field;

  Layout CalculateLayout(const PixelRect &rc) const noexcept;

public:
  SelectorButtonsWidget(std::shared_ptr<Handler> handler) noexcept;
  ~SelectorButtonsWidget() noexcept override;

  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  bool Save(bool &changed) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Leave() noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

  /**
   * Rebuild the selector content from the current handler state.
   */
  void RefreshChoices() noexcept;
};
