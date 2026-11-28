// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QuestionWidget.hpp"
#include "ButtonPanelWidget.hpp"
#include "TextWidget.hpp"
#include "Form/ButtonPanel.hpp"
#include "ui/event/KeyCode.hpp"

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

inline ButtonPanel &
QuestionWidget::GetButtonPanel() noexcept
{
  return ((ButtonPanelWidget &)GetWidget()).GetButtonPanel();
}

bool
QuestionWidget::FocusButton(unsigned index) noexcept
{
  if (!prepared || buttons.empty())
    return false;

  if (index >= buttons.size())
    index = 0;

  ButtonPanel &panel = GetButtonPanel();
  panel.DisableCursorSelection();
  panel.EnableCursorSelection(index);
  return true;
}

int
QuestionWidget::GetFocusedButton() noexcept
{
  return prepared ? GetButtonPanel().GetSelectedIndex() : -1;
}

bool
QuestionWidget::KeyPress(unsigned key_code) noexcept
{
  if (GetFocusedButton() < 0)
    return false;

  ButtonPanel &panel = GetButtonPanel();

  switch (key_code) {
  case KEY_LEFT:
  case KEY_RIGHT:
    return panel.KeyPress(key_code);

  case KEY_RETURN: {
    ::Button *button = panel.GetSelectedButton();
    if (button == nullptr)
      return true;

    /* hand the keys back first: a second Enter must not answer
       another question, and the click may delete this object */
    panel.DisableCursorSelection();
    button->Click();
    return true;
  }

  case KEY_ESCAPE:
    panel.DisableCursorSelection();
    return true;

  default:
    return false;
  }
}


