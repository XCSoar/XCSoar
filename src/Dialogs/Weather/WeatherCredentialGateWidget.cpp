// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherCredentialGateWidget.hpp"

#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/VScrollWidget.hpp"

#include <algorithm>

class WeatherCredentialGateWidget final : public Widget {
  std::function<bool()> credentials_ok;
  std::function<std::unique_ptr<Widget>()> create_main;

  std::unique_ptr<Widget> setup;
  std::unique_ptr<Widget> main;

  enum class Phase {
    Setup,
    Main,
  } phase;

  bool initialised = false;
  bool prepared = false;
  bool visible = false;
  bool continue_added = false;

  ContainerWindow *parent = nullptr;
  PixelRect position;

  Widget &Active() noexcept {
    return phase == Phase::Setup ? *setup : *main;
  }

  const Widget &Active() const noexcept {
    return phase == Phase::Setup ? *setup : *main;
  }

  void EnsureContinueButton() noexcept;
  void ContinueClicked() noexcept;
  void SwitchToMain() noexcept;

  [[gnu::pure]]
  PixelSize MaxSize(PixelSize a, PixelSize b) const noexcept {
    return {std::max(a.width, b.width), std::max(a.height, b.height)};
  }

public:
  WeatherCredentialGateWidget(std::function<bool()> _credentials_ok,
                              std::unique_ptr<Widget> (*create_config_panel)(),
                              std::function<std::unique_ptr<Widget>()> _create_main)
    :credentials_ok(std::move(_credentials_ok)),
     create_main(std::move(_create_main)),
     phase(credentials_ok() ? Phase::Main : Phase::Setup)
  {
    const DialogLook &look = UIGlobals::GetDialogLook();
    auto config_panel = create_config_panel();
    auto scroll = std::make_unique<VScrollWidget>(std::move(config_panel),
                                                  look);
    setup = std::make_unique<ButtonPanelWidget>(
      std::move(scroll), ButtonPanelWidget::Alignment::BOTTOM);
    main = create_main();
  }

  PixelSize GetMinimumSize() const noexcept override {
    return MaxSize(setup->GetMinimumSize(), main->GetMinimumSize());
  }

  PixelSize GetMaximumSize() const noexcept override {
    return MaxSize(setup->GetMaximumSize(), main->GetMaximumSize());
  }

  void Initialise(ContainerWindow &_parent,
                  const PixelRect &rc) noexcept override {
    assert(!initialised);

    initialised = true;
    parent = &_parent;
    position = rc;

    setup->Initialise(_parent, rc);
    main->Initialise(_parent, rc);
  }

  void Prepare(ContainerWindow &_parent,
               const PixelRect &rc) noexcept override {
    assert(initialised);
    assert(!prepared);

    prepared = true;
    visible = false;
    parent = &_parent;
    position = rc;

    phase = credentials_ok() ? Phase::Main : Phase::Setup;

    if (phase == Phase::Setup) {
      setup->Prepare(_parent, rc);
      EnsureContinueButton();
    } else
      main->Prepare(_parent, rc);
  }

  void Unprepare() noexcept override {
    assert(prepared);

    if (phase == Phase::Setup)
      setup->Unprepare();
    else
      main->Unprepare();

    prepared = false;
    visible = false;
  }

  bool Save(bool &changed) noexcept override {
    assert(prepared);
    return Active().Save(changed);
  }

  bool Click() noexcept override {
    assert(prepared);
    return Active().Click();
  }

  void ReClick() noexcept override {
    assert(prepared);
    Active().ReClick();
  }

  void Show(const PixelRect &rc) noexcept override {
    assert(prepared);
    assert(!visible);

    visible = true;
    position = rc;
    Active().Show(rc);
  }

  bool Leave() noexcept override {
    assert(prepared);
    assert(visible);
    return Active().Leave();
  }

  void Hide() noexcept override {
    assert(prepared);
    assert(visible);

    Active().Hide();
    visible = false;
  }

  void Move(const PixelRect &rc) noexcept override {
    assert(prepared);
    assert(visible);

    position = rc;
    Active().Move(rc);
  }

  bool SetFocus() noexcept override {
    assert(prepared);
    return Active().SetFocus();
  }

  bool HasFocus() const noexcept override {
    assert(prepared);
    return Active().HasFocus();
  }

  bool KeyPress(unsigned key_code) noexcept override {
    assert(prepared);
    return Active().KeyPress(key_code);
  }
};

void
WeatherCredentialGateWidget::EnsureContinueButton() noexcept
{
  if (continue_added)
    return;

  auto &buttons_widget = static_cast<ButtonPanelWidget &>(*setup);
  ButtonPanel &buttons = buttons_widget.GetButtonPanel();
  buttons.Add(_("Continue"), [this]() { ContinueClicked(); });
  continue_added = true;
}

void
WeatherCredentialGateWidget::ContinueClicked() noexcept
{
  bool changed = false;
  if (!setup->Save(changed))
    return;

  if (!credentials_ok()) {
    ShowMessageBox(_("Enter your account details."),
                   _("Error"), MB_OK);
    return;
  }

  if (changed)
    Profile::Save();

  SwitchToMain();
}

void
WeatherCredentialGateWidget::SwitchToMain() noexcept
{
  assert(prepared);
  assert(parent != nullptr);

  if (phase == Phase::Main)
    return;

  if (visible)
    setup->Hide();
  setup->Unprepare();

  phase = Phase::Main;
  main->Prepare(*parent, position);

  if (visible)
    main->Show(position);
}

std::unique_ptr<Widget>
CreateWeatherCredentialGateWidget(
  std::function<bool()> credentials_ok,
  std::unique_ptr<Widget> (*create_config_panel)(),
  std::function<std::unique_ptr<Widget>()> create_main) noexcept
{
  return std::make_unique<WeatherCredentialGateWidget>(
    std::move(credentials_ok), create_config_panel, std::move(create_main));
}
