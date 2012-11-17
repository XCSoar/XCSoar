/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "AirspaceConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/Button.hpp"
#include "Form/RowFormWidget.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

#ifdef HAVE_ALPHA_BLEND
#include "Screen/GDI/AlphaBlend.hpp"
#endif

enum ControlIndex {
  AirspaceDisplay,
  ClipAltitude,
  AltWarningMargin,
  AirspaceWarnings,
  WarningTime,
  AcknowledgeTime,
  UseBlackOutline,
  AirspaceFillMode,
  AirspaceTransparency
};

static constexpr StaticEnumChoice as_display_list[] = {
  { (unsigned)AirspaceDisplayMode::ALLON, N_("All on"),
    N_("All airspaces are displayed.") },
  { (unsigned)AirspaceDisplayMode::CLIP, N_("Clip"),
    N_("Display airspaces below the clip altitude.") },
  { (unsigned)AirspaceDisplayMode::AUTO, N_("Auto"),
    N_("Display airspaces within a margin of the glider.") },
  { (unsigned)AirspaceDisplayMode::ALLBELOW, N_("All below"),
    N_("Display airspaces below the glider or within a margin.") },
  { 0 }
};

static constexpr StaticEnumChoice as_fill_mode_list[] = {
  { (unsigned)AirspaceRendererSettings::FillMode::DEFAULT, N_("Default"),
    N_("This selects the best performing option for your hardware. "
      "In fact it favours 'fill padding' except for PPC 2000 system.") },
  { (unsigned)AirspaceRendererSettings::FillMode::ALL, N_("Fill all"),
    N_("Transparently fills the airspace colour over the whole area.") },
  { (unsigned)AirspaceRendererSettings::FillMode::PADDING, N_("Fill padding"),
    N_("Draws a solid outline with a half transparent border around the airspace.") },
  { 0 }
};

class AirspaceConfigPanel
  : public RowFormWidget, DataFieldListener {
private:
  WndButton *buttonColors, *buttonMode;

public:
  AirspaceConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void ShowDisplayControls(AirspaceDisplayMode mode);
  void ShowWarningControls(bool visible);
  void SetButtonsVisible(bool active);

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df);
};

static void
OnAirspaceColoursClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(true);
}

static void
OnAirspaceModeClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(false);
}

void
AirspaceConfigPanel::ShowDisplayControls(AirspaceDisplayMode mode)
{
  SetRowVisible(ClipAltitude,
                mode == AirspaceDisplayMode::CLIP);

  SetRowVisible(AltWarningMargin,
                mode == AirspaceDisplayMode::AUTO ||
                mode == AirspaceDisplayMode::ALLBELOW);
}

void
AirspaceConfigPanel::ShowWarningControls(bool visible)
{
  SetRowVisible(WarningTime, visible);
  SetRowVisible(AcknowledgeTime, visible);
}

void
AirspaceConfigPanel::SetButtonsVisible(bool active)
{
  if (buttonColors != NULL)
    buttonColors->SetVisible(active);

  if (buttonMode != NULL)
    buttonMode->SetVisible(active);
}

void
AirspaceConfigPanel::Show(const PixelRect &rc)
{
  if (buttonColors != NULL) {
    buttonColors->set_text(_("Colours"));
    buttonColors->SetOnClickNotify(OnAirspaceColoursClicked);
  }

  if (buttonMode != NULL) {
    buttonMode->set_text(_("Filter"));
    buttonMode->SetOnClickNotify(OnAirspaceModeClicked);
  }

  SetButtonsVisible(true);
  RowFormWidget::Show(rc);
}

void
AirspaceConfigPanel::Hide()
{
  RowFormWidget::Hide();
  SetButtonsVisible(false);
}

void
AirspaceConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(AirspaceDisplay, df)) {
    AirspaceDisplayMode mode = (AirspaceDisplayMode)df.GetAsInteger();
    ShowDisplayControls(mode);
  } else if (IsDataField(AirspaceWarnings, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    ShowWarningControls(dfb.GetAsBoolean());
  }
}

void
AirspaceConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::GetMapSettings().airspace;

  RowFormWidget::Prepare(parent, rc);

  AddEnum(_("Airspace display"),
          _("Controls filtering of airspace for display and warnings.  The airspace filter button also allows filtering of display and warnings independently for each airspace class."),
          as_display_list, (unsigned)renderer.altitude_mode, this);

  AddFloat(_("Clip altitude"),
           _("For clip airspace mode, this is the altitude below which airspace is displayed."),
           _T("%.0f %s"), _T("%.0f"), fixed(0), fixed(20000), fixed(100), false, UnitGroup::ALTITUDE, fixed(renderer.clip_altitude));

  AddFloat(_("Margin"),
           _("For auto and all below airspace mode, this is the altitude above/below which airspace is included."),
           _T("%.0f %s"), _T("%.0f"), fixed(0), fixed(10000), fixed(100), false, UnitGroup::ALTITUDE, fixed(computer.warnings.altitude_warning_margin));

  AddBoolean(_("Warnings"), _("Enable/disable all airspace warnings."),
             computer.enable_warnings, this);

  AddTime(_("Warning time"),
          _("This is the time before an airspace incursion is estimated at which the system will warn the pilot."),
          10, 1000, 5, computer.warnings.warning_time);
  SetExpertRow(WarningTime);

  AddTime(_("Acknowledge time"),
          _("This is the time period in which an acknowledged airspace warning will not be repeated."),
          10, 1000, 5, computer.warnings.acknowledgement_time);
  SetExpertRow(AcknowledgeTime);

  AddBoolean(_("Use black outline"),
             _("Draw a black outline around each airspace rather than the airspace color."),
             renderer.black_outline);
  SetExpertRow(UseBlackOutline);

  AddEnum(_("Airspace fill mode"),
          _("Specifies the mode for filling the airspace area."),
          as_fill_mode_list, (unsigned)renderer.fill_mode);
  SetExpertRow(AirspaceFillMode);

#if !defined(ENABLE_OPENGL) && defined(HAVE_ALPHA_BLEND)
  if (AlphaBlendAvailable()) {
    AddBoolean(_("Airspace transparency"), _("If enabled, then airspaces are filled transparently."),
               renderer.transparency);
    SetExpertRow(AirspaceTransparency);
  }
#endif

  buttonColors = ConfigPanel::GetExtraButton(1);
  assert(buttonColors != NULL);

  buttonMode = ConfigPanel::GetExtraButton(2);
  assert(buttonMode != NULL);

  ShowDisplayControls(renderer.altitude_mode); // TODO make this work the first time
  ShowWarningControls(computer.enable_warnings);
}


bool
AirspaceConfigPanel::Save(bool &_changed, bool &require_restart)
{
  bool changed = false;

  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  AirspaceRendererSettings &renderer =
    CommonInterface::SetMapSettings().airspace;

  changed |= SaveValueEnum(AirspaceDisplay, ProfileKeys::AltMode, renderer.altitude_mode);

  changed |= SaveValue(ClipAltitude, UnitGroup::ALTITUDE, ProfileKeys::ClipAlt, renderer.clip_altitude);

  changed |= SaveValue(AltWarningMargin, UnitGroup::ALTITUDE, ProfileKeys::AltMargin, computer.warnings.altitude_warning_margin);

  changed |= SaveValue(AirspaceWarnings, ProfileKeys::AirspaceWarning, computer.enable_warnings);

  if (SaveValue(WarningTime, ProfileKeys::WarningTime, computer.warnings.warning_time)) {
    changed = true;
    require_restart = true;
  }

  if (SaveValue(AcknowledgeTime, ProfileKeys::AcknowledgementTime,
                computer.warnings.acknowledgement_time)) {
    changed = true;
    require_restart = true;
  }

  changed |= SaveValue(UseBlackOutline, ProfileKeys::AirspaceBlackOutline, renderer.black_outline);

  changed |= SaveValueEnum(AirspaceFillMode, ProfileKeys::AirspaceFillMode, renderer.fill_mode);

#ifndef ENABLE_OPENGL
#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    changed |= SaveValue(AirspaceTransparency, ProfileKeys::AirspaceTransparency,
                         renderer.transparency);
#endif
#endif /* !OpenGL */

  _changed |= changed;

  return true;
}


Widget *
CreateAirspaceConfigPanel()
{
  return new AirspaceConfigPanel();
}

