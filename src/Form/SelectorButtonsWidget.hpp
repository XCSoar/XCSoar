// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"

#include <memory>

class DataFieldEnum;
class ContainerWindow;

/**
 * A compact form widget that combines one #WndProperty selector with
 * horizontal "-" / "+" step buttons on its right side.
 */
class SelectorButtonsWidget final : public WindowWidget {
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

    /**
     * Return the minimum width needed for the selector value area.
     */
    virtual unsigned GetMinimumValueWidth() const noexcept = 0;

    /**
     * Rebuild the selector choices and current value.
     */
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
  std::unique_ptr<Handler> handler;

public:
  explicit SelectorButtonsWidget(std::unique_ptr<Handler> _handler) noexcept;

  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  /**
   * Rebuild the selector content from the current handler state.
   */
  void RefreshChoices() noexcept;
};
