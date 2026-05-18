// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/SelectorButtonsWidget.hpp"

#include "Form/DataField/Enum.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Widget/ButtonWidget.hpp"
#include "Widget/RowFormWidget.hpp"

#include <cassert>

class SelectorFieldWidget final : public RowFormWidget {
  std::shared_ptr<SelectorButtonsWidget::Handler> handler;
  WndProperty *selector = nullptr;

public:
  explicit SelectorFieldWidget(std::shared_ptr<SelectorButtonsWidget::Handler> _handler) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     handler(std::move(_handler)) {
     assert(handler != nullptr);

  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override
  {
    RowFormWidget::Prepare(parent, rc);

    assert(selector == nullptr);
    selector = AddEnum(handler->GetLabel(), nullptr);
    selector->GetDataField()->SetOnModified([this]{
      const auto value = ((const DataFieldEnum &)*selector->GetDataField()).GetValue();
      handler->OnModified(value);
    });

    RefreshChoices();
  }

  void Unprepare() noexcept override
  {
    selector = nullptr;
    RowFormWidget::Unprepare();
  }

  void RefreshChoices() noexcept
  {
    if (selector == nullptr || handler == nullptr)
      return;

    auto *data_field = selector->GetDataField();
    if (data_field == nullptr)
      return;

    auto &field = (DataFieldEnum &)*data_field;
    handler->FillChoices(field);
    selector->RefreshDisplay();
  }
};

class FixedWidthButtonWidget final : public ButtonWidget {
  [[gnu::pure]]
  static unsigned GetWidth() noexcept
  {
    return Layout::GetMaximumControlHeight() * 2;
  }

public:
  FixedWidthButtonWidget(const ButtonLook &look, const char *caption,
                         std::function<void()> callback) noexcept
    :ButtonWidget(look, caption, std::move(callback)) {}

  PixelSize GetMinimumSize() const noexcept override
  {
    return {GetWidth(), Layout::GetMinimumControlHeight()};
  }

  PixelSize GetMaximumSize() const noexcept override
  {
    return {GetWidth(), Layout::GetMaximumControlHeight()};
  }

  [[gnu::pure]]
  static unsigned Width() noexcept
  {
    return GetWidth();
  }
};

SelectorButtonsWidget::SelectorButtonsWidget(std::shared_ptr<Handler> handler) noexcept
  :minus_button(std::make_unique<FixedWidthButtonWidget>(
      UIGlobals::GetDialogLook().button, "-",
      [handler]{ handler->Step(-1); })),
   plus_button(std::make_unique<FixedWidthButtonWidget>(
      UIGlobals::GetDialogLook().button, "+",
      [handler]{ handler->Step(1); })),
   selector_field(std::make_unique<SelectorFieldWidget>(handler)) {}

SelectorButtonsWidget::~SelectorButtonsWidget() noexcept = default;

PixelSize
SelectorButtonsWidget::GetMinimumSize() const noexcept
{
  const PixelSize selector = selector_field->GetMinimumSize();
  const PixelSize minus = minus_button->GetMinimumSize();
  const PixelSize plus = plus_button->GetMinimumSize();
  return {
    selector.width + 2u * FixedWidthButtonWidget::Width(),
    std::max({selector.height, minus.height, plus.height}),
  };
}

PixelSize
SelectorButtonsWidget::GetMaximumSize() const noexcept
{
  const PixelSize selector = selector_field->GetMaximumSize();
  const PixelSize minus = minus_button->GetMaximumSize();
  const PixelSize plus = plus_button->GetMaximumSize();
  return {
    selector.width + 2u * FixedWidthButtonWidget::Width(),
    std::max({selector.height, minus.height, plus.height}),
  };
}

SelectorButtonsWidget::Layout
SelectorButtonsWidget::CalculateLayout(const PixelRect &rc) const noexcept
{
  const PixelSize minus_size = minus_button->GetMinimumSize();
  const PixelSize plus_size = plus_button->GetMinimumSize();

  Layout layout{
    .minus = rc,
    .selector = rc,
    .plus = rc,
  };

  layout.minus.right = layout.minus.left + (int)minus_size.width;
  layout.plus.left = layout.plus.right - (int)plus_size.width;

  layout.selector.left = layout.minus.right;
  layout.selector.right = layout.plus.left;

  const int minus_height = (int)minus_size.height;
  const int minus_excess_height = layout.minus.GetHeight() - minus_height;
  layout.minus.top += minus_excess_height / 2;
  layout.minus.bottom = layout.minus.top + minus_height;

  const int plus_height = (int)plus_size.height;
  const int plus_excess_height = layout.plus.GetHeight() - plus_height;
  layout.plus.top += plus_excess_height / 2;
  layout.plus.bottom = layout.plus.top + plus_height;

  return layout;
}

void
SelectorButtonsWidget::Initialise(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  const auto layout = CalculateLayout(rc);
  minus_button->Initialise(parent, layout.minus);
  selector_field->Initialise(parent, layout.selector);
  plus_button->Initialise(parent, layout.plus);
}

void
SelectorButtonsWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  const auto layout = CalculateLayout(rc);
  minus_button->Prepare(parent, layout.minus);
  selector_field->Prepare(parent, layout.selector);
  plus_button->Prepare(parent, layout.plus);
}

void
SelectorButtonsWidget::Unprepare() noexcept
{
  minus_button->Unprepare();
  selector_field->Unprepare();
  plus_button->Unprepare();
}

bool
SelectorButtonsWidget::Save(bool &changed) noexcept
{
  return minus_button->Save(changed) &&
    selector_field->Save(changed) &&
    plus_button->Save(changed);
}

void
SelectorButtonsWidget::Show(const PixelRect &rc) noexcept
{
  const auto layout = CalculateLayout(rc);
  minus_button->Show(layout.minus);
  selector_field->Show(layout.selector);
  plus_button->Show(layout.plus);
}

bool
SelectorButtonsWidget::Leave() noexcept
{
  return minus_button->Leave() &&
    selector_field->Leave() &&
    plus_button->Leave();
}

void
SelectorButtonsWidget::Hide() noexcept
{
  minus_button->Hide();
  selector_field->Hide();
  plus_button->Hide();
}

void
SelectorButtonsWidget::Move(const PixelRect &rc) noexcept
{
  const auto layout = CalculateLayout(rc);
  minus_button->Move(layout.minus);
  selector_field->Move(layout.selector);
  plus_button->Move(layout.plus);
}

bool
SelectorButtonsWidget::SetFocus() noexcept
{
  return selector_field->SetFocus() ||
    minus_button->SetFocus() ||
    plus_button->SetFocus();
}

bool
SelectorButtonsWidget::HasFocus() const noexcept
{
  return minus_button->HasFocus() ||
    selector_field->HasFocus() ||
    plus_button->HasFocus();
}

bool
SelectorButtonsWidget::KeyPress(unsigned key_code) noexcept
{
  return minus_button->KeyPress(key_code) ||
    selector_field->KeyPress(key_code) ||
    plus_button->KeyPress(key_code);
}

void
SelectorButtonsWidget::RefreshChoices() noexcept
{
  selector_field->RefreshChoices();
}
