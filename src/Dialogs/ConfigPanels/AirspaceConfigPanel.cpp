/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "DataField/Enum.hpp"
#include "DataField/Boolean.hpp"
#include "Form/Button.hpp"
#include "Form/RowFormWidget.hpp"
#include "Dialogs/Airspace.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

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

static const StaticEnumChoice  as_display_list[] = {
  { ALLON, N_("All on"),
    N_("All airspaces are displayed.") },
  { CLIP, N_("Clip"),
    N_("Display airspaces below the clip altitude.") },
  { AUTO, N_("Auto"),
    N_("Display airspaces within a margin of the glider.") },
  { ALLBELOW, N_("All below"),
    N_("Display airspaces below the glider or within a margin.") },
  { 0 }
};

static const StaticEnumChoice  as_fill_mode_list[] = {
  { AirspaceRendererSettings::AS_FILL_DEFAULT, N_("Default"),
    N_("") },
  { AirspaceRendererSettings::AS_FILL_ALL, N_("Fill all"),
    N_("") },
  { AirspaceRendererSettings::AS_FILL_PADDING, N_("Fill padding"),
    N_("") },
  { 0 }
};

class AirspaceConfigPanel : public RowFormWidget {
public:
  AirspaceConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(150)) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  void ShowDisplayControls(AirspaceDisplayMode_t mode);
  void ShowWarningControls(bool visible);
};


/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static AirspaceConfigPanel *instance;

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
AirspaceConfigPanel::ShowDisplayControls(AirspaceDisplayMode_t mode)
{
  GetControl(ClipAltitude).set_visible(mode == CLIP);
  GetControl(AltWarningMargin).set_visible(mode == AUTO || mode == ALLBELOW);
}

void
AirspaceConfigPanel::ShowWarningControls(bool visible)
{
  GetControl(WarningTime).set_visible(visible);
  GetControl(AcknowledgeTime).set_visible(visible);
}

static void
OnAirspaceDisplay(DataField *Sender,
                  DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  AirspaceDisplayMode_t mode = (AirspaceDisplayMode_t)df.GetAsInteger();
  instance->ShowDisplayControls(mode);
}

static void
OnAirspaceWarning(DataField *Sender,
                  DataField::DataAccessKind_t Mode)
{
  const DataFieldBoolean &df = *(const DataFieldBoolean *)Sender;
  instance->ShowWarningControls(df.GetAsBoolean());
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = { // TODO remove it
  DeclareCallBackEntry(OnAirspaceColoursClicked),
  DeclareCallBackEntry(OnAirspaceModeClicked),
  DeclareCallBackEntry(OnAirspaceDisplay),
  DeclareCallBackEntry(OnAirspaceWarning),
  DeclareCallBackEntry(NULL)
};

void
AirspaceConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::GetMapSettings().airspace;

  instance = this;

  RowFormWidget::Prepare(parent, rc);

  AddEnum(_("Airspace display"),
          _("Controls filtering of airspace for display and warnings.  The airspace filter button also allows filtering of display and warnings independently for each airspace class."),
          as_display_list ,renderer.altitude_mode, OnAirspaceDisplay);

  AddFloat(_("Clip altitude"),
           _("For clip airspace mode, this is the altitude below which airspace is displayed."),
           _T("%.0f %s"), _T("%.0f"), fixed(0), fixed(20000), fixed(100), false, ugAltitude, fixed(renderer.clip_altitude));

  AddFloat(_("Margin"),
           _("For auto and all below airspace mode, this is the altitude above/below which airspace is included."),
           _T("%.0f %s"), _T("%.0f"), fixed(0), fixed(10000), fixed(100), false, ugAltitude, fixed(computer.warnings.AltWarningMargin));

  AddBoolean(_("Warnings"), _("Enable/disable all airspace warnings."), computer.enable_warnings, OnAirspaceWarning);

  // TODO All below is for the Expert
  AddInteger(_("Warning time"),
             _("This is the time before an airspace incursion is estimated at which the system will warn the pilot."),
             _T("%u s"), _T("%u"), 10, 1000, 5, computer.warnings.WarningTime);

  AddInteger(_("Acknowledge time"),
             _("This is the time period in which an acknowledged airspace warning will not be repeated."),
             _T("%u s"), _T("%u"), 10, 1000, 5, computer.warnings.AcknowledgementTime);

  AddBoolean(_("Use black outline"),
             _("Draw a black outline around each airspace rather than the airspace color."),
             renderer.black_outline);

  AddEnum(_("Airspace fill mode"),
          _("Specifies the mode for filling the airspace area."),
          as_fill_mode_list, renderer.fill_mode);

#if !defined(ENABLE_OPENGL) && defined(HAVE_ALPHA_BLEND)
  if (AlphaBlendAvailable())
    AddBoolean(_("Airspace transparency"), _("If enabled, then airspaces are filled transparently."),
               renderer.transparency);
  else
#endif

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

  changed |= SaveValueEnum(AirspaceDisplay, szProfileAltMode, renderer.altitude_mode);

  changed |= SaveValue(ClipAltitude, ugAltitude, szProfileClipAlt, renderer.clip_altitude);

  changed |= SaveValue(AltWarningMargin, ugAltitude, szProfileAltMargin, computer.warnings.AltWarningMargin);

  changed |= SaveValue(AirspaceWarnings, szProfileAirspaceWarning, computer.enable_warnings);

  if (SaveValue(WarningTime, szProfileWarningTime, computer.warnings.WarningTime)) {
    changed = true;
    require_restart = true;
  }

  if (SaveValue(AcknowledgeTime, szProfileAcknowledgementTime,
                computer.warnings.AcknowledgementTime)) {
    changed = true;
    require_restart = true;
  }

  changed |= SaveValue(UseBlackOutline, szProfileAirspaceBlackOutline, renderer.black_outline);

  changed |= SaveValueEnum(AirspaceFillMode, szProfileAirspaceFillMode, renderer.fill_mode);

#ifndef ENABLE_OPENGL
#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    changed |= SaveValue(AirspaceTransparency, szProfileAirspaceTransparency,
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


// TODO Find a solution for the two buttons on the AirspaceConfig Panel
// and remove the code below

#include "Form/XMLWidget.hpp"

class AirspaceTmpButtonPanel : public XMLWidget {
public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
};


void
AirspaceTmpButtonPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(CallBackTable, parent,
             Layout::landscape ? _T("IDR_XML_AIRSPACECONFIGPANEL") :
                               _T("IDR_XML_AIRSPACECONFIGPANEL_L"));
}

Widget *
CreateAirspaceTmpButtonPanel()
{
  return new AirspaceTmpButtonPanel();
}
