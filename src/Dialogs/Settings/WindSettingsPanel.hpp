// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "Blackboard/BlackboardListener.hpp"

class Button;

class WindSettingsPanel final
  : public RowFormWidget,
    private DataFieldListener, private NullBlackboardListener {
  enum ControlIndex {
    CIRCLING_WIND,
    ZIG_ZAG_WIND,
    EXTERNAL_WIND,
    TrailDrift,
    SOURCE,
    Speed,
    Direction,
    CLEAR_MANUAL_BUTTON,
  };

  const bool edit_manual_wind, clear_manual_button, edit_trail_drift;

  /**
   * Has the user modified the manual wind?
   */
  bool manual_modified;

  Button *clear_manual_window;

public:
  enum Buttons {
    /**
     * Clears the manual wind.
     */
    CLEAR_MANUAL,
  };

  /**
   * @param manual_wind edit the manual wind setting
   * @param clear_manual_button add a "Clear" button
   */
  WindSettingsPanel(bool edit_manual_wind, bool clear_manual_button,
                    bool edit_trail_drift) noexcept;

  void SetClearManualButton(Button *_button) noexcept {
    clear_manual_window = _button;
  }

  void ClearManual() noexcept;

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  void UpdateVector() noexcept;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;

  /* virtual methods from class BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) override;
};
