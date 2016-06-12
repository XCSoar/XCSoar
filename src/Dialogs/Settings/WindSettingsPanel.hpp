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

#ifndef XCSOAR_WIND_SETTINGS_PANEL_HPP
#define XCSOAR_WIND_SETTINGS_PANEL_HPP

#include "Widget/RowFormWidget.hpp"
#include "Form/ActionListener.hpp"
#include "Form/DataField/Listener.hpp"
#include "Blackboard/BlackboardListener.hpp"

class Button;

class WindSettingsPanel final
  : public RowFormWidget, public ActionListener,
    private DataFieldListener, private NullBlackboardListener {
  enum ControlIndex {
    AutoWind,
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
                    bool edit_trail_drift);

  void SetClearManualButton(Button *_button) {
    clear_manual_window = _button;
  }

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;

  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;

private:
  void UpdateVector();

  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;

  /* virtual methods from class BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) override;
};

#endif
