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

#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "SimulatorPromptWindow.hpp"
#include "WidgetDialog.hpp"
#include "Widget/WindowWidget.hpp"
#include "UIGlobals.hpp"
#include "Simulator.hpp"

#ifdef SIMULATOR_AVAILABLE

class SimulatorPromptWidget final : public WindowWidget {
  SimulatorPromptWindow w;

public:
  SimulatorPromptWidget(const DialogLook &_look,
                        ActionListener &_action_listener)
    :w(_look, _action_listener, true) {}

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    WindowStyle style;
    style.Hide();
    style.ControlParent();

    w.Create(parent, rc, style);
    SetWindow(&w);
  }
};

#endif

SimulatorPromptResult
dlgSimulatorPromptShowModal()
{
#ifdef SIMULATOR_AVAILABLE
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  SimulatorPromptWidget widget(look, dialog);

  dialog.CreateFull(UIGlobals::GetMainWindow(), _T(""), &widget);

  const int result = dialog.ShowModal();
  dialog.StealWidget();

  switch (result) {
  case SimulatorPromptWindow::FLY:
    return SPR_FLY;

  case SimulatorPromptWindow::SIMULATOR:
    return SPR_SIMULATOR;

  default:
    return SPR_QUIT;
  }
#else
  return SPR_FLY;
#endif
}

