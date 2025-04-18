// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/PanelWidget.hpp"
#include "Polar/Shape.hpp"

#include <memory>

class WndFrame;
class WndProperty;
class DataFieldListener;

class PolarShapeEditWidget : public PanelWidget {
public:
  struct PointEditor {
    std::unique_ptr<WndProperty> v, w;
  };

private:
  PolarShape shape;

  std::unique_ptr<WndFrame> v_label, w_label;

  PointEditor points[3];

  DataFieldListener *const listener;

public:
  PolarShapeEditWidget(const PolarShape &shape,
                       DataFieldListener *_listener) noexcept;
  ~PolarShapeEditWidget() noexcept;

  const PolarShape &GetPolarShape() const noexcept {
    return shape;
  }

  /**
   * Fill new values into the form.
   */
  void SetPolarShape(const PolarShape &shape) noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};
