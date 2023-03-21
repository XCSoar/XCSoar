// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

