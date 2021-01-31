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

#ifndef XCSOAR_ROW_TWO_WIDGETS_HPP
#define XCSOAR_ROW_TWO_WIDGETS_HPP

#include "Widget.hpp"
#include "ui/dim/Rect.hpp"

#include <memory>
#include <utility>

/**
 * A #Widget that contains two other widgets, the second one following
 * below the first one.  The two #Widget pointers are deleted in the
 * destructor.
 *
 * If you construct this object with vertical=false, the second Widget
 * will be right of the first one.
 */
class TwoWidgets : public NullWidget {
  const bool vertical;

  const std::unique_ptr<Widget> first, second;

  PixelRect rc;

public:
  TwoWidgets(std::unique_ptr<Widget> &&_first, std::unique_ptr<Widget> &&_second, bool _vertical=true) noexcept
    :vertical(_vertical),
     first(std::move(_first)),
     second(std::move(_second)) {}

  /**
   * Update the layout after one of the widgets has indicated a size
   * change.  This may only be called between Prepare() and
   * Unprepare().
   */
  void UpdateLayout();

  Widget &GetFirst() {
    return *first;
  }

  const Widget &GetFirst() const {
    return *first;
  }

  Widget &GetSecond() {
    return *second;
  }

  const Widget &GetSecond() const {
    return *second;
  }

protected:
  gcc_pure
  int CalculateSplit(const PixelRect &rc) const;

  gcc_pure
  std::pair<PixelRect,PixelRect> CalculateLayout(const PixelRect &rc) const;

public:
  /* virtual methods from Widget */
  PixelSize GetMinimumSize() const override;
  PixelSize GetMaximumSize() const override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;
  bool Save(bool &changed) override;
  bool Click() override;
  void ReClick() override;
  void Show(const PixelRect &rc) override;
  bool Leave() override;
  void Hide() override;
  void Move(const PixelRect &rc) override;
  bool SetFocus() override;
  bool KeyPress(unsigned key_code) override;
};

#endif
