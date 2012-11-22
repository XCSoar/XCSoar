/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Form/WindowWidget.hpp"
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"

class WndButton;

class MacCreadySetupPanel : public WindowWidget,
                            private ActionListener {
public:
  WndButton &GetButton() {
    return *(WndButton *)GetWindow();
  }

  gcc_pure
  static const TCHAR *GetCaption() {
    return CommonInterface::GetComputerSettings().task.auto_mc
      ? _("MANUAL")
      : _("AUTO");
  }

  void UpdateCaption() {
    GetButton().SetCaption(GetCaption());
  }

  /* virtual methods from class Widget */
  virtual PixelSize GetMinimumSize() const gcc_override {
    return PixelSize{Layout::Scale(80u), Layout::Scale(30u)};
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override;

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) gcc_override;
};

void
MacCreadySetupPanel::OnAction(int id)
{
  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;
  task_behaviour.auto_mc = !task_behaviour.auto_mc;
  Profile::Set(ProfileKeys::AutoMc, task_behaviour.auto_mc);

  UpdateCaption();
}

void
MacCreadySetupPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  ButtonWindowStyle style;
  style.Hide();
  style.TabStop();

  SetWindow(new WndButton(parent, UIGlobals::GetDialogLook(), GetCaption(), rc,
                          style, *this, 1));
}

Widget *
LoadMacCreadySetupPanel(unsigned id)
{
  return new MacCreadySetupPanel();
}
