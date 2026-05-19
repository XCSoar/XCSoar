// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherConfigPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/Features.hpp"
#include "Widget/RowFormWidget.hpp"
#include "net/http/Features.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "util/NumberParser.hpp"

enum ControlIndex {
#ifdef HAVE_PCMET
  PCMET_USER,
  PCMET_PASSWORD,
#if 0
  PCMET_FTP_USER,
  PCMET_FTP_PASSWORD,
#endif
#endif

#ifdef HAVE_HTTP
  ENABLE_TIM,
  /* AddSpacer() in Prepare() — visual section divider between general
     weather rows and the XCTherm group. Spacers DO consume an index
     slot in the row list (HLine is a real Row), so a dummy enum entry
     keeps every SaveValue() / AddXxx() index aligned. Same trick
     OpenSoar uses in its Skysight panel. */
  XCTHERM_SPACER,
  XCTHERM_EMAIL,
  XCTHERM_PASSWORD,
  XCTHERM_REGION,
  XCTHERM_AUTO_SWITCH,
  XCTHERM_OVERLAY_LOCATION,
#endif
};

#ifdef HAVE_HTTP

static constexpr StaticEnumChoice xctherm_overlay_location_list[] = {
  { (unsigned)XCThermSettings::OverlayLocation::MAIN_MAP,
    N_("On every map"),
    N_("Show the XCTherm wave forecast overlay on every map page.\n"
       "The altitude/time cursor at the bottom only appears on pages "
       "where you assigned it as the \"Unten\" widget in Aussehen → "
       "Seiten (Config → System → Pages).") },
  { (unsigned)XCThermSettings::OverlayLocation::SEPARATE_MAP,
    N_("Only on dedicated XCTherm page"),
    N_("Show the overlay only on pages that have the XCTherm cursor as "
       "their \"Unten\" widget. Assign it in Aussehen → Seiten "
       "(Config → System → Pages). Pages without that cursor stay "
       "overlay-free.") },
  nullptr
};

/* The CH/UK constants live as numbered model IDs in XCThermDialog.cpp;
   re-declare them here as named enums so the picker stays readable. */
enum class XCThermRegion : unsigned {
  CH = 0,
  UK = 1,
};

static constexpr StaticEnumChoice xctherm_region_list[] = {
  { (unsigned)XCThermRegion::CH, N_("CH (Alps)"),
    N_("Covers the entire Alpine arc from Vienna to Perpignan, "
       "forecasting wave throughout the whole Alps region. "
       "ICON-CH model.") },
  { (unsigned)XCThermRegion::UK, N_("UK"),
    N_("United Kingdom. ICON-UK model.") },
  nullptr
};

#endif

class WeatherConfigPanel final
  : public RowFormWidget {
public:
  WeatherConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
WeatherConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
#if defined(HAVE_PCMET) || defined(HAVE_HTTP)
  const auto &settings = CommonInterface::GetComputerSettings().weather;
#endif

  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_PCMET
  AddText("pc_met Username", "",
          settings.pcmet.www_credentials.username);
  AddPassword("pc_met Password", "",
              settings.pcmet.www_credentials.password);

#if 0
  // code disabled because DWD has terminated our access */
  AddText("pc_met FTP Username", "",
          settings.pcmet.ftp_credentials.username);
  AddPassword("pc_met FTP Password", "",
              settings.pcmet.ftp_credentials.password);
#endif
#endif

#ifdef HAVE_HTTP
  AddBoolean("Thermal Information Map",
             _("Show thermal locations downloaded from Thermal Information Map (thermalmap.info)."),
             settings.enable_tim);

  /* Visual divider between the general weather row(s) above and the
     XCTherm group below — same pattern OpenSoar uses for its Skysight
     section in src/Dialogs/Settings/Panels/WeatherConfigPanel.cpp. */
  AddSpacer();

  AddText(_("XCTherm email"),
          _("Email address for your XCTherm account."),
          settings.xctherm.credentials.email);

  AddPassword(_("XCTherm password"),
              _("Password for your XCTherm account."),
              settings.xctherm.credentials.password);

  AddEnum(_("XCTherm region"),
          _("Forecast region. Changes which model XCTherm fetches data "
            "from. Restart or re-download after changing."),
          xctherm_region_list,
          settings.xctherm.model);

  AddBoolean(_("XCTherm Auto Layer/Time"),
             _("Automatically switch altitude layer based on GPS altitude "
               "and forecast time based on UTC clock."),
             settings.xctherm.auto_switch);

  AddEnum(_("XCTherm overlay"),
          _("Where to render the XCTherm wave forecast overlay."),
          xctherm_overlay_location_list,
          (unsigned)settings.xctherm.overlay_location);
#endif
}

bool
WeatherConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

#if defined(HAVE_PCMET) || defined(HAVE_HTTP)
  auto &settings = CommonInterface::SetComputerSettings().weather;
#endif

#ifdef HAVE_PCMET
  changed |= SaveValue(PCMET_USER, ProfileKeys::PCMetUsername,
                       settings.pcmet.www_credentials.username);

  changed |= SaveValue(PCMET_PASSWORD, ProfileKeys::PCMetPassword,
                       settings.pcmet.www_credentials.password);

#if 0
  // code disabled because DWD has terminated our access */
  changed |= SaveValue(PCMET_FTP_USER, ProfileKeys::PCMetFtpUsername,
                       settings.pcmet.ftp_credentials.username);

  changed |= SaveValue(PCMET_FTP_PASSWORD, ProfileKeys::PCMetFtpPassword,
                       settings.pcmet.ftp_credentials.password);
#endif
#endif

#ifdef HAVE_HTTP
  changed |= SaveValue(ENABLE_TIM, ProfileKeys::EnableThermalInformationMap,
                       settings.enable_tim);

  changed |= SaveValueEnum(XCTHERM_OVERLAY_LOCATION,
                           ProfileKeys::XCThermOverlayLocation,
                           settings.xctherm.overlay_location);

  changed |= SaveValue(XCTHERM_AUTO_SWITCH,
                       ProfileKeys::XCThermAutoSwitch,
                       settings.xctherm.auto_switch);

  changed |= SaveValue(XCTHERM_EMAIL, ProfileKeys::XCThermEmail,
                       settings.xctherm.credentials.email);

  changed |= SaveValue(XCTHERM_PASSWORD, ProfileKeys::XCThermPassword,
                       settings.xctherm.credentials.password);

  /* Region used to live as the dialog's "Model" button — now only
     configurable here, matching how OpenSoar handles Skysight. The row
     is an enum dropdown, so save it via SaveValueEnum (using Integer
     here would assert at runtime — the DataField is enum-typed). */
  changed |= SaveValueEnum(XCTHERM_REGION, ProfileKeys::XCThermModel,
                           settings.xctherm.model);
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWeatherConfigPanel()
{
  return std::make_unique<WeatherConfigPanel>();
}
