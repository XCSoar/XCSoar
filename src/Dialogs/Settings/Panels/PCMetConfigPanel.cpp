// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PCMetConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/Features.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

/** The account of flugwetter.de. */
class PCMetConfigPanel final : public ConfigListPanel {
#ifdef HAVE_PCMET
  StaticString<64> username, password;
#endif

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Leave() noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
PCMetConfigPanel::LoadSettings() noexcept
{
#ifdef HAVE_PCMET
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  username = settings.pcmet.www_credentials.username;
  password = settings.pcmet.www_credentials.password;
#endif
}

void
PCMetConfigPanel::Fill() noexcept
{
#ifdef HAVE_PCMET
  AddGroup();

  AddTextItem(_("pc_met Username"), nullptr, username);
  AddTextItem(_("pc_met Password"), nullptr, password, true);
#endif
}

bool
PCMetConfigPanel::Leave() noexcept
{
  /* the list of the Weather group says whether the account is set
     up: store it as soon as the page is left, not only when the
     configuration is closed */
  bool changed = false;
  Save(changed);
  if (changed)
    Profile::Save();

  return ConfigListPanel::Leave();
}

bool
PCMetConfigPanel::Save(bool &_changed) noexcept
{
#ifdef HAVE_PCMET
  auto &settings = CommonInterface::SetComputerSettings().weather;

  const bool username_changed =
    Profile::Update(ProfileKeys::PCMetUsername,
                    settings.pcmet.www_credentials.username, username);
  const bool password_changed =
    Profile::Update(ProfileKeys::PCMetPassword,
                    settings.pcmet.www_credentials.password, password);

  _changed |= username_changed || password_changed;
#else
  (void)_changed;
#endif

  return true;
}

std::unique_ptr<Widget>
CreatePCMetConfigPanel()
{
  return std::make_unique<PCMetConfigPanel>();
}
