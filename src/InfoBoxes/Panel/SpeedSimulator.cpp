// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Simulator.hpp"
#include "SpeedSimulator.hpp"
#include "Look/DialogLook.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Form/Button.hpp"
#include "Math/Angle.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Units/Units.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "Widget/WindowWidget.hpp"
#include "util/StringFormat.hpp"
#include "util/TruncateString.hpp"

#include <array>
#include <algorithm>
#include <cmath>

#ifdef SIMULATOR_AVAILABLE

struct SpeedSimulatorAction {
  enum class Type {
    BEARING,
    SPEED,
  } type;

  double step;
};

static constexpr int DIRECTION_STEP_DEG = 5;
static constexpr double SPEED_STEP_KMH = 10;

static void
TriggerSpeedSimulatorAction(const SpeedSimulatorAction action) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!is_simulator() || backend_components == nullptr ||
      backend_components->device_blackboard == nullptr)
    return;

  auto &device_blackboard = *backend_components->device_blackboard;

  switch (action.type) {
  case SpeedSimulatorAction::Type::BEARING:
    device_blackboard.SetTrack(basic.track + Angle::Degrees(action.step));
    break;

  case SpeedSimulatorAction::Type::SPEED:
    device_blackboard.SetSpeed(std::max(basic.ground_speed + action.step, 0.0));
    break;
  }
}

static void
FormatSpeedSimulatorCaption(const SpeedSimulatorAction action,
                            char *buffer, size_t size) noexcept
{
  const char sign = action.step < 0 ? '-' : '+';
  const double value = std::abs(action.step);
  char formatted[128]{};

  switch (action.type) {
  case SpeedSimulatorAction::Type::BEARING: {
    const auto degrees = (unsigned)std::lround(value);
    StringFormat(formatted, sizeof(formatted), "%s %c%s",
                 C_("Abbreviation", "Brg"), sign, FormatBearing(degrees).c_str());
    break;
  }

  case SpeedSimulatorAction::Type::SPEED: {
    BasicStringBuffer<char, 32> step_buffer;
    FormatUserSpeed(value, step_buffer.data(), true, false);
    StringFormat(formatted, sizeof(formatted), "%s %c%s",
                 C_("Abbreviation", "Spd"), sign, step_buffer.c_str());
    break;
  }
  }

  CopyTruncateString(buffer, size, formatted);
}

static constexpr std::array<PixelRect, 4>
LayoutSpeedButtons(const PixelRect &total_rc) noexcept
{
  const unsigned total_width = total_rc.GetWidth();
  PixelRect rc = { 0, total_rc.top, total_rc.left, total_rc.bottom };

  std::array<PixelRect, 4> buttons{};
  for (unsigned i = 0; i < 4; ++i) {
    rc.left = rc.right;
    rc.right = total_rc.left + (i + 1) * total_width / 4;
    buttons[i] = rc;
  }

  return buttons;
}

class SpeedSimulatorWindow final : public ContainerWindow {
  const DialogLook &look;

  Button direction_minus_button, direction_plus_button;
  Button speed_minus_button, speed_plus_button;

public:
  explicit SpeedSimulatorWindow(const DialogLook &_look) noexcept
    :look(_look) {}

protected:
  void OnCreate() override;
  void OnResize(PixelSize new_size) noexcept override;

private:
  void MoveButtons(const PixelRect &rc) noexcept;
};

void
SpeedSimulatorWindow::OnCreate()
{
  ContainerWindow::OnCreate();

  WindowStyle style;
  style.TabStop();
  style.Hide();

  const SpeedSimulatorAction direction_minus{
    SpeedSimulatorAction::Type::BEARING, -DIRECTION_STEP_DEG
  };
  const SpeedSimulatorAction direction_plus{
    SpeedSimulatorAction::Type::BEARING, DIRECTION_STEP_DEG
  };
  const SpeedSimulatorAction speed_minus{
    SpeedSimulatorAction::Type::SPEED, -Units::ToSysSpeed(SPEED_STEP_KMH)
  };
  const SpeedSimulatorAction speed_plus{
    SpeedSimulatorAction::Type::SPEED, Units::ToSysSpeed(SPEED_STEP_KMH)
  };

  char caption[32];

  FormatSpeedSimulatorCaption(direction_minus, caption, sizeof(caption));
  direction_minus_button.Create(*this, look.button, caption, GetClientRect(), style,
                                [direction_minus](){
                                  TriggerSpeedSimulatorAction(direction_minus);
                                });

  FormatSpeedSimulatorCaption(direction_plus, caption, sizeof(caption));
  direction_plus_button.Create(*this, look.button, caption, GetClientRect(), style,
                               [direction_plus](){
                                 TriggerSpeedSimulatorAction(direction_plus);
                               });

  FormatSpeedSimulatorCaption(speed_minus, caption, sizeof(caption));
  speed_minus_button.Create(*this, look.button, caption, GetClientRect(), style,
                            [speed_minus](){
                              TriggerSpeedSimulatorAction(speed_minus);
                            });

  FormatSpeedSimulatorCaption(speed_plus, caption, sizeof(caption));
  speed_plus_button.Create(*this, look.button, caption, GetClientRect(), style,
                           [speed_plus](){
                             TriggerSpeedSimulatorAction(speed_plus);
                           });
}

void
SpeedSimulatorWindow::MoveButtons(const PixelRect &rc) noexcept
{
  const auto buttons = LayoutSpeedButtons(rc);

  direction_minus_button.MoveAndShow(buttons[0]);
  direction_plus_button.MoveAndShow(buttons[1]);
  speed_minus_button.MoveAndShow(buttons[2]);
  speed_plus_button.MoveAndShow(buttons[3]);
}

void
SpeedSimulatorWindow::OnResize(PixelSize new_size) noexcept
{
  ContainerWindow::OnResize(new_size);
  MoveButtons(GetClientRect());
}

class SpeedSimulatorWidget final : public WindowWidget {
  const DialogLook &look;

public:
  explicit SpeedSimulatorWidget(const DialogLook &_look) noexcept
    :look(_look) {}

  [[gnu::pure]] PixelSize GetMinimumSize() const noexcept override;
  [[gnu::pure]] PixelSize GetMaximumSize() const noexcept override;

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

PixelSize
SpeedSimulatorWidget::GetMinimumSize() const noexcept
{
  char caption[32];

  const SpeedSimulatorAction direction_minus{
    SpeedSimulatorAction::Type::BEARING, -DIRECTION_STEP_DEG
  };
  const SpeedSimulatorAction direction_plus{
    SpeedSimulatorAction::Type::BEARING, DIRECTION_STEP_DEG
  };
  const SpeedSimulatorAction speed_minus{
    SpeedSimulatorAction::Type::SPEED, -Units::ToSysSpeed(SPEED_STEP_KMH)
  };
  const SpeedSimulatorAction speed_plus{
    SpeedSimulatorAction::Type::SPEED, Units::ToSysSpeed(SPEED_STEP_KMH)
  };

  FormatSpeedSimulatorCaption(direction_minus, caption, sizeof(caption));
  const unsigned width1 = look.button.font->TextSize(caption).width;
  FormatSpeedSimulatorCaption(direction_plus, caption, sizeof(caption));
  const unsigned width2 = look.button.font->TextSize(caption).width;
  FormatSpeedSimulatorCaption(speed_minus, caption, sizeof(caption));
  const unsigned width3 = look.button.font->TextSize(caption).width;
  FormatSpeedSimulatorCaption(speed_plus, caption, sizeof(caption));
  const unsigned width4 = look.button.font->TextSize(caption).width;

  const unsigned min_button_width = std::max({ width1, width2, width3, width4 }) +
    2 * Layout::GetTextPadding();
  const unsigned min_button_height =
    std::max(Layout::GetMaximumControlHeight(),
             look.button.font->GetHeight() + 2 * Layout::GetTextPadding());

  return {
    4 * min_button_width,
    min_button_height,
  };
}

PixelSize
SpeedSimulatorWidget::GetMaximumSize() const noexcept
{
  return GetMinimumSize();
}

void
SpeedSimulatorWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  auto window = std::make_unique<SpeedSimulatorWindow>(look);
  WindowStyle style;
  style.ControlParent();
  style.Hide();
  window->Create(parent, rc, style);
  SetWindow(std::move(window));
}

#endif

std::unique_ptr<Widget>
LoadSpeedSimulatorPanel([[maybe_unused]] unsigned id)
{
#ifdef SIMULATOR_AVAILABLE
  return std::make_unique<SpeedSimulatorWidget>(UIGlobals::GetDialogLook());
#else
  return nullptr;
#endif
}
