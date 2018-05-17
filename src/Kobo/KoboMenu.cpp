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

#include "Dialogs/DialogSettings.hpp"
#include "Dialogs/SimulatorPromptWindow.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/WindowWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Init.hpp"
#include "Screen/Layout.hpp"
#include "Event/KeyCode.hpp"
#include "../test/src/Fonts.hpp"
#include "Language/Language.hpp"
#include "Form/ActionListener.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Canvas.hpp"
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
static SingleWindow *global_main_window;
static DialogLook *global_dialog_look;

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  return dialog_settings;
}

SingleWindow &
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

class KoboMenuWidget final : public WindowWidget, ActionListener {
  ActionListener &dialog;
  SimulatorPromptWindow w;

public:
  KoboMenuWidget(const DialogLook &_look,
                 ActionListener &_dialog)
    :dialog(_dialog),
     w(_look, _dialog, false) {}

  void CreateButtons(WidgetDialog &buttons);

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

  virtual bool KeyPress(unsigned key_code) override;

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
KoboMenuWidget::CreateButtons(WidgetDialog &buttons)
{
  buttons.AddButton(("Nickel"), dialog, LAUNCH_NICKEL)
      ->SetEnabled(!IsKoboOTGKernel());
  buttons.AddButton(("Tools"), *this, TOOLS);
  buttons.AddButton(_("Network"), *this, NETWORK);
  buttons.AddButton("System", *this, SYSTEM);
  buttons.AddButton(("Poweroff"), dialog, POWEROFF);
}

void
KoboMenuWidget::Prepare(ContainerWindow &parent,
                        const PixelRect &rc)
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();

  w.Create(parent, rc, style);
  SetWindow(&w);
}

bool
KoboMenuWidget::KeyPress(unsigned key_code)
{
  switch (key_code) {
#ifdef KOBO
  case KEY_POWER:
    dialog.OnAction(POWEROFF);
    return true;
#endif

  default:
    return false;
  }
}

void
KoboMenuWidget::OnAction(int id)
{
  switch (id) {
  case TOOLS:
    ShowToolsDialog();
    break;

  case NETWORK:
    ShowNetworkDialog();
    break;

  case SYSTEM:
    ShowSystemDialog();
    break;
  }
}

static int
Main(SingleWindow &main_window, const DialogLook &dialog_look)
{
  WidgetDialog dialog(dialog_look);
  KoboMenuWidget widget(dialog_look, dialog);
  dialog.CreateFull(main_window, _T(""), &widget);
  widget.CreateButtons(dialog);

  const int result = dialog.ShowModal();
  dialog.StealWidget();
  return result;
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

  TopWindowStyle main_style;
  main_style.Resizable();

  SingleWindow main_window;
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

    case SimulatorPromptWindow::FLY:
      KoboRunXCSoar("-fly");
      /* return to menu after XCSoar quits */
      break;

    case SimulatorPromptWindow::SIMULATOR:
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
