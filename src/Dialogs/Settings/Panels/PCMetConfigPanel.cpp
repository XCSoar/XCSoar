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
  MOSMIX_FORECAST_TEMPERATURE,
#endif
};

class PCMetConfigPanel final : public RowFormWidget {
  /** the forecast switch belongs to the configuration dialogue only */
  const bool with_forecast_switch;

public:
  explicit PCMetConfigPanel(bool _with_forecast_switch) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     with_forecast_switch(_with_forecast_switch) {}

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

  /* No account of its own: MOSMIX is open data.  It sits here because
     it is the DWD, which is what a pilot looking for it will read. */
  if (with_forecast_switch)
    AddBoolean(_("Max. temp. from forecast"),
               _("Fill Max. temp. in the flight setup from the day's "
                 "DWD forecast for the nearest MOSMIX station.  Needs "
                 "a position fix, and fetches once a day; no account."),
               settings.mosmix_forecast_temperature);
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

  if (with_forecast_switch)
    changed |= SaveValue(MOSMIX_FORECAST_TEMPERATURE,
                         ProfileKeys::MosmixForecastTemperature,
                         settings.mosmix_forecast_temperature);
#endif

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreatePCMetConfigPanel()
{
  return std::make_unique<PCMetConfigPanel>(true);
}

std::unique_ptr<Widget>
CreatePCMetCredentialsPanel()
{
  return std::make_unique<PCMetConfigPanel>(false);
}
