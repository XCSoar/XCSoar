/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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
#include "Form/ActionListener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "System.hpp"

class SystemWidget final
  : public RowFormWidget, ActionListener {
  enum Buttons {
    REBOOT,
    SWITCH_KERNEL,
  };

public:
  SystemWidget(const DialogLook &look):RowFormWidget(look) {}

private:
  void SwitchKernel();

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
SystemWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  AddButton("Reboot", *this, REBOOT);
  AddButton(IsKoboOTGKernel() ? "Disable USB-OTG" : "Enable USB-OTG",
            *this, SWITCH_KERNEL);
}

inline void
SystemWidget::SwitchKernel()
{
#ifdef KOBO
  const char *otg_kernel_image = "/opt/xcsoar/lib/kernel/uImage.otg";
  const char *kobo_kernel_image = "/opt/xcsoar/lib/kernel/uImage.kobo";

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

void
SystemWidget::OnAction(int id)
{
  switch (id) {
  case REBOOT:
    KoboReboot();
    break;

  case SWITCH_KERNEL:
    SwitchKernel();
    break;
  }
}

void
ShowSystemDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  SystemWidget widget(look);
  WidgetDialog dialog(look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), "System", &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
