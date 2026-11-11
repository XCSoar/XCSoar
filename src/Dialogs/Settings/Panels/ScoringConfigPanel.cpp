// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScoringConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Engine/Contest/Solvers/Contests.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

static constexpr StaticEnumChoice fai_triangle_threshold_list[] = {
  { FAITriangleSettings::Threshold::FAI, "750km (FAI)" },
  { FAITriangleSettings::Threshold::KM500, "500km (OLC, DMSt)" },
  nullptr
};

/**
 * The contest which is scored, and the helpers the map draws for it.
 */
class ScoringConfigPanel final : public ConfigListPanel {
  /**
   * The contests, with their names from ContestToString(): the list
   * lives as long as the page, because the item which opens the
   * choice keeps a pointer to it.
   */
  const StaticEnumChoice contests_list[14] = {
    { Contest::NONE, ContestToString(Contest::NONE),
      N_("Disable contest calculations") },
    { Contest::OLC_FAI, ContestToString(Contest::OLC_FAI),
      N_("Conforms to FAI triangle rules. Three turns and common start and finish. No leg less than 28% "
          "of total except for tasks longer than 500km: No leg less than 25% or larger than 45%.") },
    { Contest::OLC_CLASSIC, ContestToString(Contest::OLC_CLASSIC),
      N_("Up to seven points including start and finish, finish height must not be lower than "
          "start height less 1000 meters.") },
    { Contest::OLC_LEAGUE, ContestToString(Contest::OLC_LEAGUE),
      N_("The most recent contest with Sprint task rules.") },
    { Contest::OLC_PLUS, ContestToString(Contest::OLC_PLUS),
      N_("A combination of Classic and FAI rules. 30% of the FAI score are added to the Classic score.") },
    { Contest::DMST, ContestToString(Contest::DMST),
      /* German competition, no translation */
      "Deutsche Meisterschaft im Streckensegelflug." },
    { Contest::XCONTEST, ContestToString(Contest::XCONTEST),
      N_("PG online contest with different track values: Free flight - 1 km = 1.0 point; "
          "flat triangle - 1 km = 1.2 p; FAI triangle - 1 km = 1.4 p.") },
    { Contest::DHV_XC, ContestToString(Contest::DHV_XC),
      N_("European PG online contest of the DHV organization. Pretty much the same as the XContest rules, "
          "but with different track values: 1 km = 1.5 points, 1.75 p and 2.0 p for FAI triangles respectively.") },
    { Contest::SIS_AT, ContestToString(Contest::SIS_AT),
      N_("Austrian online glider contest. Tracks around max. six waypoints are scored. The "
          "bounding box part with 1 km = 1.0 point and the additional zick-zack part with 1 km = 0.5 p.") },
    { Contest::NET_COUPE, ContestToString(Contest::NET_COUPE),
      N_("FFVP Federal Cup (NetCoupe) on WeGlide. The scored path has at most "
          "three turnpoints between start and finish and at least 25 km total. "
          "Points are proportional to credited distance, 100 divided by the "
          "glider handicap (DAeC-style index), and a success factor of 1.0 for "
          "a free flight or 1.2 for a task declared electronically before "
          "takeoff. XCSoar live scoring uses a success factor of 1.0 only, not "
          "the 1.2 multiplier for declared tasks.") },
    { Contest::WEGLIDE_FREE, ContestToString(Contest::WEGLIDE_FREE),
      N_("WeGlide combines multiple scoring systems in the WeGlide Free contest. The free score is a combination "
          "of the free distance score and the area bonus. For the area bonus, the scoring program determines the "
          "largest FAI triangle and the largest Out & Return distance that can be fitted into the flight route.") },
    { Contest::WEGLIDE_OR, ContestToString(Contest::WEGLIDE_OR),
      N_("A start point, one turn point and a finish point are chosen from the flight path such that "
          "the distance between the start point and the turn point is maximized.") },
    { Contest::CHARRON, ContestToString(Contest::CHARRON),
      N_("LVZC Charron.online, 5 legs under 200km 6 legs above. Minimum leg distance is 20km, 5 points per km.") },
    nullptr
  };

  Contest contest;
  bool predict;

  bool show_fai_triangle_areas;
  FAITriangleSettings::Threshold fai_triangle_threshold;
  bool show_95_percent_rule_helpers;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
ScoringConfigPanel::LoadSettings() noexcept
{
  const ContestSettings &contest_settings =
    CommonInterface::GetComputerSettings().contest;
  const MapSettings &map_settings = CommonInterface::GetMapSettings();

  contest = contest_settings.contest;
  predict = contest_settings.predict;

  show_fai_triangle_areas = map_settings.show_fai_triangle_areas;
  fai_triangle_threshold = map_settings.fai_triangle_settings.threshold;
  show_95_percent_rule_helpers = map_settings.show_95_percent_rule_helpers;
}

void
ScoringConfigPanel::Fill() noexcept
{
  AddGroup();

  AddEnumItem(_("Contest"),
              _("Select the rules used for calculating optimal points for a contest."),
              contests_list, contest);

  AddToggleItem(_("Predict Contest"),
                _("If enabled, then the next task point is included in the "
                  "score calculation, assuming that you will reach it."),
                predict);

  if (!IsExpert())
    return;

  AddGroup();

  AddToggleItem(_("FAI triangle areas"),
                _("Show FAI triangle areas on the map."),
                show_fai_triangle_areas);

  if (show_fai_triangle_areas)
    AddEnumItem(_("FAI triangle threshold"),
                _("Specifies which threshold is used for \"large\" FAI triangles."),
                fai_triangle_threshold_list, fai_triangle_threshold);

  AddGroup();

  // xgettext:no-c-format
  AddToggleItem(_("95% dist. rule helpers"),
                _("Show helpers for Argentinean Federation \"95% distance\" rule. "
                  "The AAT Distance Around Target InfoBox will show projected "
                  "distance vs. maximum and change colors as you approach 95%."),
                show_95_percent_rule_helpers);
}

bool
ScoringConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ContestSettings &contest_settings =
    CommonInterface::SetComputerSettings().contest;
  MapSettings &map_settings = CommonInterface::SetMapSettings();

  changed |= Profile::Update(ProfileKeys::OLCRules,
                             contest_settings.contest, contest);
  changed |= Profile::Update(ProfileKeys::PredictContest,
                             contest_settings.predict, predict);

  changed |= Profile::Update(ProfileKeys::ShowFAITriangleAreas,
                             map_settings.show_fai_triangle_areas,
                             show_fai_triangle_areas);

  changed |= Profile::Update(ProfileKeys::FAITriangleThreshold,
                             map_settings.fai_triangle_settings.threshold,
                             fai_triangle_threshold);

  changed |= Profile::Update(ProfileKeys::Show95PercentRuleHelpers,
                             map_settings.show_95_percent_rule_helpers,
                             show_95_percent_rule_helpers);

  /* ContestEnumLayout=2 = current Contest encoding (see ContestProfile).
     Only stamp when OLCRules is present — do not add the key to
     untouched profiles.  Rewrite OLCRules so a migrated old NONE
     (stored as 14) is not read as NET_COUPE after the stamp. */
  unsigned contest_enum_layout = 0;
  if (Profile::Exists(ProfileKeys::OLCRules) &&
      (!Profile::Get(ProfileKeys::ContestEnumLayout, contest_enum_layout) ||
       contest_enum_layout < 2U)) {
    Profile::Set(ProfileKeys::ContestEnumLayout, 2U);
    Profile::SetEnum(ProfileKeys::OLCRules, contest_settings.contest);
    changed = true;
  }

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateScoringConfigPanel()
{
  return std::make_unique<ScoringConfigPanel>();
}
