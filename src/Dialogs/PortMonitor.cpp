/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/SingleWindow.hpp"
#include "Screen/TerminalWindow.hpp"
#include "Screen/Layout.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Device/Descriptor.hpp"
#include "Util/Macros.hpp"
#include "Util/FifoBuffer.hpp"
#include "Language/Language.hpp"
#include "Operation.hpp"
#include "Thread/Notify.hpp"

static DeviceDescriptor *device;
static WndForm *dialog;
static TerminalWindow *terminal;

/**
 * A bridge between Port::Handler and TerminalWindow: copy all data
 * received from the Port to the TerminalWindow.
 */
class PortTerminalBridge : public Port::Handler, Notify {
  TerminalWindow &terminal;
  Mutex mutex;
  FifoBuffer<char, 1024> buffer;

public:
  PortTerminalBridge(TerminalWindow &_terminal):terminal(_terminal) {}
  virtual ~PortTerminalBridge() {}

  virtual void DataReceived(const void *data, size_t length) {
    mutex.Lock();
    auto range = buffer.Write();
    if (range.second < length)
      length = range.second;
    memcpy(range.first, data, length);
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
        if (range.second == 0)
          break;

        length = std::min(ARRAY_SIZE(data), size_t(range.second));
        memcpy(data, range.first, length);
        buffer.Consume(length);
      }

      terminal.Write(data, length);
    }
  }
};

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
  MessageOperationEnvironment env;
  device->Reopen(env);
}

void
ShowPortMonitor(SingleWindow &parent, const DialogLook &dialog_look,
                const TerminalLook &terminal_look,
                DeviceDescriptor &_device)
{
  device = &_device;

  UPixelScalar margin = Layout::Scale(2);

  /* create the dialog */

  WindowStyle dialog_style;
  dialog_style.hide();
  dialog_style.control_parent();

  PixelSize size = parent.get_size();

  TCHAR buffer[64], caption[128];
  _sntprintf(caption, ARRAY_SIZE(caption), _T("%s: %s"),
             _("Port monitor"),
             device->GetConfig().GetPortName(buffer, ARRAY_SIZE(buffer)));

  dialog = new WndForm(parent, dialog_look, 0, 0, size.cx, size.cy,
                       caption, dialog_style);

  ContainerWindow &client_area = dialog->GetClientAreaWindow();

  ButtonPanel buttons(client_area, dialog_look);
  buttons.Add(_("Close"), OnCloseClicked);
  buttons.Add(_("Clear"), OnClearClicked);
  buttons.Add(_("Reconnect"), OnReconnectClicked);

  const PixelRect rc = buttons.GetRemainingRect();

  /* create the terminal */

  terminal = new TerminalWindow(terminal_look);
  terminal->set(dialog->GetClientAreaWindow(),
                rc.left + margin, rc.top + margin,
                rc.right - rc.left - 2 * margin,
                rc.bottom - rc.top - 2 * margin);

  PortTerminalBridge *bridge = new PortTerminalBridge(*terminal);
  device->SetMonitor(bridge);

  /* run it */

  dialog->ShowModal();

  device->SetMonitor(NULL);
  delete bridge;
  delete terminal;
  delete dialog;
}
