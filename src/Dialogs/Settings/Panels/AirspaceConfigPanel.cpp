// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "ui/canvas/Features.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"

using namespace std::chrono;

enum ControlIndex {
  AirspaceDisplay,
  AirspaceLabelSelection,
  ClipAltitude,
  AltWarningMargin,
  AirspaceWarnings,
  WarningDialog,
  WarningTime,
  RepetitiveSound,
  AcknowledgeTime,
  UseBlackOutline,
  AirspaceFillMode,
  AirspaceTransparency
};

static constexpr StaticEnumChoice as_display_list[] = {
  { AirspaceDisplayMode::ALLON, N_("All on"),
    N_("All airspaces are displayed.") },
  { AirspaceDisplayMode::CLIP, N_("Clip"),
    N_("Display airspaces below the clip altitude.") },
  { AirspaceDisplayMode::AUTO, N_("Auto"),
    N_("Display airspaces within a margin of the glider.") },
  { AirspaceDisplayMode::ALLBELOW, N_("All below"),
    N_("Display airspaces below the glider or within a margin.") },
  nullptr
};

static constexpr StaticEnumChoice as_fill_mode_list[] = {
  { AirspaceRendererSettings::FillMode::DEFAULT, N_("Default"),
    N_("This selects the best performing option for your hardware. "
      "In fact it favours 'fill padding' except for PPC 2000 system.") },
  { AirspaceRendererSettings::FillMode::ALL, N_("Fill all"),
    N_("Transparently fills the airspace colour over the whole area.") },
  { AirspaceRendererSettings::FillMode::PADDING, N_("Fill padding"),
    N_("Draws a solid outline with a half transparent border around the airspace.") },
  { AirspaceRendererSettings::FillMode::NONE, N_("No fill"),
    N_("Don't fill the airspace area.") },
  nullptr
};

static constexpr StaticEnumChoice as_label_selection_list[] = {
  { AirspaceRendererSettings::LabelSelection::NONE, N_("None"),
    N_("No labels will be displayed.") },
  { AirspaceRendererSettings::LabelSelection::ALL, N_("All"),
    N_("All labels will be displayed.") },
  nullptr
};

class AirspaceConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  AirspaceConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void ShowDisplayControls(AirspaceDisplayMode mode);
  void ShowWarningControls(bool visible);

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

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
  SetRowVisible(WarningDialog, visible);
  SetRowVisible(WarningTime, visible);
  SetRowVisible(RepetitiveSound, visible);
  SetRowVisible(AcknowledgeTime, visible);
}

void
AirspaceConfigPanel::Show(const PixelRect &rc) noexcept
{
  ConfigPanel::BorrowExtraButton(1, _("Colours"), [](){
    dlgAirspaceShowModal(true);
  });

  ConfigPanel::BorrowExtraButton(2, _("Filter"), [](){
    dlgAirspaceShowModal(false);
  });

  RowFormWidget::Show(rc);
}

void
AirspaceConfigPanel::Hide() noexcept
{
  RowFormWidget::Hide();
  ConfigPanel::ReturnExtraButton(1);
  ConfigPanel::ReturnExtraButton(2);
}

void
AirspaceConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(AirspaceDisplay, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    AirspaceDisplayMode mode = (AirspaceDisplayMode)dfe.GetValue();
    ShowDisplayControls(mode);
  } else if (IsDataField(AirspaceWarnings, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    ShowWarningControls(dfb.GetValue());
  }
}

void
AirspaceConfigPanel::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::GetMapSettings().airspace;
  const UISettings &ui_settings =
    CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  AddEnum(_("Airspace display"),
          _("Controls filtering of airspace for display and warnings. The airspace filter button also allows filtering of display and warnings independently for each airspace class."),
          as_display_list, (unsigned)renderer.altitude_mode, this);

  AddEnum(_("Label visibility"),
          _("Determines what labels are displayed."),
          as_label_selection_list, (unsigned)renderer.label_selection);
  SetExpertRow(AirspaceLabelSelection);

  AddFloat(_("Clip altitude"),
           _("For clip airspace mode, this is the altitude below which airspace is displayed."),
           "%.0f %s", "%.0f", 0, 20000, 100, false,
           UnitGroup::ALTITUDE, renderer.clip_altitude);

  AddFloat(_("Margin"),
           _("For auto and all below airspace mode, this is the altitude above/below which airspace is included."),
           "%.0f %s", "%.0f", 0, 10000, 100, false,
           UnitGroup::ALTITUDE, computer.warnings.altitude_warning_margin);

  AddBoolean(_("Warnings"), _("Enable/disable all airspace warnings."),
             computer.enable_warnings, this);

  AddBoolean(_("Warnings dialog"),
             _("Enable/disable displaying airspaces warnings dialog."),
             ui_settings.enable_airspace_warning_dialog, this);
  SetExpertRow(WarningDialog);

  AddDuration(_("Warning time"),
              _("This is the time before an airspace incursion is estimated at which the system will warn the pilot."),
              seconds{10}, seconds{1000}, seconds{5},
              computer.warnings.warning_time);
  SetExpertRow(WarningTime);

  AddBoolean(_("Repetitive sound"),
             _("Enable/disable repetitive warning sound when airspaces warnings dialog is displayed."),
             computer.warnings.repetitive_sound, this);
  SetExpertRow(RepetitiveSound);

  AddDuration(_("Acknowledge time"),
              _("This is the time period in which an acknowledged airspace warning will not be repeated."),
              seconds{10}, seconds{1000}, seconds{5},
              computer.warnings.acknowledgement_time);
  SetExpertRow(AcknowledgeTime);

  AddBoolean(_("Use black outline"),
             _("Draw a black outline around each airspace rather than the airspace color."),
             renderer.black_outline);
  SetExpertRow(UseBlackOutline);

  AddEnum(_("Airspace fill mode"),
          _("Specifies the mode for filling the airspace area."),
          as_fill_mode_list, (unsigned)renderer.fill_mode);
  SetExpertRow(AirspaceFillMode);

#if defined(HAVE_HATCHED_BRUSH) && defined(HAVE_ALPHA_BLEND)
  AddBoolean(_("Airspace transparency"), _("If enabled, then airspaces are filled transparently."),
             renderer.transparency);
  SetExpertRow(AirspaceTransparency);
#endif

  ShowDisplayControls(renderer.altitude_mode); // TODO make this work the first time
  ShowWarningControls(computer.enable_warnings);
}


bool
AirspaceConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  AirspaceRendererSettings &renderer =
    CommonInterface::SetMapSettings().airspace;
  UISettings &ui_settings = CommonInterface::SetUISettings();

  changed |= SaveValueEnum(AirspaceDisplay, ProfileKeys::AltMode, renderer.altitude_mode);

  changed |= SaveValueEnum(AirspaceLabelSelection, ProfileKeys::AirspaceLabelSelection, renderer.label_selection);

  changed |= SaveValue(ClipAltitude, UnitGroup::ALTITUDE, ProfileKeys::ClipAlt, renderer.clip_altitude);

  changed |= SaveValue(AltWarningMargin, UnitGroup::ALTITUDE, ProfileKeys::AltMargin, computer.warnings.altitude_warning_margin);

  changed |= SaveValue(AirspaceWarnings, ProfileKeys::AirspaceWarning, computer.enable_warnings);

  changed |= SaveValue(WarningDialog, ProfileKeys::AirspaceWarningDialog,
                       ui_settings.enable_airspace_warning_dialog);

  if (SaveValue(WarningTime, ProfileKeys::WarningTime, computer.warnings.warning_time)) {
    changed = true;
    require_restart = true;
  }

  changed |= SaveValue(RepetitiveSound, ProfileKeys::RepetitiveSound,
                       computer.warnings.repetitive_sound);

  if (SaveValue(AcknowledgeTime, ProfileKeys::AcknowledgementTime,
                computer.warnings.acknowledgement_time)) {
    changed = true;
    require_restart = true;
  }

  changed |= SaveValue(UseBlackOutline, ProfileKeys::AirspaceBlackOutline, renderer.black_outline);

  changed |= SaveValueEnum(AirspaceFillMode, ProfileKeys::AirspaceFillMode, renderer.fill_mode);

#if defined(HAVE_HATCHED_BRUSH) && defined(HAVE_ALPHA_BLEND)
  changed |= SaveValue(AirspaceTransparency, ProfileKeys::AirspaceTransparency,
                       renderer.transparency);
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateAirspaceConfigPanel()
{
  return std::make_unique<AirspaceConfigPanel>();
}

