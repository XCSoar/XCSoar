/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_INFO_BOX_MANAGER_HPP
#define XCSOAR_INFO_BOX_MANAGER_HPP

#include "Screen/PaintWindow.hpp"
#include "InfoBoxes/Content/Base.hpp"
#include "Profile/InfoBoxConfig.hpp"

extern InfoBoxManagerConfig infoBoxManagerConfig;

struct InfoBoxLook;
class InfoBoxWindow;

namespace InfoBoxLayout {
  struct Layout;
};

class InfoBoxFullWindow : public PaintWindow {
protected:
  virtual void on_paint(Canvas &canvas);
};

namespace InfoBoxManager
{
  enum PanelSelection {
    PANEL_CIRCLING,
    PANEL_CRUISE,
    PANEL_FINAL_GLIDE,
    PANEL_AUXILIARY,
  };

  extern InfoBoxLayout::Layout layout;

  void Event_Select(int i);
  void Event_Change(int i);

  /**
   * The following two handle direct access to InfoBox values.
   * ProcessKey takes an keycode and expects the target InfoBox
   * to be focussed.
   * @param keycode
   */
  void ProcessKey(InfoBoxContent::InfoBoxKeyCodes keycode);
  /**
   * ProcessQuickAccess takes the id of the InfoBox where to pass the
   * value Value. It doesn't expect the target InfoBox to be focussed.
   * @param id
   * @param Value
   */
  void ProcessQuickAccess(const int id, const TCHAR *Value);

  bool Click(InfoBoxWindow &ib);

  void ProcessTimer();
  void SetDirty();

  void Create(PixelRect rc, const InfoBoxLayout::Layout &layout,
              const InfoBoxLook &look);
  void Destroy();
  void Paint();
  void Show();
  void Hide();

  InfoBoxContent::DialogContent* GetDialogContent(const int id);

  unsigned GetCurrentPanel();
  const TCHAR* GetCurrentPanelName();
  const TCHAR* GetPanelName(unsigned panel);

  unsigned GetType(unsigned box, unsigned panel);
  void SetType(unsigned box, unsigned type, unsigned panel);
  void SetCurrentType(unsigned box, unsigned type);
  const TCHAR* GetTitle(unsigned box);

  bool IsEmpty(unsigned panel);

  bool HasFocus();

  void ShowDlgInfoBox(const int id);

  /**
   * Opens a configuration dialog for the focused InfoBox.
   */
  void SetupFocused(const int id = -1);
};

#endif
