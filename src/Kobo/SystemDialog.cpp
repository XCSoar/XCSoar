/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#ifdef KOBO
#include "Model.hpp"
#endif

#include "system/FileUtil.hpp"

#include <sys/stat.h>

class SystemWidget final
  : public RowFormWidget {

  enum Buttons {
    REBOOT,
    SWITCH_OTG_MODE,
    USB_STORAGE,
    INCREASE_BACKLIGHT_BRIGHTNESS,
    DECREASE_BACKLIGHT_BRIGHTNESS,
  };

  Button *switch_otg_mode;
  Button *usb_storage;
  Button *increase_backlight_brightness;
  Button *decrease_backlight_brightness;

public:
  SystemWidget(const DialogLook &look):RowFormWidget(look) {}

private:
  void SwitchOTGMode();
  void SwitchKernel();
  void ExportUSBStorage();
  void IncreaseBacklightBrightness();
  void DecreaseBacklightBrightness();
  void UpdateBacklightBrightnessButtons(int percent);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
SystemWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddButton("Reboot", [](){ KoboReboot(); });
  switch_otg_mode = AddButton(IsKoboOTGHostMode() ? "Disable USB-OTG" : "Enable USB-OTG",
            [this](){ SwitchOTGMode(); });
  usb_storage = AddButton("Export USB storage", [this](){ ExportUSBStorage(); });
  SetRowEnabled(USB_STORAGE, !IsKoboOTGHostMode());

  if(KoboCanChangeBacklightBrightness()) {
    increase_backlight_brightness = AddButton("Increase Backlight Brightness", [this]() { IncreaseBacklightBrightness(); });
    decrease_backlight_brightness = AddButton("Decrease Backlight Brightness", [this]() { DecreaseBacklightBrightness(); });
    int current_brightness = KoboGetBacklightBrightness();
    UpdateBacklightBrightnessButtons(current_brightness);
  } else {
    AddDummy();
    AddDummy();
  }
}

inline void
SystemWidget::SwitchOTGMode()
{
#ifdef KOBO
  KoboModel kobo_model = DetectKoboModel();
  if (kobo_model == KoboModel::CLARA_HD || kobo_model == KoboModel::CLARA_2E
      || kobo_model == KoboModel::LIBRA2 || kobo_model == KoboModel::LIBRA_H2O) {
    bool success;
    if (IsKoboOTGHostMode()) {
      success = File::WriteExisting(Path("/sys/kernel/debug/ci_hdrc.0/role"),
                                    "gadget");
      if (success && !IsKoboOTGHostMode()) {
        File::Delete(Path("/mnt/onboard/XCSoarData/kobo/OTG_Host_Active"));
        switch_otg_mode->SetCaption("Enable USB-OTG");
        usb_storage->SetEnabled(true);
      } else {
        ShowMessageBox(_T("Failed to switch OTG mode."), _("Error"), MB_OK);
      }
    } else {
      success = File::WriteExisting(Path("/sys/kernel/debug/ci_hdrc.0/role"),
                                    "host");
      if (success && IsKoboOTGHostMode()) {
        mkdir("/mnt/onboard/XCSoarData", 0777);
        mkdir("/mnt/onboard/XCSoarData/kobo", 0777);
        File::CreateExclusive(Path("/mnt/onboard/XCSoarData/kobo/OTG_Host_Active"));
        switch_otg_mode->SetCaption("Disable USB-OTG");
        usb_storage->SetEnabled(false);
      } else {
        ShowMessageBox(_T("Failed to switch OTG mode."), _("Error"), MB_OK);
      }
    }
  } else {
    SwitchKernel();
  }
#endif
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

  const char *kernel_image = IsKoboOTGHostMode()
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

inline void
SystemWidget::IncreaseBacklightBrightness()
{
  int current_brightness = KoboGetBacklightBrightness();
  KoboSetBacklightBrightness(current_brightness + 20);
  UpdateBacklightBrightnessButtons(current_brightness + 20);
}

inline void
SystemWidget::DecreaseBacklightBrightness()
{
  int current_brightness = KoboGetBacklightBrightness();
  KoboSetBacklightBrightness(current_brightness - 20);
  UpdateBacklightBrightnessButtons(current_brightness - 20);
}

inline void
SystemWidget::UpdateBacklightBrightnessButtons(int percent)
{
  if(decrease_backlight_brightness != nullptr) {
    decrease_backlight_brightness->SetEnabled(percent != 0);
  }
  if(increase_backlight_brightness != nullptr) {
    increase_backlight_brightness->SetEnabled(percent < 100);
  }
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
