// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/dlgSimulatorPrompt.hpp"
#include "SimulatorPromptWindow.hpp"
#include "WidgetDialog.hpp"
#include "Widget/WindowWidget.hpp"
#include "UIGlobals.hpp"
#include "Simulator.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/window/SingleWindow.hpp"

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

  bool KeyPress(unsigned key_code) noexcept override {
    /* Bitmap buttons have no visible focus; activate immediately. */
    auto &prompt = (SimulatorPromptWindow &)GetWindow();
    switch (key_code) {
    case KEY_LEFT:
      prompt.SelectFly();
      return true;

    case KEY_RIGHT:
      prompt.SelectSimulator();
      return true;

    default:
      return false;
    }
  }
};

#endif

SimulatorPromptResult
dlgSimulatorPromptShowModal()
{
#ifdef SIMULATOR_AVAILABLE
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto &main_window = UIGlobals::GetMainWindow();
  TWidgetDialog<SimulatorPromptWidget> dialog(WidgetDialog::Full{},
                                              main_window, look, nullptr);
  /* Full{} uses the safe area; expand to the client so the gradient
     can paint edge to edge.  SimulatorPromptWindow keeps Quit, Fly,
     Simulator and the version string inside the safe area. */
  dialog.Move(main_window.GetClientRect());

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
  dialog.ForceLayout();

  dialog.ShowModal();

  return result;
#else
  return SPR_FLY;
#endif
}

