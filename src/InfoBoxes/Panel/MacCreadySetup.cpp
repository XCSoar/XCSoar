// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  PixelSize GetMinimumSize() const noexcept override {
    return PixelSize{Layout::Scale(80u), Layout::Scale(30u)};
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override;
};

void
MacCreadySetupPanel::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
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
MacCreadySetupPanel::Show(const PixelRect &rc) noexcept
{
  auto &auto_mc = (CheckBoxControl &)GetWindow();
  auto_mc.SetState(CommonInterface::GetComputerSettings().task.auto_mc);
  WindowWidget::Show(rc);
}

std::unique_ptr<Widget>
LoadMacCreadySetupPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<MacCreadySetupPanel>();
}
