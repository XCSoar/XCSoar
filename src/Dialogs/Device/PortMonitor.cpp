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

#include "PortMonitor.hpp"
#include "Dialogs/Message.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/TerminalWindow.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/ActionListener.hpp"
#include "Device/Descriptor.hpp"
#include "Util/Macros.hpp"
#include "Util/FifoBuffer.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Event/DelayedNotify.hpp"
#include "Thread/Mutex.hpp"

enum Buttons {
  CLEAR = 100,
  RECONNECT,
  PAUSE,
};

/**
 * A bridge between DataHandler and TerminalWindow: copy all data
 * received from the Port to the TerminalWindow.
 */
class PortTerminalBridge : public DataHandler, private DelayedNotify {
  TerminalWindow &terminal;
  Mutex mutex;
  FifoBuffer<char, 1024> buffer;

public:
  PortTerminalBridge(TerminalWindow &_terminal)
    :DelayedNotify(100), terminal(_terminal) {}
  virtual ~PortTerminalBridge() {}

  virtual void DataReceived(const void *data, size_t length) {
    mutex.Lock();
    auto range = buffer.Write();
    if (range.size < length)
      length = range.size;
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

        length = std::min(ARRAY_SIZE(data), size_t(range.size));
        memcpy(data, range.data, length);
        buffer.Consume(length);
      }

      terminal.Write(data, length);
    }
  }
};

class PortMonitorGlue : public ActionListener {
  DeviceDescriptor &device;
  TerminalWindow terminal;
  PortTerminalBridge bridge;

  WndButton *pause_button;
  bool paused;

public:
  PortMonitorGlue(DeviceDescriptor &_device, const TerminalLook &look)
    :device(_device), terminal(look), bridge(terminal), paused(false) {}

  ~PortMonitorGlue() {
    device.SetMonitor(nullptr);
  }

  void CreateButtons(ButtonPanel &buttons);

  void CreateTerminal(ContainerWindow &parent, const PixelRect &rc) {
    terminal.Create(parent, rc);
    device.SetMonitor(&bridge);
  }

  void Clear() {
    terminal.Clear();
  }

  void Reconnect();
  void TogglePause();

  virtual void OnAction(int id) override {
    switch (id) {
    case CLEAR:
      Clear();
      break;

    case RECONNECT:
      Reconnect();
      break;

    case PAUSE:
      TogglePause();
      break;
    }
  }
};

void
PortMonitorGlue::CreateButtons(ButtonPanel &buttons)
{
  buttons.Add(_("Clear"), *this, CLEAR);
  buttons.Add(_("Reconnect"), *this, RECONNECT);
  pause_button = buttons.Add(_("Pause"), *this, PAUSE);
}

void
PortMonitorGlue::Reconnect()
{
  if (device.IsOccupied()) {
    ShowMessageBox(_("Device is occupied"), _("Manage"), MB_OK | MB_ICONERROR);
    return;
  }

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static MessageOperationEnvironment env;
  device.Reopen(env);
}

void
PortMonitorGlue::TogglePause()
{
  paused = !paused;

  if (paused) {
    pause_button->SetCaption(_("Resume"));
    device.SetMonitor(nullptr);
  } else {
    pause_button->SetCaption(_("Pause"));
    device.SetMonitor(&bridge);
  }
}

void
ShowPortMonitor(SingleWindow &parent, const DialogLook &dialog_look,
                const TerminalLook &terminal_look,
                DeviceDescriptor &device)
{
  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.Hide();
  dialog_style.ControlParent();

  TCHAR buffer[64];
  StaticString<128> caption;
  caption.Format(_T("%s: %s"), _("Port monitor"),
                 device.GetConfig().GetPortName(buffer, ARRAY_SIZE(buffer)));

  WndForm dialog(dialog_look);
  dialog.Create(parent, caption, dialog_style);

  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  PortMonitorGlue glue(device, terminal_look);

  ButtonPanel buttons(client_area, dialog_look.button);
  buttons.Add(_("Close"), dialog, mrOK);
  glue.CreateButtons(buttons);
  glue.CreateTerminal(client_area, buttons.UpdateLayout());

  /* run it */

  dialog.ShowModal();
}
