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
#include "Screen/SingleWindow.hpp"
#include "Screen/TerminalWindow.hpp"
#include "Screen/Layout.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Device/Descriptor.hpp"
#include "Util/Macros.hpp"
#include "Util/FifoBuffer.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Thread/DelayedNotify.hpp"

static DeviceDescriptor *device;
static WndForm *dialog;
static TerminalWindow *terminal;
static bool paused;

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

static PortTerminalBridge *bridge;

static void
OnClearClicked(gcc_unused WndButton &button)
{
  terminal->Clear();
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrOK);
}

static void
OnReconnectClicked(gcc_unused WndButton &button)
{
  if (device->IsOccupied()) {
    ShowMessageBox(_("Device is occupied"), _("Manage"), MB_OK | MB_ICONERROR);
    return;
  }

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static MessageOperationEnvironment env;
  device->Reopen(env);
}

static void
OnPauseClicked(WndButton &button)
{
  paused = !paused;

  if (paused) {
    button.SetCaption(_("Resume"));
    device->SetMonitor(NULL);
  } else {
    button.SetCaption(_("Pause"));
    device->SetMonitor(bridge);
  }
}

void
ShowPortMonitor(SingleWindow &parent, const DialogLook &dialog_look,
                const TerminalLook &terminal_look,
                DeviceDescriptor &_device)
{
  device = &_device;

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.Hide();
  dialog_style.ControlParent();

  TCHAR buffer[64];
  StaticString<128> caption;
  caption.Format(_T("%s: %s"), _("Port monitor"),
                 device->GetConfig().GetPortName(buffer, ARRAY_SIZE(buffer)));

  dialog = new WndForm(parent, dialog_look, parent.GetClientRect(),
                       caption, dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  ButtonPanel buttons(client_area, dialog_look);
  buttons.Add(_("Close"), OnCloseClicked);
  buttons.Add(_("Clear"), OnClearClicked);
  buttons.Add(_("Reconnect"), OnReconnectClicked);
  buttons.Add(_("Pause"), OnPauseClicked);

  const PixelRect rc = buttons.UpdateLayout();

  /* create the terminal */

  terminal = new TerminalWindow(terminal_look);
  terminal->set(dialog->GetClientAreaWindow(), rc.left, rc.top,
                rc.right - rc.left, rc.bottom - rc.top);

  bridge = new PortTerminalBridge(*terminal);
  device->SetMonitor(bridge);
  paused = false;

  /* run it */

  dialog->ShowModal();

  device->SetMonitor(NULL);
  delete bridge;
  delete terminal;
  delete dialog;
}
