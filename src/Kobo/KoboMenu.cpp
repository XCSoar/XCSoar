/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Look/DialogLook.hpp"
#include "Screen/Init.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "../test/src/Fonts.hpp"
#include "UIGlobals.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/ActionListener.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/TerminalWindow.hpp"
#include "Look/TerminalLook.hpp"
#include "Event/DelayedNotify.hpp"
#include "Thread/Mutex.hpp"
#include "Util/Macros.hpp"
#include "Util/FifoBuffer.hpp"
#include "IO/DataHandler.hpp"
#include "System.hpp"

#include <algorithm>
#include <stdio.h>

enum Buttons {
  LAUNCH_XCSOAR = 100,
  LAUNCH_NICKEL,
  WIFI,
  PAUSE,
  CLEAR,
  REBOOT,
  POWEROFF
};

/**
 * A bridge between DataHandler and TerminalWindow: copy all data
 * received from the Log to the TerminalWindow.
 */
class LogTerminalBridge
  : public DataHandler,
    private DelayedNotify {
  TerminalWindow &terminal;
  Mutex mutex;
  FifoBuffer<char, 1024> buffer;

public:
  LogTerminalBridge(TerminalWindow &_terminal)
    :DelayedNotify(100), terminal(_terminal) {}

  virtual void DataReceived(const void *data, size_t length) {
    mutex.Lock();
    auto range = buffer.Write();
    if (range.length < length)
      length = range.length;
    memcpy(range.data, data, length);
    buffer.Append(length);
    mutex.Unlock();
    SendNotification();
  }

private:
  virtual void OnNotification() {
    while (true) {
      char data[64];
      size_t length;

      {
        ScopeLock protect(mutex);
        auto range = buffer.Read();
        if (range.IsEmpty())
          break;

        length = std::min(ARRAY_SIZE(data), size_t(range.length));
        memcpy(data, range.data, length);
        buffer.Consume(length);
      }

      terminal.Write(data, length);
    }
  }
};

class LogMonitorGlue : public ActionListener {
  TerminalWindow terminal;
  LogTerminalBridge bridge;

  WndButton *pause_button;
  bool paused;

  WndButton *wifi_button;
  bool wifi;
  WndForm &dialog;

public:
  LogMonitorGlue(const TerminalLook &look, WndForm &_dialog)
    :terminal(look), bridge(terminal), paused(false), wifi(false), dialog(_dialog) {}

  ~LogMonitorGlue() {
    // device.SetMonitor(nullptr);
  }

  void CreateButtons(ButtonPanel &buttons);

  void CreateTerminal(ContainerWindow &parent, const PixelRect &rc) {
    terminal.Create(parent, rc);
    // device.SetMonitor(&bridge);
  }

  void Clear() {
    terminal.Clear();
  }

  void TogglePause();
  void ToggleWifi();

  virtual void OnAction(int id) override {
    switch (id) {
    case CLEAR:
      Clear();
      break;
    case PAUSE:
      TogglePause();
      break;
    case WIFI:
      ToggleWifi();
      break;
    default:
      break;
    }
  }
};

void
LogMonitorGlue::CreateButtons(ButtonPanel &buttons)
{
  buttons.Add(("XCSoar"), dialog, LAUNCH_XCSOAR);
  buttons.Add(("Nickel"), dialog, LAUNCH_NICKEL);
  buttons.Add(("Clear"), *this, CLEAR);
  pause_button = buttons.Add(("Pause"), *this, PAUSE);
  wifi_button = buttons.Add(("Wifi ON"), *this, WIFI);
  buttons.Add(("Reboot"), dialog, REBOOT);
  buttons.Add(("Poweroff"), dialog, POWEROFF);
}

void
LogMonitorGlue::TogglePause()
{
  paused = !paused;

  if (paused) {
    pause_button->SetCaption(("Resume"));
    //    device.SetMonitor(nullptr);
  } else {
    pause_button->SetCaption(("Pause"));
    //    device.SetMonitor(&bridge);
  }
}

void
LogMonitorGlue::ToggleWifi()
{
  wifi = !wifi;

  if (wifi) {
    wifi_button->SetCaption(("Wifi OFF"));
    KoboWifiOn();
    //    device.SetMonitor(nullptr);
  } else {
    wifi_button->SetCaption(("Wifi ON"));
    //    device.SetMonitor(&bridge);
    KoboWifiOff();
  }
}

static int
Main(SingleWindow &main_window, const DialogLook &dialog_look,
     const TerminalLook &terminal_look)
{
  WndForm dialog(dialog_look);
#ifdef USE_LINUX_INPUT
  dialog.SetKeyDownFunction([&dialog](unsigned key_code){
      switch (key_code) {
      case KEY_POWER:
        dialog.SetModalResult(POWEROFF);
        return true;

      default:
        return false;
      }
    });
#endif

  dialog.Create(main_window, _T("Kobo XCSoar Launcher"));
  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  LogMonitorGlue glue(terminal_look, dialog);

  ButtonPanel buttons(client_area, dialog_look);

  glue.CreateButtons(buttons);
  glue.CreateTerminal(client_area, buttons.UpdateLayout());

  // must be down at start
  KoboWifiOff();

  return dialog.ShowModal();

  // /proc/kmsg
}

static int
Main()
{
  ScreenGlobalInit screen_init;
  Layout::Initialize({600, 800});
  InitialiseFonts();

  DialogLook dialog_look;
  dialog_look.Initialise(bold_font, normal_font, small_font,
                         bold_font, bold_font, bold_font);

  TerminalLook terminal_look;
  terminal_look.Initialise(monospace_font);

  SingleWindow main_window;
  main_window.Create(_T("Test"), {600, 800});
  main_window.Show();

  int action = Main(main_window, dialog_look, terminal_look);

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
      printf("launch nickel\n");
      fflush(stdout);

      KoboExecNickel();
      return EXIT_FAILURE;

    case LAUNCH_XCSOAR:
      printf("launch xcsoar\n");
      fflush(stdout);

      KoboRunXCSoar();
      /* return to menu after XCSoar quits */
      break;

    case REBOOT:
      KoboReboot();
      return EXIT_SUCCESS;

    case POWEROFF:
      KoboPowerOff();
      return EXIT_SUCCESS;

    default:
      return EXIT_SUCCESS;
    }
  }
}
