// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/ContainerWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

#include <memory>

class Button;
class FlarmTrafficControl;

class TrafficWidget : public ContainerWidget,
                      private NullBlackboardListener {
  struct Windows;

  std::unique_ptr<Windows> windows;

public:
  TrafficWidget() noexcept;
  ~TrafficWidget() noexcept;

protected:
  void UpdateLayout() noexcept;
  void UpdateButtons() noexcept;

public:
  void Update() noexcept;
  void OpenDetails() noexcept;
  void ZoomIn() noexcept;
  void ZoomOut() noexcept;
  void PreviousTarget() noexcept;
  void NextTarget() noexcept;
  void SwitchData() noexcept;

  [[gnu::pure]]
  bool GetAutoZoom() const noexcept;
  void SetAutoZoom(bool value) noexcept;
  void ToggleAutoZoom() noexcept;
  void SaveZoom(unsigned value) noexcept;

  [[gnu::pure]]
  bool GetNorthUp() const noexcept;
  void SetNorthUp(bool value) noexcept;
  void ToggleNorthUp() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;

private:
  /* virtual methods from class BlackboardListener */
  virtual void OnGPSUpdate(const MoreData &basic) override;
};
