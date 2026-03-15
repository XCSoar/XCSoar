// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/SelectorButtonsWidget.hpp"

#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "ui/window/ContainerWindow.hpp"

namespace {
static constexpr unsigned selector_button_count = 2;

[[gnu::pure]]
static unsigned
GetSelectorButtonWidth() noexcept
{
  return Layout::GetMaximumControlHeight() * 2;
}

class SelectorButtonsRowWindow final : public ContainerWindow {
  SelectorButtonsWidget::Handler &handler;
  WndProperty selector;
  Button minus_button;
  Button plus_button;
  bool controls_created = false;

  [[gnu::pure]]
  PixelRect GetSelectorRect(const PixelRect &rc) const noexcept
  {
    PixelRect selector_rc = rc;
    selector_rc.right -= selector_button_count * GetSelectorButtonWidth();
    return selector_rc;
  }

  [[gnu::pure]]
  PixelRect GetMinusButtonRect(const PixelRect &rc) const noexcept
  {
    PixelRect button_rc = rc;
    button_rc.left = rc.right - selector_button_count * GetSelectorButtonWidth();
    button_rc.right = button_rc.left + GetSelectorButtonWidth();
    return button_rc;
  }

  [[gnu::pure]]
  PixelRect GetPlusButtonRect(const PixelRect &rc) const noexcept
  {
    PixelRect button_rc = rc;
    button_rc.left = rc.right - GetSelectorButtonWidth();
    return button_rc;
  }

  void RefreshSelectorChoices() noexcept
  {
    if (!controls_created)
      return;

    auto &field = (DataFieldEnum &)*selector.GetDataField();
    handler.FillChoices(field);
    selector.RefreshDisplay();
  }

  void OnResize(PixelSize new_size) noexcept override
  {
    ContainerWindow::OnResize(new_size);

    if (!controls_created)
      return;

    const PixelRect rc = {
      0, 0,
      (int)new_size.width,
      (int)new_size.height,
    };
    selector.Move(GetSelectorRect(rc));
    minus_button.Move(GetMinusButtonRect(rc));
    plus_button.Move(GetPlusButtonRect(rc));
  }

public:
  explicit SelectorButtonsRowWindow(SelectorButtonsWidget::Handler &_handler) noexcept
    :handler(_handler),
     selector(UIGlobals::GetDialogLook()) {}

  void Create(ContainerWindow &parent, const PixelRect &rc) noexcept
  {
    WindowStyle style;
    style.Hide();
    style.ControlParent();

    ContainerWindow::Create(parent, rc, style);

    WindowStyle control_style;
    control_style.Hide();
    control_style.TabStop();

    selector.Create(*this, GetSelectorRect(GetClientRect()),
                    handler.GetLabel(), 0, control_style);
    selector.SetDataField(new DataFieldEnum());
    selector.GetDataField()->SetOnModified([this]{
      const auto value = ((const DataFieldEnum &)*selector.GetDataField()).GetValue();
      handler.OnModified(value);
    });

    minus_button.Create(*this, UIGlobals::GetDialogLook().button, "-",
                        GetMinusButtonRect(GetClientRect()), control_style,
                        [this]{ handler.Step(-1); });
    plus_button.Create(*this, UIGlobals::GetDialogLook().button, "+",
                       GetPlusButtonRect(GetClientRect()), control_style,
                       [this]{ handler.Step(1); });

    controls_created = true;
    selector.Show();
    minus_button.Show();
    plus_button.Show();
    RefreshSelectorChoices();
  }

  void Refresh() noexcept
  {
    if (IsDefined() && controls_created)
      RefreshSelectorChoices();
  }
};
} // namespace

SelectorButtonsWidget::SelectorButtonsWidget(std::unique_ptr<Handler> _handler) noexcept
  :handler(std::move(_handler)) {}

PixelSize
SelectorButtonsWidget::GetMinimumSize() const noexcept
{
  const unsigned label_width =
    UIGlobals::GetDialogLook().text_font.TextSize(handler->GetLabel()).width;
  return {
    label_width + handler->GetMinimumValueWidth() +
      selector_button_count * GetSelectorButtonWidth(),
    Layout::GetMaximumControlHeight(),
  };
}

PixelSize
SelectorButtonsWidget::GetMaximumSize() const noexcept
{
  return GetMinimumSize();
}

void
SelectorButtonsWidget::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  auto row = std::make_unique<SelectorButtonsRowWindow>(*handler);
  row->Create(parent, rc);
  SetWindow(std::move(row));
}

void
SelectorButtonsWidget::Unprepare() noexcept
{
  if (IsDefined())
    DeleteWindow();
}

void
SelectorButtonsWidget::RefreshChoices() noexcept
{
  if (IsDefined())
    ((SelectorButtonsRowWindow &)GetWindow()).Refresh();
}
