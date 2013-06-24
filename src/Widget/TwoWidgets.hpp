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

#ifndef XCSOAR_ROW_TWO_WIDGETS_HPP
#define XCSOAR_ROW_TWO_WIDGETS_HPP

#include "Widget.hpp"
#include "Screen/Point.hpp"

#include <utility>
#include <assert.h>

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

  Widget *first, *second;

  PixelRect rc;

public:
  TwoWidgets(bool _vertical=true):vertical(_vertical) {}

  TwoWidgets(Widget *_first, Widget *_second, bool _vertical=true)
    :vertical(_vertical), first(_first), second(_second) {
    assert(first != NULL);
    assert(second != NULL);
  }

  virtual ~TwoWidgets();

  /**
   * Update the layout after one of the widgets has indicated a size
   * change.  This may only be called between Prepare() and
   * Unprepare().
   */
  void UpdateLayout();

protected:
  /**
   * Call this method if the default constructor has been used.  It
   * must be called before Initialise().
   */
  void Set(Widget *_first, Widget *_second) {
    first = _first;
    second = _second;

    assert(first != NULL);
    assert(second != NULL);
  }

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

  gcc_pure
  PixelScalar CalculateSplit(const PixelRect &rc) const;

  gcc_pure
  std::pair<PixelRect,PixelRect> CalculateLayout(const PixelRect &rc) const;

public:
  /* virtual methods from Widget */
  virtual PixelSize GetMinimumSize() const override;
  virtual PixelSize GetMaximumSize() const override;
  virtual void Initialise(ContainerWindow &parent,
                          const PixelRect &rc) override;
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
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
