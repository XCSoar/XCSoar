/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_TAB_WIDGET_HPP
#define XCSOAR_TAB_WIDGET_HPP

#include "PagerWidget.hpp"

#include <assert.h>
#include <tchar.h>

class MaskedIcon;
class TabDisplay;

/**
 * A #PagerWidget that navigates with a #TabDisplay.
 */
class TabWidget : public PagerWidget {
public:
  enum class Orientation {
    AUTO,
    VERTICAL,
    HORIZONTAL,
  };

private:
  struct Layout {
    PixelRect tab_display, extra, pager;

    bool vertical;

    Layout(Orientation orientation, PixelRect rc,
           const TabDisplay &td, const Widget *e);

    static bool IsVertical(Orientation orientation, PixelRect rc) {
      switch (orientation) {
      case Orientation::AUTO:
        break;

      case Orientation::VERTICAL:
        return true;

      case Orientation::HORIZONTAL:
        return false;
      }

      return rc.GetWidth() > rc.GetHeight();
    }
  };

  const Orientation orientation;

  TabDisplay *tab_display;

  /**
   * An optional #Widget that is shown at the corner right or below
   * the tabs.
   */
  Widget *extra;

  PixelRect extra_position;

  bool large_extra;

public:
  explicit TabWidget(Orientation _orientation=Orientation::AUTO,
                     Widget *_extra=nullptr)
    :orientation(_orientation), tab_display(nullptr),
     extra(_extra), large_extra(false) {}

  ~TabWidget() override;

  /**
   * Must be called before Initialise().
   */
  void SetExtra(Widget *_extra) {
    assert(extra == nullptr);
    assert(_extra != nullptr);

    extra = _extra;
    large_extra = false;
  }

  Widget &GetExtra() {
    assert(extra != nullptr);

    return *extra;
  }

  /**
   * Make the "extra" Widget large, as if it were a page.
   */
  void LargeExtra();

  const PixelRect &GetEffectiveExtraPosition() const {
    assert(extra != nullptr);

    return large_extra
      ? PagerWidget::GetPosition()
      : extra_position;
  }

  /**
   * Restore the "extra" widget to regular size.
   */
  void RestoreExtra();

  void ToggleLargeExtra() {
    if (large_extra)
      RestoreExtra();
    else
      LargeExtra();
  }

  void AddTab(Widget *widget, const TCHAR *caption,
              const MaskedIcon *icon=nullptr);

  gcc_pure
  const TCHAR *GetButtonCaption(unsigned i) const;

  bool ClickPage(unsigned i);
  bool NextPage();
  bool PreviousPage();

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const override;
  PixelSize GetMaximumSize() const override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;
  void Show(const PixelRect &rc) override;
  void Hide() override;
  void Move(const PixelRect &rc) override;
  bool SetFocus() override;
  bool KeyPress(unsigned key_code) override;

protected:
  /* virtual methods from class PagerWidget */
  void OnPageFlipped() override;
};

#endif
