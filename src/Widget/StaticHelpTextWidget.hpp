// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TextWidget.hpp"
#include "TwoWidgets.hpp"
#include "ui/event/Timer.hpp"

#include <memory>

/**
 * A #Widget wrapper that displays static help text at the bottom of
 * another widget.  Uses a #TwoWidgets layout with a #TextWidget for
 * the help text area.  The text is set via a deferred timer because
 * the #TwoWidgets instance is not fully initialised in Show().
 */
class StaticHelpTextWidget : public TwoWidgets {
  const char *help_text;

  UI::Timer set_help_timer{[this]() {
    ((TextWidget &)GetSecond()).SetText(help_text);
    UpdateLayout();
  }};

public:
  StaticHelpTextWidget(std::unique_ptr<Widget> main_widget,
                       const char *_help_text) noexcept
    :TwoWidgets(std::move(main_widget),
                std::make_unique<TextWidget>()),
     help_text(_help_text) {}

  void Show(const PixelRect &rc) noexcept override {
    TwoWidgets::Show(rc);
    set_help_timer.Schedule({});
  }

  void Hide() noexcept override {
    set_help_timer.Cancel();
    TwoWidgets::Hide();
  }
};
