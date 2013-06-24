/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_SOLID_WIDGET_HPP
#define XCSOAR_SOLID_WIDGET_HPP

#include "WindowWidget.hpp"

/**
 * A #Widget wrapper that inserts a panel into the "parent chain" that
 * draws a form background using #SolidContainerWindow.
 */
class SolidWidget : public WindowWidget {
  Widget *const widget;

public:
  SolidWidget(Widget *_widget):widget(_widget) {}
  virtual ~SolidWidget() override;

  Widget &GetWidget() {
    return *widget;
  }

  virtual PixelSize GetMinimumSize() const override;
  virtual PixelSize GetMaximumSize() const override;
  virtual void Initialise(ContainerWindow &parent,
                          const PixelRect &rc) override;
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void Unprepare() override;
  virtual bool Save(bool &changed) override;
  virtual bool Click() override;
  virtual void ReClick() override;
  virtual void Show(const PixelRect &rc) override;
  virtual bool Leave() override;
  virtual void Hide() override;
  virtual void Move(const PixelRect &rc) override;
  virtual bool SetFocus() override;
  virtual bool KeyPress(unsigned key_code) override;
};

#endif
