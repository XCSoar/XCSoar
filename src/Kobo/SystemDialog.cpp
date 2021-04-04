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

#include "SystemDialog.hpp"
#include "Kernel.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "System.hpp"
#include "Model.hpp"

class SystemWidget final
  : public RowFormWidget {

  enum Buttons {
    REBOOT,
    SWITCH_KERNEL,
    USB_STORAGE,
  };

public:
  SystemWidget(const DialogLook &look):RowFormWidget(look) {}

private:
  void SwitchKernel();
  void ExportUSBStorage();

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
SystemWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  AddButton("Reboot", [](){ KoboReboot(); });
  AddButton(IsKoboOTGKernel() ? "Disable USB-OTG" : "Enable USB-OTG",
            [this](){ SwitchKernel(); });

  AddButton("Export USB storage", [this](){ ExportUSBStorage(); });
  SetRowEnabled(USB_STORAGE, !IsKoboOTGKernel());
}

inline void
SystemWidget::SwitchKernel()
{
#ifdef KOBO
  KoboModel model = DetectKoboModel();
  if (model != KoboModel::MINI &&
      model != KoboModel::GLO &&
      model != KoboModel::TOUCH2 &&
      model != KoboModel::GLO_HD &&
      model != KoboModel::AURA2 &&
      ShowMessageBox(_T("This feature was designed for the Kobo Mini, Touch 2.0, Glo HD and Aura 2, but this is not one.  Use at your own risk.  Continue?"),
                     _T("USB-OTG"), MB_YESNO) != IDYES)
    return;

  const char *otg_kernel_image, *kobo_kernel_image;
  switch (model)
  {
  case KoboModel::GLO_HD:
  case KoboModel::TOUCH2:
    otg_kernel_image = "/opt/xcsoar/lib/kernel/uImage.glohd.otg";
    kobo_kernel_image = "/opt/xcsoar/lib/kernel/uImage.glohd";
    break;

  case KoboModel::AURA2:
    otg_kernel_image = "/opt/xcsoar/lib/kernel/uImage.aura2.otg";
    kobo_kernel_image = "/opt/xcsoar/lib/kernel/uImage.aura2";
    break;

  default:
    otg_kernel_image = "/opt/xcsoar/lib/kernel/uImage.otg";
    kobo_kernel_image = "/opt/xcsoar/lib/kernel/uImage.kobo";
  }

  const char *kernel_image = IsKoboOTGKernel()
    ? kobo_kernel_image
    : otg_kernel_image;

  if (!KoboInstallKernel(kernel_image)) {
      ShowMessageBox(_T("Failed to activate kernel."), _("Error"), MB_OK);
      return;
  }

  KoboReboot();
#endif
}

inline void
SystemWidget::ExportUSBStorage()
{
  if (!KoboUmountData()) {
      ShowMessageBox(_T("Failed to unmount data partition."), _("Error"),
                     MB_OK);
      return;
  }

  if (!KoboExportUSBStorage()) {
      ShowMessageBox(_T("Failed to export data partition."), _("Error"),
                     MB_OK);
      KoboMountData();
      return;
  }

  ShowMessageBox(_T("Your PC has now access to the data partition until you close this dialog."),
                 _T("Export USB storage"),
                 MB_OK);

  KoboUnexportUSBStorage();
  KoboMountData();
}

void
ShowSystemDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<SystemWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), look, "System");
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(look);
  dialog.ShowModal();
}
