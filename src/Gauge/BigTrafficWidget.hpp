/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_TRAFFIC_WIDGET_HPP
#define XCSOAR_TRAFFIC_WIDGET_HPP

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

#endif
