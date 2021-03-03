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

#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "SimulatorPromptWindow.hpp"
#include "WidgetDialog.hpp"
#include "Widget/WindowWidget.hpp"
#include "UIGlobals.hpp"
#include "Simulator.hpp"

#ifdef SIMULATOR_AVAILABLE

class SimulatorPromptWidget final : public WindowWidget {
  const DialogLook &look;
  std::function<void(SimulatorPromptWindow::Result)> callback;

public:
  SimulatorPromptWidget(const DialogLook &_look,
                        std::function<void(SimulatorPromptWindow::Result)> _callback) noexcept
    :look(_look), callback(std::move(_callback)) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();
    style.ControlParent();

    auto w = std::make_unique<SimulatorPromptWindow>(look, std::move(callback),
                                                     true);
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

#endif

SimulatorPromptResult
dlgSimulatorPromptShowModal()
{
#ifdef SIMULATOR_AVAILABLE
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<SimulatorPromptWidget> dialog(WidgetDialog::Full{},
                                              UIGlobals::GetMainWindow(),
                                              look, nullptr);

  SimulatorPromptResult result = SPR_QUIT;
  dialog.SetWidget(look, [&](SimulatorPromptWindow::Result r){
    switch (r) {
    case SimulatorPromptWindow::Result::FLY:
      result = SPR_FLY;
      break;

    case SimulatorPromptWindow::Result::SIMULATOR:
      result = SPR_SIMULATOR;
      break;

    case SimulatorPromptWindow::Result::QUIT:
      result = SPR_QUIT;
      break;
    }

    dialog.SetModalResult(mrOK);
  });

  dialog.ShowModal();

  return result;
#else
  return SPR_FLY;
#endif
}

