// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QuestionWidget.hpp"
#include "ButtonPanelWidget.hpp"
#include "TextWidget.hpp"
#include "Form/ButtonPanel.hpp"

QuestionWidget::QuestionWidget(const char *_message) noexcept
  :SolidWidget(std::make_unique<ButtonPanelWidget>(std::make_unique<TextWidget>(),
                                                   ButtonPanelWidget::Alignment::BOTTOM)),
   message(_message) {}

void
QuestionWidget::SetMessage(const char *_message) noexcept
{
  auto &bpw = (ButtonPanelWidget &)GetWidget();
  auto &tw = (TextWidget &)bpw.GetWidget();
  tw.SetText(_message);
}

inline void
QuestionWidget::ApplyMessageColors() noexcept
{
  auto &bpw = (ButtonPanelWidget &)GetWidget();
  auto &tw = (TextWidget &)bpw.GetWidget();
  tw.SetBackgroundColor(message_colors->background);
  tw.SetColor(message_colors->text);
}

void
QuestionWidget::SetMessageColors(Color background, Color text) noexcept
{
  message_colors.emplace(MessageColors{background, text});

  if (prepared)
    ApplyMessageColors();
}

void
QuestionWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  SolidWidget::Prepare(parent, rc);

  ButtonPanelWidget &bpw = (ButtonPanelWidget &)GetWidget();

  ((TextWidget &)bpw.GetWidget()).SetText(message);

  ButtonPanel &panel = bpw.GetButtonPanel();

  for (auto button : buttons)
    panel.Add(button.caption, button.callback);

  prepared = true;

  if (message_colors)
    ApplyMessageColors();
}


