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

#ifndef XCSOAR_DEVICE_EDIT_WIDGET_HPP
#define XCSOAR_DEVICE_EDIT_WIDGET_HPP

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
  PolarShapeEditWidget(const PolarShape &shape, DataFieldListener *_listener);
  ~PolarShapeEditWidget() noexcept;

  const PolarShape &GetPolarShape() const {
    return shape;
  }

  /**
   * Fill new values into the form.
   */
  void SetPolarShape(const PolarShape &shape);

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const override;
  PixelSize GetMaximumSize() const override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  bool Save(bool &changed) override;
};

#endif
