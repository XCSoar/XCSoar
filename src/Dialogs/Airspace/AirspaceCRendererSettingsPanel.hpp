// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"

class AirspaceClassRendererSettingsPanel:
  public RowFormWidget
{
  enum ControlIndex {
    BorderColor,
    FillColor,
    FillBrush,
    BorderWidth,
    FillMode,
  };

  bool border_color_changed;
  bool fill_color_changed;
  bool fill_brush_changed;
  AirspaceClass type;
  AirspaceClassRendererSettings settings;

public:
  explicit AirspaceClassRendererSettingsPanel(AirspaceClass type) noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void FillAirspaceClasses() noexcept;
};
