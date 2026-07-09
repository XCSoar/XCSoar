// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMetConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/Features.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

enum ControlIndex {
#ifdef HAVE_PCMET
  PCMET_USER,
  PCMET_PASSWORD,
#endif
};

class PCMetConfigPanel final : public RowFormWidget {
public:
  PCMetConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
PCMetConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
#ifdef HAVE_PCMET
  const auto &settings = CommonInterface::GetComputerSettings().weather;
#endif

  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_PCMET
  AddText(_("pc_met Username"), "",
          settings.pcmet.www_credentials.username);
  AddPassword(_("pc_met Password"), "",
              settings.pcmet.www_credentials.password);
#endif
}

bool
PCMetConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

#ifdef HAVE_PCMET
  auto &settings = CommonInterface::SetComputerSettings().weather;

  changed |= SaveValue(PCMET_USER, ProfileKeys::PCMetUsername,
                       settings.pcmet.www_credentials.username);

  changed |= SaveValue(PCMET_PASSWORD, ProfileKeys::PCMetPassword,
                       settings.pcmet.www_credentials.password);
#endif

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreatePCMetConfigPanel()
{
  return std::make_unique<PCMetConfigPanel>();
}
