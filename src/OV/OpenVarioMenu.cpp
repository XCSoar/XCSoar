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
#include "Dialogs/ProcessDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "../test/src/Fonts.hpp"
#include "ui/window/Init.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/event/Queue.hpp"
#include "ui/event/Timer.hpp"
#include "Language/Language.hpp"
#include "system/Process.hpp"
#include "util/ScopeExit.hxx"

#include <cassert>

enum Buttons {
  LAUNCH_SHELL = 100,
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

class FileMenuWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

public:
  FileMenuWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 const DialogLook &look) noexcept
    :RowFormWidget(look),
     display(_display), event_queue(_event_queue) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
FileMenuWidget::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  AddButton("Download XCSoar IGC files to USB", [this](){
    const UI::ScopeDropMaster drop_master{display};
    const UI::ScopeSuspendEventQueue suspend_event_queue{event_queue};
    Run("/usr/bin/download-igc.sh");
  });

  AddButton("Download XCSoar to USB", [](){
    static constexpr const char *argv[] = {
      "/usr/bin/download-all.sh", nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Downloading files", argv);
  });

  AddButton("Upload files from USB to XCSoar", [](){
    static constexpr const char *argv[] = {
      "/usr/bin/upload-xcsoar.sh", nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Uploading files", argv);
  });
}

class SystemMenuWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

public:
  SystemMenuWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                   const DialogLook &look) noexcept
    :RowFormWidget(look),
     display(_display), event_queue(_event_queue) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

static void
CalibrateSensors() noexcept
{
  /* make sure sensord is stopped while calibrating sensors */
  static constexpr const char *start_sensord[] = {
    "/bin/systemctl", "start", "sensord.service", nullptr
  };
  static constexpr const char *stop_sensord[] = {
    "/bin/systemctl", "stop", "sensord.service", nullptr
  };

  RunProcessDialog(UIGlobals::GetMainWindow(),
                   UIGlobals::GetDialogLook(),
                   "Calibrate Sensors", stop_sensord,
                   [](int status){
                     return status == EXIT_SUCCESS ? mrOK : 0;
                   });

  AtScopeExit(){
    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Calibrate Sensors", start_sensord,
                     [](int status){
                       return status == EXIT_SUCCESS ? mrOK : 0;
                     });
  };

  /* calibrate the sensors */
  static constexpr const char *calibrate_sensors[] = {
    "/opt/bin/sensorcal", "-c", nullptr
  };

  static constexpr int STATUS_BOARD_NOT_INITIALISED = 2;
  static constexpr int RESULT_BOARD_NOT_INITIALISED = 100;
  int result = RunProcessDialog(UIGlobals::GetMainWindow(),
                                UIGlobals::GetDialogLook(),
                                "Calibrate Sensors", calibrate_sensors,
                                [](int status){
                                  return status == STATUS_BOARD_NOT_INITIALISED
                                    ? RESULT_BOARD_NOT_INITIALISED
                                    : 0;
                                });
  if (result != RESULT_BOARD_NOT_INITIALISED)
    return;

  /* initialise the sensors? */
  if (ShowMessageBox("Sensorboard is virgin. Do you want to initialise it?",
                     "Calibrate Sensors", MB_YESNO) != IDYES)
    return;

  static constexpr const char *init_sensors[] = {
    "/opt/bin/sensorcal", "-i", nullptr
  };

  result = RunProcessDialog(UIGlobals::GetMainWindow(),
                            UIGlobals::GetDialogLook(),
                            "Calibrate Sensors", init_sensors,
                            [](int status){
                              return status == EXIT_SUCCESS
                                ? mrOK
                                : 0;
                            });
  if (result != mrOK)
    return;

  /* calibrate again */
  RunProcessDialog(UIGlobals::GetMainWindow(),
                   UIGlobals::GetDialogLook(),
                   "Calibrate Sensors", calibrate_sensors,
                   [](int status){
                     return status == STATUS_BOARD_NOT_INITIALISED
                       ? RESULT_BOARD_NOT_INITIALISED
                       : 0;
                   });
}

void
SystemMenuWidget::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  AddButton("Update System", [](){
    static constexpr const char *argv[] = {
      "/usr/bin/update-system.sh", nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Update System", argv);
  });

  AddButton("Update Maps", [](){
    static constexpr const char *argv[] = {
      "/usr/bin/update-maps.sh", nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Update Maps", argv);
  });

  AddButton("Calibrate Sensors", CalibrateSensors);
  AddButton("Calibrate Touch", [this](){
    const UI::ScopeDropMaster drop_master{display};
    const UI::ScopeSuspendEventQueue suspend_event_queue{event_queue};
    Run("/usr/bin/ov-calibrate-ts.sh");
  });

  AddButton("System Settings", [this](){
    const UI::ScopeDropMaster drop_master{display};
    const UI::ScopeSuspendEventQueue suspend_event_queue{event_queue};
    Run("/usr/lib/openvario/libexec/system_settings.sh");
  });

  AddButton("System Info", [this](){
    const UI::ScopeDropMaster drop_master{display};
    const UI::ScopeSuspendEventQueue suspend_event_queue{event_queue};
    Run("/usr/lib/openvario/libexec/system_info.sh");
  });
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
    TIMER,
  };

  UI::Display &display;
  UI::EventQueue &event_queue;

  WndForm &dialog;

  UI::Timer timer{[this](){
    if (--remaining_seconds == 0) {
      HideRow(Controls::TIMER);
      StartXCSoar();
    } else {
      ScheduleTimer();
    }
  }};

  unsigned remaining_seconds = 3;

public:
  MainMenuWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 WndForm &_dialog) noexcept
    :RowFormWidget(_dialog.GetLook()),
     display(_display), event_queue(_event_queue),
     dialog(_dialog) {}

private:
  void StartXCSoar() noexcept {
    const UI::ScopeDropMaster drop_master{display};
    const UI::ScopeSuspendEventQueue suspend_event_queue{event_queue};
    Run("/usr/bin/xcsoar", "-fly");
  }

  void ScheduleTimer() noexcept {
    assert(remaining_seconds > 0);

    timer.Schedule(std::chrono::seconds{1});

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Starting XCSoar in %u seconds (press any key to cancel)",
             remaining_seconds);
    SetText(Controls::TIMER, buffer);
  }

  void CancelTimer() noexcept {
    timer.Cancel();
    remaining_seconds = 0;
    HideRow(Controls::TIMER);
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    RowFormWidget::Show(rc);

    if (remaining_seconds > 0)
      ScheduleTimer();
  }

  void Hide() noexcept override {
    CancelTimer();
    RowFormWidget::Hide();
  }

  bool KeyPress(unsigned key_code) noexcept override {
    CancelTimer();
    return RowFormWidget::KeyPress(key_code);
  }
};

void
MainMenuWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  AddButton("Start XCSoar", [this](){
    CancelTimer();
    StartXCSoar();
  });

  AddButton("File", [this](){
    CancelTimer();

    TWidgetDialog<FileMenuWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "OpenVario File");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });

  AddButton("System", [this](){
    CancelTimer();

    TWidgetDialog<SystemMenuWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "OpenVario System");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
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

  AddReadOnly("");
}

static int
Main(UI::EventQueue &event_queue, UI::SingleWindow &main_window,
     const DialogLook &dialog_look)
{
  TWidgetDialog<MainMenuWidget>
    dialog(WidgetDialog::Full{}, main_window,
           dialog_look, "OpenVario");
  dialog.SetWidget(main_window.GetDisplay(), event_queue, dialog);

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

  int action = Main(screen_init.GetEventQueue(), main_window, dialog_look);

  main_window.Destroy();

  DeinitialiseFonts();

  return action;
}

int main(int argc, char **argv)
{
  int action = Main();

  switch (action) {
  case LAUNCH_SHELL:
    execl("/bin/bash", "bash", "--login", nullptr);
    execl("/bin/ash", "-ash", nullptr);
    execl("/bin/ash", "-sh", nullptr);
    perror("Failed to launch shell");
    return EXIT_FAILURE;
  }

  return action;
}
