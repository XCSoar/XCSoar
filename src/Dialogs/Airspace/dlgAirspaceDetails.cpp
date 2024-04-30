// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

#include <cassert>

class AirspaceDetailsWidget final
  : public RowFormWidget {
  ConstAirspacePtr airspace;
  ProtectedAirspaceWarningManager *warnings;

public:
  /**
   * Hack to allow the widget to close its surrounding dialog.
   */
  WndForm *dialog;

  AirspaceDetailsWidget(ConstAirspacePtr _airspace,
                        ProtectedAirspaceWarningManager *_warnings)
    :RowFormWidget(UIGlobals::GetDialogLook()),
     airspace(std::move(_airspace)), warnings(_warnings) {}

  void AckDayOrEnable() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
AirspaceDetailsWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                               [[maybe_unused]] const PixelRect &rc) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();

  StaticString<64> buffer;

  AddMultiLine(airspace->GetName());

  if (airspace->GetRadioFrequency().Format(buffer.data(),
                                           buffer.capacity()) != nullptr) {
    buffer += _T(" MHz");
    AddReadOnly(_("Radio"), nullptr, buffer);

    AddButton(_("Set Active Frequency"), [this](){
      ActionInterface::SetActiveFrequency(airspace->GetRadioFrequency(),
                                          airspace->GetName());
    });

    AddButton(_("Set Standby Frequency"), [this](){
      ActionInterface::SetStandbyFrequency(airspace->GetRadioFrequency(),
                                           airspace->GetName());
    });
  }

  AddReadOnly(_("Class"), nullptr, AirspaceFormatter::GetClassShort(*airspace));
  AddReadOnly(_("Type"), nullptr, AirspaceFormatter::GetType(*airspace));

  AirspaceFormatter::FormatAltitude(buffer.data(), airspace->GetTop());
  AddReadOnly(_("Top"), nullptr, buffer);

  AirspaceFormatter::FormatAltitude(buffer.data(), airspace->GetBase());
  AddReadOnly(_("Base"), nullptr, buffer);

  if (warnings != nullptr) {
    const GeoPoint closest =
      airspace->ClosestPoint(basic.location, warnings->GetProjection());
    const auto distance = closest.Distance(basic.location);

    AddReadOnly(_("Distance"), nullptr, FormatUserDistance(distance));
  }
}

void
AirspaceDetailsWidget::AckDayOrEnable() noexcept
{
  assert(warnings != nullptr);

  const bool acked = warnings->GetAckDay(*airspace);
  warnings->AcknowledgeDay(airspace, !acked);

  dialog->SetModalResult(mrOK);
}

void
dlgAirspaceDetails(ConstAirspacePtr airspace,
                   ProtectedAirspaceWarningManager *warnings)
{
  AirspaceDetailsWidget *widget =
    new AirspaceDetailsWidget(airspace, warnings);
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _("Airspace Details"), widget);

  if (warnings != nullptr) {
    widget->dialog = &dialog;
    dialog.AddButton(warnings->GetAckDay(*airspace)
                     ? _("Enable") : _("Ack Day"),
                     [widget](){ widget->AckDayOrEnable(); });
  }
  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}
