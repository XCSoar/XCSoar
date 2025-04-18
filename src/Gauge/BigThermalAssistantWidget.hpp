// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/ContainerWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

#include <memory>

struct AttitudeState;
class LiveBlackboard;
struct ThermalAssistantLook;
class Button;
class BigThermalAssistantWindow;

class BigThermalAssistantWidget
  : public ContainerWidget,
    private NullBlackboardListener {
  LiveBlackboard &blackboard;
  const ThermalAssistantLook &look;

  std::unique_ptr<BigThermalAssistantWindow> view;

  std::unique_ptr<Button> close_button;

public:
  BigThermalAssistantWidget(LiveBlackboard &_blackboard,
                            const ThermalAssistantLook &_look) noexcept;

  ~BigThermalAssistantWidget() noexcept;

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) noexcept override;
  virtual void Show(const PixelRect &rc) noexcept override;
  virtual void Hide() noexcept override;
  virtual void Move(const PixelRect &rc) noexcept override;
  virtual bool SetFocus() noexcept override;

private:
  void UpdateLayout() noexcept;
  void Update(const AttitudeState &attitude,
              const DerivedInfo &calculated) noexcept;

  /* virtual methods from class BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) override;
};
