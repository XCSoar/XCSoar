// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
FileMenuWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                        [[maybe_unused]] const PixelRect &rc) noexcept
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

class ScreenLanguageWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

public:
  ScreenLanguageWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 const DialogLook &look) noexcept
    :RowFormWidget(look),
     display(_display), event_queue(_event_queue) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

};

void
ScreenLanguageWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                              [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton("English", [this](){
    ChangeConfigString("LANG", "en_EN.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to English",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Englisch", argv);
  });

  AddButton("Deutsch", [this](){
    ChangeConfigString("LANG", "de_DE.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to German",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Deutsch", argv);
  });

  AddButton("Français", [this](){
    ChangeConfigString("LANG", "fr_FR.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to French",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Français", argv);
  });

  AddButton("Italiano", [this](){
    ChangeConfigString("LANG", "it_IT.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Italian",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Italiano", argv);
  });

  AddButton("Magyar", [this](){;
    ChangeConfigString("LANG", "hu_HU.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Hungarian",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Magyar", argv);
  });

  AddButton("Polski", [this](){
    ChangeConfigString("LANG", "pl_PL.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Polish",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Polski", argv);
  });

  AddButton("Čeština", [this](){
    ChangeConfigString("LANG", "cs_CZ.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Czech",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Čeština", argv);
  });

  AddButton("Slovenčina", [this](){
    ChangeConfigString("LANG", "sk_SK.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Slovak",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Slovenčina", argv);
  });

  AddButton("Lietuvių", [this](){
    ChangeConfigString("LANG", "lt_LT.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Lithuanian",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Lietuvių", argv);
  });

  AddButton("Русский", [this](){
    ChangeConfigString("LANG", "ru_RU.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Russian",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Русский", argv);
  });

  AddButton("Español", [this](){
    ChangeConfigString("LANG", "es_ES.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Spanish",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Español", argv);
  });

  AddButton("Dutch", [this](){
    ChangeConfigString("LANG", "nl_NL.UTF-8", "/etc/locale.conf");
    static constexpr const char *argv[] = {
      "/bin/sh", "-c",
      "echo The language has been set to Dutch",
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Dutch", argv);
  });
}

class ScreenTimeoutWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

public:
  ScreenTimeoutWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 const DialogLook &look) noexcept
    :RowFormWidget(look),
     display(_display), event_queue(_event_queue) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void SaveTimeout(int timeoutvalue);
};

void
ScreenTimeoutWidget::SaveTimeout(int timeoutInt)
{
   ChangeConfigInt("timeout", timeoutInt, "/boot/config.uEnv");
}

void
ScreenTimeoutWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                             [[maybe_unused]] const PixelRect &rc) noexcept

{
  AddButton("immediately", [this](){
    SaveTimeout(0);
        static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "echo Automatic timeout was set to 0s", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "immediately", argv);
  });

  AddButton("1s", [this](){
    SaveTimeout(1);
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "echo Automatic timeout was set to 1s", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "1s", argv);
  });

  AddButton("3s", [this](){
    SaveTimeout(3);
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "echo Automatic timeout was set to 3s", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "3s", argv);
  });

  AddButton("5s", [this](){
    SaveTimeout(5);
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "echo Automatic timeout was set to 5s", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "5s", argv);
  });

  AddButton("10s", [this](){
    SaveTimeout(10);
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "echo Automatic timeout was set to 10s", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "10s", argv);
  });

  AddButton("30s", [this](){
    SaveTimeout(30);
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "echo Automatic timeout was set to 30s", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "30s", argv);
  });
}

class ScreenSSHWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

public:
  ScreenSSHWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 const DialogLook &look) noexcept
    :RowFormWidget(look),
     display(_display), event_queue(_event_queue) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
ScreenSSHWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                         [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton("Enable", [](){
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "systemctl enable dropbear.socket && printf '\nSSH has been enabled'", 
      
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Enable", argv);
  });

  AddButton("Disable", [](){
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "systemctl disable dropbear.socket && printf '\nSSH has been disabled'", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Disable", argv);
  });
}

class ScreenVariodWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

public:
  ScreenVariodWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 const DialogLook &look) noexcept
    :RowFormWidget(look),
     display(_display), event_queue(_event_queue) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
ScreenVariodWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton("Enable", [](){
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "systemctl enable variod && printf '\nvariod has been enabled'", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Enable", argv);
  });

  AddButton("Disable", [](){
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "systemctl disable variod && printf '\nvariod has been disabled'", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Disable", argv);
  });
}

class SystemSettingsWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

  WndForm &dialog;

public:
  SystemSettingsWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 WndForm &_dialog) noexcept 
    :RowFormWidget(_dialog.GetLook()),
     display(_display), event_queue(_event_queue),
     dialog(_dialog) {}

private:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
SystemSettingsWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                              [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton("Screen Rotation", [this](){
    TWidgetDialog<ScreenRotationWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "Display Rotation Settings");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });

  AddButton("Screen Brightness", [this](){
    TWidgetDialog<ScreenBrightnessWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "Display Brightness Settings");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });

  AddButton("Language", [this](){
    TWidgetDialog<ScreenLanguageWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "Language Settings");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });

  AddButton("Autostart Timeout", [this](){
    TWidgetDialog<ScreenTimeoutWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "Autostart Timeout");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });

  AddButton("SSH", [this](){
    TWidgetDialog<ScreenSSHWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "Enable or Disable SSH");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });

  AddButton("Variod", [this](){
    TWidgetDialog<ScreenVariodWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "Enable or Disable Variod");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });

  AddButton("Sensord", [this](){
    TWidgetDialog<ScreenSensordWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "Enable or Disable Sensord");
    sub_dialog.SetWidget(display, event_queue, GetLook());
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });
}

class SystemMenuWidget final
  : public RowFormWidget
{
  UI::Display &display;
  UI::EventQueue &event_queue;

  WndForm &dialog;

public:
  SystemMenuWidget(UI::Display &_display, UI::EventQueue &_event_queue,
          WndForm &_dialog) noexcept
    :RowFormWidget(_dialog.GetLook()),
     display(_display), event_queue(_event_queue),
     dialog(_dialog) {}

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
SystemMenuWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton("WiFi Settings", [](){
    static constexpr const char *argv[] = {
      "/bin/sh", "-c", 
      "printf '\nWiFi-Settings are not implemented, yet!! \n\nIf you are interessted to help with this, write me an email: dirk@freevario.de'", 
      nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "WiFi Settings", argv);
  });

  AddButton("Update System", [](){
    static constexpr const char *argv[] = {
      "/usr/bin/update-system.sh", nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Update System", argv);
  });

  AddButton("Calibrate Sensors", CalibrateSensors);
  AddButton("Calibrate Touch", [this](){
    const UI::ScopeDropMaster drop_master{display};
    const UI::ScopeSuspendEventQueue suspend_event_queue{event_queue};
    Run("/usr/bin/ov-calibrate-ts.sh");
  });

  AddButton("System Settings", [this](){
      
    TWidgetDialog<SystemSettingsWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "OpenVario System Settings");
    sub_dialog.SetWidget(display, event_queue, sub_dialog); 
    sub_dialog.AddButton(_("Close"), mrOK);
    return sub_dialog.ShowModal();
  });  

  AddButton("System Info", [](){
    static constexpr const char *argv[] = {
      "/usr/bin/system-info.sh", nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "System Info", argv);
  });
}

class MainMenuWidget final
  : public RowFormWidget
{
  int remaining_seconds = 3;

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

public:
  MainMenuWidget(UI::Display &_display, UI::EventQueue &_event_queue,
                 WndForm &_dialog) noexcept
    :RowFormWidget(_dialog.GetLook()),
     display(_display), event_queue(_event_queue),
     dialog(_dialog)
     {
       GetConfigInt("timeout", remaining_seconds, "/boot/config.uEnv");
     }

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

    if (remaining_seconds > 0) {
      ScheduleTimer();
    }
    else {
	  HideRow(Controls::TIMER);
      StartXCSoar();
	}
  }

  void Hide() noexcept override {
    CancelTimer();
    RowFormWidget::Hide();
  }

  bool KeyPress(unsigned key_code) noexcept override {
    CancelTimer();

  /* ignore escape key at first menu page */
    if (key_code != KEY_ESCAPE) {
	    return RowFormWidget::KeyPress(key_code);
	}
	else {
        return true;
	}
  }
};

void
MainMenuWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
			[[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton("Start XCSoar", [this](){
    CancelTimer();
    StartXCSoar();
  });

  AddButton("Logbook", [this](){
    CancelTimer();
    static constexpr const char *argv[] = {
      "/usr/bin/logbook.sh", nullptr
    };

    RunProcessDialog(UIGlobals::GetMainWindow(),
                     UIGlobals::GetDialogLook(),
                     "Logbook", argv);
  });

  AddButton("Files", [this](){
    CancelTimer();

    TWidgetDialog<FileMenuWidget>
      sub_dialog(WidgetDialog::Full{}, dialog.GetMainWindow(),
                 GetLook(), "OpenVario Files");
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
  main_style.InitialOrientation(Display::DetectInitialOrientation());

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

int main()
{
  /*the x-menu is waiting a second to solve timing problem with display rotation */
  std::chrono::high_resolution_clock hrc;
  auto start = hrc.now();
  while(std::chrono::duration_cast<std::chrono::milliseconds>(hrc.now() - start).count() < 1000)
  {
    //I'm just waiting ;-)
  }

  int action = Main();

  switch (action) {
  case LAUNCH_SHELL:
    execl("/bin/sh", "sh", "-c", "clear; bash -il", nullptr);
    perror("Failed to launch shell");
    return EXIT_FAILURE;
  }

  return action;
}
