// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "UtilsSettings.hpp"

static constexpr StaticEnumChoice as_display_list[] = {
  { AirspaceDisplayMode::ALLON, N_("All on"),
    N_("All airspaces are displayed.") },
  { AirspaceDisplayMode::CLIP, N_("Clip"),
    N_("Display airspaces below the clip altitude.") },
  { AirspaceDisplayMode::AUTO, NC_("Setting", "Auto"),
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

/**
 * The display of the airspaces and the warnings about them.  The
 * views of the filter and of the colours act on the settings and the
 * profile themselves; Save() writes the values of this page one by
 * one, so that it does not undo them.
 */
class AirspaceConfigPanel final : public ConfigListPanel {
  AirspaceRendererSettings renderer;
  AirspaceWarningConfig warnings;
  bool enable_warnings, warning_dialog;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
AirspaceConfigPanel::LoadSettings() noexcept
{
  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;

  renderer = CommonInterface::GetMapSettings().airspace;
  warnings = computer.warnings;
  enable_warnings = computer.enable_warnings;
  warning_dialog =
    CommonInterface::GetUISettings().enable_airspace_warning_dialog;
}

void
AirspaceConfigPanel::Fill() noexcept
{
  AddGroup();

  AddEnumItem(_("Airspace display"),
              _("Controls filtering of airspace for display and warnings. The airspace filter button also allows filtering of display and warnings independently for each airspace class."),
              as_display_list, renderer.altitude_mode);

  if (renderer.altitude_mode == AirspaceDisplayMode::CLIP)
    AddAltitudeItem(_("Clip altitude"),
                    _("For clip airspace mode, this is the altitude below which airspace is displayed."),
                    0, 20000, 100, renderer.clip_altitude);

  if (renderer.altitude_mode == AirspaceDisplayMode::AUTO ||
      renderer.altitude_mode == AirspaceDisplayMode::ALLBELOW)
    AddAltitudeItem(_("Margin"),
                    _("For auto and all below airspace mode, this is the altitude above/below which airspace is included."),
                    0, 10000, 100, warnings.altitude_warning_margin);

  if (IsExpert()) {
    AddEnumItem(_("Label visibility"),
                _("Determines what labels are displayed."),
                as_label_selection_list, renderer.label_selection);

#ifdef HAVE_HTTP
    AddToggleItem(_("Show NOTAM labels"),
                  _("Show brief NOTAM text labels on the map when zoomed in sufficiently."),
                  renderer.show_notam_labels);
#endif
  }

  /* the display and the warnings of each class */
  AddGroup();

  AddItem(_("Filter"), [](){
    dlgAirspaceShowModal(false);
  }, {.chevron = true});

  AddItem(_("Colours"), [](){
    dlgAirspaceShowModal(true);
  }, {.chevron = true});

  AddGroup();

  AddToggleItem(_("Warnings"), _("Enable/disable all airspace warnings."),
                enable_warnings);

  if (enable_warnings && IsExpert()) {
    AddToggleItem(_("Warnings dialog"),
                  _("Enable/disable displaying airspaces warnings dialog."),
                  warning_dialog);

    AddDurationItem(_("Warning time"),
                    _("This is the time before an airspace incursion is estimated at which the system will warn the pilot."),
                    10, 1000, 5, warnings.warning_time);

    AddToggleItem(_("Repetitive sound"),
                  _("Enable/disable repetitive warning sound when airspaces warnings dialog is displayed."),
                  warnings.repetitive_sound);

    AddDurationItem(_("Acknowledge time"),
                    _("This is the time period in which an acknowledged airspace warning will not be repeated."),
                    10, 1000, 5, warnings.acknowledgement_time);
  }

  if (IsExpert()) {
    AddGroup();

    AddToggleItem(_("Use black outline"),
                  _("Draw a black outline around each airspace rather than the airspace color."),
                  renderer.black_outline);

    AddEnumItem(_("Airspace fill mode"),
                _("Specifies the mode for filling the airspace area."),
                as_fill_mode_list, renderer.fill_mode);
  }
}

bool
AirspaceConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  AirspaceRendererSettings &settings_renderer =
    CommonInterface::SetMapSettings().airspace;
  UISettings &ui_settings = CommonInterface::SetUISettings();

  changed |= Profile::Update(ProfileKeys::AltMode,
                             settings_renderer.altitude_mode,
                             renderer.altitude_mode);

  changed |= Profile::Update(ProfileKeys::AirspaceLabelSelection,
                             settings_renderer.label_selection,
                             renderer.label_selection);

#ifdef HAVE_HTTP
  changed |= Profile::Update(ProfileKeys::AirspaceShowNOTAMLabels,
                             settings_renderer.show_notam_labels,
                             renderer.show_notam_labels);
#endif

  changed |= Profile::Update(ProfileKeys::ClipAlt,
                             settings_renderer.clip_altitude,
                             renderer.clip_altitude);

  changed |= Profile::Update(ProfileKeys::AltMargin,
                             computer.warnings.altitude_warning_margin,
                             warnings.altitude_warning_margin);

  changed |= Profile::Update(ProfileKeys::AirspaceWarning,
                             computer.enable_warnings, enable_warnings);

  changed |= Profile::Update(ProfileKeys::AirspaceWarningDialog,
                             ui_settings.enable_airspace_warning_dialog,
                             warning_dialog);

  if (Profile::Update(ProfileKeys::WarningTime,
                      computer.warnings.warning_time,
                      warnings.warning_time)) {
    changed = true;
    require_restart = true;
  }

  changed |= Profile::Update(ProfileKeys::RepetitiveSound,
                             computer.warnings.repetitive_sound,
                             warnings.repetitive_sound);

  if (Profile::Update(ProfileKeys::AcknowledgementTime,
                      computer.warnings.acknowledgement_time,
                      warnings.acknowledgement_time)) {
    changed = true;
    require_restart = true;
  }

  changed |= Profile::Update(ProfileKeys::AirspaceBlackOutline,
                             settings_renderer.black_outline,
                             renderer.black_outline);

  changed |= Profile::Update(ProfileKeys::AirspaceFillMode,
                             settings_renderer.fill_mode,
                             renderer.fill_mode);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateAirspaceConfigPanel()
{
  return std::make_unique<AirspaceConfigPanel>();
}
