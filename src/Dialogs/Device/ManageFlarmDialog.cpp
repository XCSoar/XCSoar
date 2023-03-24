// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageFlarmDialog.hpp"
#include "FLARM/ConfigWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Version.hpp"

class ManageFLARMWidget final
  : public RowFormWidget {
  enum Controls {
    Setup,
    Reboot,
  };

  FlarmDevice &device;
  const FlarmVersion version;

public:
  ManageFLARMWidget(const DialogLook &look, FlarmDevice &_device,
                    const FlarmVersion &version)
    :RowFormWidget(look), device(_device), version(version) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
ManageFLARMWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  if (version.available) {
    StaticString<64> buffer;

    if (!version.hardware_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.hardware_version.c_str());
      AddReadOnly(_("Hardware version"), NULL, buffer.c_str());
    }

    if (!version.software_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.software_version.c_str());
      AddReadOnly(_("Firmware version"), NULL, buffer.c_str());
    }

    if (!version.obstacle_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.obstacle_version.c_str());
      AddReadOnly(_("Obstacle database"), NULL, buffer.c_str());
    }
  }

  AddButton(_("Setup"), [this](){
    FLARMConfigWidget widget(GetLook(), device);
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                        _T("FLARM"), widget);
  });

  AddButton(_("Reboot"), [this](){
    try {
      MessageOperationEnvironment env;
      device.Restart(env);
    } catch (OperationCancelled) {
    } catch (...) {
      ShowError(std::current_exception(), _("Error"));
    }
  });
}

void
ManageFlarmDialog(Device &device, const FlarmVersion &version)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _T("FLARM"),
                      new ManageFLARMWidget(UIGlobals::GetDialogLook(),
                                            (FlarmDevice &)device, version));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
