// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PortMonitor.hpp"
#include "Dialogs/Message.hpp"
#include "Look/Look.hpp"
#include "ui/control/TerminalWindow.hpp"
#include "Widget/WindowWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Device/Descriptor.hpp"
#include "util/StaticFifoBuffer.hxx"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "ui/event/DelayedNotify.hpp"
#include "thread/Mutex.hxx"
#include "UIGlobals.hpp"

/**
 * A bridge between DataHandler and TerminalWindow: copy all data
 * received from the Port to the TerminalWindow.
 */
class PortTerminalBridge final : public DataHandler {
  TerminalWindow &terminal;
  Mutex mutex;
  StaticFifoBuffer<std::byte, 1024> buffer;

  UI::DelayedNotify notify{
    std::chrono::milliseconds(100),
    [this]{ OnNotification(); },
  };

public:
  PortTerminalBridge(TerminalWindow &_terminal)
    :terminal(_terminal) {}
  virtual ~PortTerminalBridge() {}

  bool DataReceived(std::span<const std::byte> s) noexcept {
    {
      const std::lock_guard lock{mutex};
      buffer.Shift();
      auto range = buffer.Write();
      const std::size_t nbytes = std::min(s.size(), range.size());
      std::copy_n(s.begin(), nbytes, range.begin());
      buffer.Append(nbytes);
    }

    notify.SendNotification();
    return true;
  }

private:
  void OnNotification() noexcept {
    while (true) {
      std::array<std::byte, 64> data;
      size_t length;

      {
        const std::lock_guard lock{mutex};
        auto range = buffer.Read();
        if (range.empty())
          break;

        length = std::min(data.size(), range.size());
        std::copy_n(range.begin(), length, data.begin());
        buffer.Consume(length);
      }

      terminal.Write((const char *)data.data(), length);
    }
  }
};

class PortMonitorWidget final : public WindowWidget {
  DeviceDescriptor &device;
  const TerminalLook &look;
  std::unique_ptr<PortTerminalBridge> bridge;

  Button *pause_button;
  bool paused;

public:
  PortMonitorWidget(DeviceDescriptor &_device,
                    const TerminalLook &_look) noexcept
    :device(_device), look(_look), paused(false) {}

  void CreateButtons(WidgetDialog &dialog);

  void Clear() {
    auto &terminal = (TerminalWindow &)GetWindow();
    terminal.Clear();
  }

  void Reconnect();
  void TogglePause();

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique<TerminalWindow>(look);
    w->Create(parent, rc, style);

    bridge = std::make_unique<PortTerminalBridge>(*w);
    device.SetMonitor(bridge.get());

    SetWindow(std::move(w));
  }

  void Unprepare() noexcept override {
    device.SetMonitor(nullptr);
  }
};

void
PortMonitorWidget::CreateButtons(WidgetDialog &dialog)
{
  dialog.AddButton(_("Clear"), [this](){ Clear(); });
  dialog.AddButton(_("Reconnect"), [this](){ Reconnect(); });
  pause_button = dialog.AddButton(_("Pause"), [this](){ TogglePause(); });
}

void
PortMonitorWidget::Reconnect()
{
  if (device.IsBorrowed()) {
    ShowMessageBox(_("Device is occupied"), _("Reconnect"),
                   MB_OK | MB_ICONERROR);
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
    device.SetMonitor(bridge.get());
  }
}

void
ShowPortMonitor(DeviceDescriptor &device)
{
  const Look &look = UIGlobals::GetLook();

  std::array<TCHAR, 64> buffer;
  StaticString<128> caption;
  caption.Format(_T("%s: %s"), _("Port monitor"),
                 device.GetConfig().GetPortName(buffer.data(), buffer.size()));

  TWidgetDialog<PortMonitorWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look.dialog, caption);
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(device, look.terminal);
  dialog.GetWidget().CreateButtons(dialog);

  dialog.ShowModal();
}
