// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PagerWidget.hpp"

#include <cassert>
#include <memory>
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

  TabDisplay *tab_display = nullptr;

  /**
   * An optional #Widget that is shown at the corner right or below
   * the tabs.
   */
  std::unique_ptr<Widget> extra;

  PixelRect extra_position;

  bool large_extra = false;

public:
  explicit TabWidget(Orientation _orientation=Orientation::AUTO,
                     std::unique_ptr<Widget> &&_extra=nullptr) noexcept
    :orientation(_orientation),
     extra(std::move(_extra)) {}

  ~TabWidget() noexcept override;

  /**
   * Must be called before Initialise().
   */
  void SetExtra(std::unique_ptr<Widget> &&_extra) noexcept {
    assert(extra == nullptr);
    assert(_extra != nullptr);

    extra = std::move(_extra);
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

  void AddTab(std::unique_ptr<Widget> widget, const TCHAR *caption,
              const MaskedIcon *icon=nullptr);

  [[gnu::pure]]
  const TCHAR *GetButtonCaption(unsigned i) const;

  bool ClickPage(unsigned i);
  bool NextPage();
  bool PreviousPage();

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

protected:
  /* virtual methods from class PagerWidget */
  void OnPageFlipped() noexcept override;
};
