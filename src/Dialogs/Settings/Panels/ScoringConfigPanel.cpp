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

#include "ScoringConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Engine/Contest/Solvers/Contests.hpp"

enum ControlIndex {
  Contests,
  PREDICT_CONTEST,
  SPACER,
  SHOW_FAI_TRIANGLE_AREAS,
  FAI_TRIANGLE_THRESHOLD,
};

class ScoringConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  ScoringConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

protected:
  void ShowFAITriangleControls(bool show);

public:
  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
ScoringConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(SHOW_FAI_TRIANGLE_AREAS, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    ShowFAITriangleControls(dfb.GetAsBoolean());
  }
}

void
ScoringConfigPanel::ShowFAITriangleControls(bool show)
{
  SetRowVisible(FAI_TRIANGLE_THRESHOLD, show);
}

static constexpr StaticEnumChoice fai_triangle_threshold_list[] = {
  { (unsigned)FAITriangleSettings::Threshold::FAI, _T("750km (FAI)") },
  { (unsigned)FAITriangleSettings::Threshold::KM500, _T("500km (OLC, DMSt)") },
  { 0 }
};

void
ScoringConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const ContestSettings &contest_settings = settings_computer.contest;
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  const StaticEnumChoice contests_list[] = {
    { (unsigned)Contest::NONE, ContestToString(Contest::NONE),
      N_("Disable contest calculations") },
    { (unsigned)Contest::OLC_FAI, ContestToString(Contest::OLC_FAI),
      N_("Conforms to FAI triangle rules. Three turns and common start and finish. No leg less than 28% "
          "of total except for tasks longer than 500km: No leg less than 25% or larger than 45%.") },
    { (unsigned)Contest::OLC_CLASSIC, ContestToString(Contest::OLC_CLASSIC),
      N_("Up to seven points including start and finish, finish height must not be lower than "
          "start height less 1000 meters.") },
    { (unsigned)Contest::OLC_LEAGUE, ContestToString(Contest::OLC_LEAGUE),
      N_("The most recent contest with Sprint task rules.") },
    { (unsigned)Contest::OLC_PLUS, ContestToString(Contest::OLC_PLUS),
      N_("A combination of Classic and FAI rules. 30% of the FAI score are added to the Classic score.") },
    { (unsigned)Contest::DMST, ContestToString(Contest::DMST),
      /* German competition, no translation */
      _T("Deutsche Meisterschaft im Streckensegelflug.") },
    { (unsigned)Contest::XCONTEST, ContestToString(Contest::XCONTEST),
      N_("PG online contest with different track values: Free flight - 1 km = 1.0 point; "
          "flat trianlge - 1 km = 1.2 p; FAI triangle - 1 km = 1.4 p.") },
    { (unsigned)Contest::DHV_XC, ContestToString(Contest::DHV_XC),
      N_("European PG online contest of the DHV organization. Pretty much the same as the XContest rules, "
          "but with different track values: 1 km = 1.5 points, 1.75 p and 2.0 p for FAI triangles respectively.") },
    { (unsigned)Contest::SIS_AT, ContestToString(Contest::SIS_AT),
      N_("Austrian online glider contest. Tracks around max. six waypoints are scored. The "
          "bounding box part with 1 km = 1.0 point and the additional zick-zack part with 1 km = 0.5 p.") },
    { (unsigned)Contest::NET_COUPE, ContestToString(Contest::NET_COUPE),
      N_("The FFVV NetCoupe \"libre\" competiton.") },
    { (unsigned)Contest::WEGLIDE_FREE, ContestToString(Contest::WEGLIDE_FREE),
      N_("WeGlide combines multiple scoring systems in the WeGlide Free contest. The free score is a combination "
          "of the free distance score and the area bonus. For the area bonus, the scoring program determines the "
          "largest FAI triangle and the largest Out & Return distance that can be fitted into the flight route.") },
    { (unsigned)Contest::WEGLIDE_OR, ContestToString(Contest::WEGLIDE_OR),
      N_("A start point, one turn point and a finish point are chosen from the flight path such that "
          "the distance between the start point and the turn point is maximized") },
    { 0 }
  };
  AddEnum(_("Contest"),
      _("Select the rules used for calculating optimal points for a contest."),
          contests_list, (unsigned)contest_settings.contest);

  AddBoolean(_("Predict Contest"),
             _("If enabled, then the next task point is included in the "
               "score calculation, assuming that you will reach it."),
             contest_settings.predict);

  AddSpacer();
  SetExpertRow(SPACER);

  AddBoolean(_("FAI triangle areas"),
             _("Show FAI triangle areas on the map."),
             map_settings.show_fai_triangle_areas, this);
  SetExpertRow(SHOW_FAI_TRIANGLE_AREAS);

  AddEnum(_("FAI triangle threshold"),
          _("Specifies which threshold is used for \"large\" FAI triangles."),
          fai_triangle_threshold_list,
          (unsigned)map_settings.fai_triangle_settings.threshold);
  SetExpertRow(FAI_TRIANGLE_THRESHOLD);

  ShowFAITriangleControls(map_settings.show_fai_triangle_areas);
}

bool
ScoringConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  ContestSettings &contest_settings = settings_computer.contest;
  MapSettings &map_settings = CommonInterface::SetMapSettings();

  changed |= SaveValueEnum(Contests, ProfileKeys::OLCRules,
                           contest_settings.contest);
  changed |= SaveValueEnum(PREDICT_CONTEST, ProfileKeys::PredictContest,
                           contest_settings.predict);

  changed |= SaveValue(SHOW_FAI_TRIANGLE_AREAS,
                       ProfileKeys::ShowFAITriangleAreas,
                       map_settings.show_fai_triangle_areas);

  changed |= SaveValueEnum(FAI_TRIANGLE_THRESHOLD,
                           ProfileKeys::FAITriangleThreshold,
                           map_settings.fai_triangle_settings.threshold);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateScoringConfigPanel()
{
  return std::make_unique<ScoringConfigPanel>();
}
