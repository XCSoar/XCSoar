// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ATCSetup.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

class ATCSetupPanel : public RowFormWidget,
                      private DataFieldListener {
public:
  ATCSetupPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  /* virtual methods from class DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
ATCSetupPanel::Prepare(ContainerWindow &parent,
                       const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const Angle &declination =
    CommonInterface::GetComputerSettings().poi.magnetic_declination;

  AddFloat(_("Mag declination"),
           _("Magnetic declination used to calculate radial to reference. Negative values are W declination, positive is E."),
           "%.0f°", "%.0f°", // unfortunately AngleDataField does not
           -30.0, +30.0, 1.0, false, // support negative values to handle
           declination.Degrees(),    // this formatting more gracefully
           this);
}

void
ATCSetupPanel::OnModified(DataField &_df) noexcept
{
  DataFieldFloat &df = (DataFieldFloat &)_df;
  ComputerSettings &settings =
    CommonInterface::SetComputerSettings();

  settings.poi.magnetic_declination = Angle::Degrees(df.GetValue());
}

std::unique_ptr<Widget>
LoadATCSetupPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<ATCSetupPanel>();
}
