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
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "../test/src/Fonts.hpp"
#include "ui/window/Init.hpp"
#include "ui/window/SingleWindow.hpp"
#include "system/Process.hpp"

#include <cassert>

enum Buttons {
  LAUNCH_XCSOAR = 100,
  LAUNCH_SHELL,
};

static DialogSettings dialog_settings;
static UI::SingleWindow *global_main_window;
static DialogLook *global_dialog_look;

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  return dialog_settings;
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  assert(global_dialog_look != nullptr);

  return *global_dialog_look;
}

UI::SingleWindow &
UIGlobals::GetMainWindow()
{
  assert(global_main_window != nullptr);

  return *global_main_window;
}

class MainMenuWidget final
  : public RowFormWidget
{
  enum Controls {
    XCSOAR,
    FILE,
    SYSTEM,
    SHELL,
    REBOOT,
    SHUTDOWN,
  };

  WndForm &dialog;

public:
  explicit MainMenuWidget(WndForm &_dialog) noexcept
    :RowFormWidget(_dialog.GetLook()),
     dialog(_dialog) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
MainMenuWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  AddButton("Start XCSoar", [this](){
    dialog.SetModalResult(LAUNCH_XCSOAR);
  });

  AddButton("File", [](){
    ShowMessageBox("Not yet implemented", nullptr, MB_OK);
  });

  AddButton("System", [](){
    ShowMessageBox("Not yet implemented", nullptr, MB_OK);
  });

  AddButton("Shell", [this](){
    dialog.SetModalResult(LAUNCH_SHELL);
  });

  AddButton("Reboot", [](){
    Run("/sbin/reboot");
  });

  AddButton("Power off", [](){
    Run("/sbin/poweroff");
  });
}

static int
Main(UI::SingleWindow &main_window, const DialogLook &dialog_look)
{
  TWidgetDialog<MainMenuWidget>
    dialog(WidgetDialog::Full{}, main_window,
           dialog_look, "OpenVario");
  dialog.SetWidget(dialog);

  return dialog.ShowModal();
}

static int
Main()
{
  dialog_settings.SetDefaults();

  ScreenGlobalInit screen_init;
  Layout::Initialise(screen_init.GetDisplay(), {600, 800});
  InitialiseFonts();

  DialogLook dialog_look;
  dialog_look.Initialise();

  UI::TopWindowStyle main_style;
  main_style.Resizable();

  UI::SingleWindow main_window{screen_init.GetDisplay()};
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
  int action = Main();

  switch (action) {
  case LAUNCH_XCSOAR:
    execl("/usr/bin/xcsoar", "xcsoar", "-fly", nullptr);
    perror("Failed to launch XCSoar");
    return EXIT_FAILURE;

  case LAUNCH_SHELL:
    execl("/bin/bash", "bash", "--login", nullptr);
    execl("/bin/ash", "-ash", nullptr);
    execl("/bin/ash", "-sh", nullptr);
    perror("Failed to launch shell");
    return EXIT_FAILURE;
  }

  return action;
}
