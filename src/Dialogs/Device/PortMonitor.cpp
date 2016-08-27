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

#include "PortMonitor.hpp"
#include "Dialogs/Message.hpp"
#include "Look/Look.hpp"
#include "Screen/TerminalWindow.hpp"
#include "Widget/WindowWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/ActionListener.hpp"
#include "Device/Descriptor.hpp"
#include "Util/Macros.hpp"
#include "Util/StaticFifoBuffer.hxx"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Event/DelayedNotify.hpp"
#include "Thread/Mutex.hpp"
#include "UIGlobals.hpp"

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
  StaticFifoBuffer<char, 1024> buffer;

public:
  PortTerminalBridge(TerminalWindow &_terminal)
    :DelayedNotify(100), terminal(_terminal) {}
  virtual ~PortTerminalBridge() {}

  virtual void DataReceived(const void *data, size_t length) {
    mutex.Lock();
    buffer.Shift();
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

class PortMonitorWidget final : public WindowWidget, public ActionListener {
  DeviceDescriptor &device;
  TerminalWindow terminal;
  PortTerminalBridge bridge;

  Button *pause_button;
  bool paused;

public:
  PortMonitorWidget(DeviceDescriptor &_device, const TerminalLook &look)
    :device(_device), terminal(look), bridge(terminal), paused(false) {}

  void CreateButtons(WidgetDialog &dialog);

  void Clear() {
    terminal.Clear();
  }

  void Reconnect();
  void TogglePause();

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent, const PixelRect &rc) override {
    WindowStyle style;
    style.Hide();
    terminal.Create(parent, rc, style);
    SetWindow(&terminal);
    device.SetMonitor(&bridge);
  }

  void Unprepare() override {
    device.SetMonitor(nullptr);
  }

  /* virtual methods from class ActionListener */
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
PortMonitorWidget::CreateButtons(WidgetDialog &dialog)
{
  dialog.AddButton(_("Clear"), *this, CLEAR);
  dialog.AddButton(_("Reconnect"), *this, RECONNECT);
  pause_button = dialog.AddButton(_("Pause"), *this, PAUSE);
}

void
PortMonitorWidget::Reconnect()
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
PortMonitorWidget::TogglePause()
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
ShowPortMonitor(DeviceDescriptor &device)
{
  const Look &look = UIGlobals::GetLook();

  TCHAR buffer[64];
  StaticString<128> caption;
  caption.Format(_T("%s: %s"), _("Port monitor"),
                 device.GetConfig().GetPortName(buffer, ARRAY_SIZE(buffer)));

  PortMonitorWidget widget(device, look.terminal);

  WidgetDialog dialog(look.dialog);
  dialog.CreateFull(UIGlobals::GetMainWindow(), caption, &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons(dialog);

  dialog.ShowModal();
  dialog.StealWidget();
}
