// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "util/tstring.hpp"

class PluggableOperationEnvironment;

/**
 * A widget which displays a progress bar.
 */
class ProgressWidget final : public WindowWidget, MessageOperationEnvironment {
  class ProgressBar;

  PluggableOperationEnvironment &env;

  tstring text;

public:
  explicit ProgressWidget(PluggableOperationEnvironment &_env,
                          const char *_text) noexcept
    :env(_env), text(_text) {}

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

private:
  /* virtual methods from class OperationEnvironment */
  void SetText(const char *text) noexcept override;
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};
