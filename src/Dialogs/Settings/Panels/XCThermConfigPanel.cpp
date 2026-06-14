// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermConfigPanel.hpp"

#ifdef HAVE_HTTP

#include "Form/DataField/Enum.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/xctherm/XCThermAPI.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  XCTHERM_EMAIL,
  XCTHERM_PASSWORD,
  XCTHERM_REGION,
  XCTHERM_AUTO_SWITCH,
};

static constexpr StaticEnumChoice xctherm_region_list[] = {
  { unsigned(XCTherm::Region::CH), N_("CH (Alps)"),
    N_("Covers the entire Alpine arc from Vienna to Perpignan, "
       "forecasting wave throughout the whole Alps region. "
       "ICON-CH model.") },
  { unsigned(XCTherm::Region::UK), N_("UK"),
    N_("United Kingdom. ICON-UK model.") },
  nullptr
};

class XCThermConfigPanel final : public RowFormWidget {
public:
  XCThermConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
XCThermConfigPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  RowFormWidget::Prepare(parent, rc);

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
}

bool
XCThermConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;
  auto &settings = CommonInterface::SetComputerSettings().weather;

  changed |= SaveValue(XCTHERM_AUTO_SWITCH,
                       ProfileKeys::XCThermAutoSwitch,
                       settings.xctherm.auto_switch);

  changed |= SaveValue(XCTHERM_EMAIL, ProfileKeys::XCThermEmail,
                       settings.xctherm.credentials.email);

  changed |= SaveValue(XCTHERM_PASSWORD, ProfileKeys::XCThermPassword,
                       settings.xctherm.credentials.password);

  changed |= SaveValueEnum(XCTHERM_REGION, ProfileKeys::XCThermModel,
                           settings.xctherm.model);

  XCThermAPI::Instance().ApplySessionSettings(settings.xctherm);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateXCThermConfigPanel()
{
  return std::make_unique<XCThermConfigPanel>();
}

#endif /* HAVE_HTTP */
