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
           _T("%.0f°"), _T("%.0f°"), // unfortunately AngleDataField does not
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
