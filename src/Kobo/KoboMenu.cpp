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

#include "Dialogs/DialogSettings.hpp"
#include "Dialogs/SimulatorPromptWindow.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/WindowWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "ui/window/Init.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "../test/src/Fonts.hpp"
#include "Language/Language.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Kernel.hpp"
#include "System.hpp"
#include "NetworkDialog.hpp"
#include "SystemDialog.hpp"
#include "ToolsDialog.hpp"

enum Buttons {
  LAUNCH_NICKEL = 100,
  TOOLS,
  NETWORK,
  SYSTEM,
  POWEROFF
};

static DialogSettings dialog_settings;
static UI::SingleWindow *global_main_window;
static DialogLook *global_dialog_look;

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  return dialog_settings;
}

UI::SingleWindow &
UIGlobals::GetMainWindow()
{
  assert(global_main_window != nullptr);

  return *global_main_window;
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  assert(global_dialog_look != nullptr);

  return *global_dialog_look;
}

class KoboMenuWidget final : public WindowWidget {
  WndForm &dialog;

public:
  KoboMenuWidget(const DialogLook &_look,
                 WndForm &_dialog)
    :dialog(_dialog) {}

  void CreateButtons(WidgetDialog &buttons);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};

void
KoboMenuWidget::CreateButtons(WidgetDialog &buttons)
{
  buttons.AddButton(("Nickel"), dialog.MakeModalResultCallback(LAUNCH_NICKEL))
      ->SetEnabled(!IsKoboOTGKernel());
  buttons.AddButton(("Tools"), [](){ ShowToolsDialog(); });
  buttons.AddButton(_("Network"), [](){ ShowNetworkDialog(); });
  buttons.AddButton("System", [](){ ShowSystemDialog(); });
  buttons.AddButton(("Poweroff"), dialog.MakeModalResultCallback(POWEROFF));
}

void
KoboMenuWidget::Prepare(ContainerWindow &parent,
                        const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();

  auto w = std::make_unique<SimulatorPromptWindow>(dialog.GetLook(),
                                                   [this](SimulatorPromptWindow::Result result){
                                                     dialog.SetModalResult(int(result));
                                                   }, false);
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

bool
KoboMenuWidget::KeyPress(unsigned key_code) noexcept
{
  switch (key_code) {
#ifdef KOBO
  case KEY_POWER:
    dialog.SetModalResult(POWEROFF);
    return true;
#endif

  default:
    return false;
  }
}

static int
Main(UI::SingleWindow &main_window, const DialogLook &dialog_look)
{
  TWidgetDialog<KoboMenuWidget>
    dialog(WidgetDialog::Full{}, main_window,
           dialog_look, nullptr);
  dialog.SetWidget(dialog_look, dialog);
  dialog.GetWidget().CreateButtons(dialog);

  return dialog.ShowModal();
}

static int
Main()
{
  dialog_settings.SetDefaults();

  ScreenGlobalInit screen_init;
  Layout::Initialize({600, 800});
  InitialiseFonts();

  DialogLook dialog_look;
  dialog_look.Initialise();

  UI::TopWindowStyle main_style;
  main_style.Resizable();

  UI::SingleWindow main_window;
  main_window.Create(_T("XCSoar/KoboMenu"), {600, 800}, main_style);
  main_window.Show();

  global_dialog_look = &dialog_look;
  global_main_window = &main_window;

  int action = Main(main_window, dialog_look);

  main_window.Destroy();

  DeinitialiseFonts();

  return action;
}

int main(int argc, char **argv)
{
  while (true) {
    int action = Main();

    switch (action) {
    case LAUNCH_NICKEL:
      KoboExecNickel();
      return EXIT_FAILURE;

    case int(SimulatorPromptWindow::Result::FLY):
      KoboRunXCSoar("-fly");
      /* return to menu after XCSoar quits */
      break;

    case int(SimulatorPromptWindow::Result::SIMULATOR):
      KoboRunXCSoar("-simulator");
      /* return to menu after XCSoar quits */
      break;

    case POWEROFF:
      KoboPowerOff();
      return EXIT_SUCCESS;

    default:
      return EXIT_SUCCESS;
    }
  }
}
