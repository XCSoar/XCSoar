// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
    INCREASE_BACKLIGHT_COLOUR_TEMPERATURE,
    DECREASE_BACKLIGHT_COLOUR_TEMPERATURE,
  };

  Button *switch_otg_mode;
  Button *usb_storage;
  Button *increase_backlight_brightness;
  Button *decrease_backlight_brightness;
  Button *increase_backlight_colour_temperature;
  Button *decrease_backlight_colour_temperature;

public:
  SystemWidget(const DialogLook &look):RowFormWidget(look) {}

private:
  void SwitchOTGMode();
  void SwitchKernel();
  void ExportUSBStorage();
  void IncreaseBacklightBrightness();
  void DecreaseBacklightBrightness();
  void UpdateBacklightBrightnessButtons(int percent);
  void IncreaseBacklightColourTemperature() noexcept;
  void DecreaseBacklightColourTemperature() noexcept;
  void UpdateBacklightColourButtons(unsigned int colour);

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
    if(KoboCanChangeBacklightColour()) {
      increase_backlight_colour_temperature = AddButton("Increase Backlight Colour Temperature",
                                                        [this]() { IncreaseBacklightColourTemperature(); });
      decrease_backlight_colour_temperature = AddButton("Decrease Backlight Colour Temperature",
                                                        [this]() { DecreaseBacklightColourTemperature(); });
      unsigned int current_colour = 0;
      KoboGetBacklightColour(current_colour);
      UpdateBacklightColourButtons(current_colour);
    } else {
      AddDummy();
      AddDummy();
    }
  } else {
    AddDummy();
    AddDummy();
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
        ShowMessageBox("Failed to switch OTG mode.", _("Error"), MB_OK);
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
        ShowMessageBox("Failed to switch OTG mode.", _("Error"), MB_OK);
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
      ShowMessageBox("This feature was designed for the Kobo Mini, Touch 2.0, Glo HD and Aura 2, but this is not one.  Use at your own risk.  Continue?",
                     "USB-OTG", MB_YESNO) != IDYES)
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
      ShowMessageBox("Failed to activate kernel.", _("Error"), MB_OK);
      return;
  }

  KoboReboot();
#endif
}

inline void
SystemWidget::ExportUSBStorage()
{
  if (!KoboUmountData()) {
      ShowMessageBox("Failed to unmount data partition.", _("Error"),
                     MB_OK);
      return;
  }

  if (!KoboExportUSBStorage()) {
      ShowMessageBox("Failed to export data partition.", _("Error"),
                     MB_OK);
      KoboMountData();
      return;
  }

  ShowMessageBox("Your PC has now access to the data partition until you close this dialog.",
                 "Export USB storage",
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

inline void
SystemWidget::IncreaseBacklightColourTemperature() noexcept
{
  unsigned int current_colour;
  if (KoboGetBacklightColour(current_colour)) {
    KoboSetBacklightColour(current_colour + 2);
    UpdateBacklightColourButtons(current_colour + 2);
  }
}

inline void
SystemWidget::DecreaseBacklightColourTemperature() noexcept
{
  unsigned int current_colour;
  if (KoboGetBacklightColour(current_colour)) {
    if (current_colour < 2) current_colour = 2;
    KoboSetBacklightColour(current_colour - 2);
    UpdateBacklightColourButtons(current_colour - 2);
  }
}

inline void
SystemWidget::UpdateBacklightColourButtons(unsigned int colour)
{
  if(decrease_backlight_colour_temperature != nullptr) {
    decrease_backlight_colour_temperature->SetEnabled(colour > 0);
  }
  if(increase_backlight_colour_temperature != nullptr) {
    increase_backlight_colour_temperature->SetEnabled(colour < 10);
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
