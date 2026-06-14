// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMetConfigPanel.hpp"

#include "Weather/Features.hpp"

#ifdef HAVE_PCMET

#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  PCMET_USER,
  PCMET_PASSWORD,
};

class PCMetConfigPanel final : public RowFormWidget {
public:
  PCMetConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
PCMetConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  RowFormWidget::Prepare(parent, rc);

  AddText(_("Flugwetter username"),
          _("Credentials for www.flugwetter.de (Deutscher Wetterdienst)."),
          settings.pcmet.www_credentials.username);
  AddPassword(_("Flugwetter password"), "",
              settings.pcmet.www_credentials.password);
}

bool
PCMetConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;
  auto &settings = CommonInterface::SetComputerSettings().weather;

  changed |= SaveValue(PCMET_USER, ProfileKeys::PCMetUsername,
                       settings.pcmet.www_credentials.username);

  changed |= SaveValue(PCMET_PASSWORD, ProfileKeys::PCMetPassword,
                       settings.pcmet.www_credentials.password);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreatePCMetConfigPanel()
{
  return std::make_unique<PCMetConfigPanel>();
}

#endif /* HAVE_PCMET */
