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

#include "MacCreadySetup.hpp"
#include "Screen/Layout.hpp"
#include "Widget/WindowWidget.hpp"
#include "Form/CheckBox.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"

class MacCreadySetupPanel : public WindowWidget {
public:
  /* virtual methods from class Widget */
  virtual PixelSize GetMinimumSize() const override {
    return PixelSize{Layout::Scale(80u), Layout::Scale(30u)};
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

  virtual void Show(const PixelRect &rc) override;
};

void
MacCreadySetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle style;
  style.Hide();
  style.TabStop();

  auto w = std::make_unique<CheckBoxControl>();
  w->Create(parent, UIGlobals::GetDialogLook(), _("Auto"), rc, style, [](bool value){
    TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;
    task_behaviour.auto_mc = value;
    Profile::Set(ProfileKeys::AutoMc, task_behaviour.auto_mc);
  });
  SetWindow(std::move(w));
}

void
MacCreadySetupPanel::Show(const PixelRect &rc)
{
  auto &auto_mc = (CheckBoxControl &)GetWindow();
  auto_mc.SetState(CommonInterface::GetComputerSettings().task.auto_mc);
  WindowWidget::Show(rc);
}

std::unique_ptr<Widget>
LoadMacCreadySetupPanel(unsigned id)
{
  return std::make_unique<MacCreadySetupPanel>();
}
