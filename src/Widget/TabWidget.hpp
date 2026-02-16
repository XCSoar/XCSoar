// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PagerWidget.hpp"
#include "Form/TabHandler.hpp"

#include <cassert>
#include <memory>
#include <tchar.h>

class MaskedIcon;
class TabDisplay;

/**
 * A #PagerWidget that navigates with a #TabDisplay.
 */
class TabWidget : public PagerWidget, TabHandler {
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

    [[nodiscard]]
    Layout(Orientation orientation, PixelRect rc,
           const TabDisplay &td, const Widget *e) noexcept;

    static constexpr bool IsVertical(Orientation orientation, PixelRect rc) noexcept {
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

  Widget &GetExtra() noexcept {
    assert(extra != nullptr);

    return *extra;
  }

  /**
   * Make the "extra" Widget large, as if it were a page.
   */
  void LargeExtra() noexcept;

  const PixelRect &GetEffectiveExtraPosition() const noexcept {
    assert(extra != nullptr);

    return large_extra
      ? PagerWidget::GetPosition()
      : extra_position;
  }

  /**
   * Restore the "extra" widget to regular size.
   */
  void RestoreExtra() noexcept;

  void ToggleLargeExtra() noexcept {
    if (large_extra)
      RestoreExtra();
    else
      LargeExtra();
  }

  void AddTab(std::unique_ptr<Widget> widget, const char *caption,
              const MaskedIcon *icon=nullptr) noexcept;

  [[gnu::pure]]
  const char *GetButtonCaption(unsigned i) const noexcept;

public:
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

private:
  /* virtual methods from class TabHandler */
  bool ClickPage(unsigned i) noexcept override;
  bool NextPage() noexcept override;
  bool PreviousPage() noexcept override;
};
