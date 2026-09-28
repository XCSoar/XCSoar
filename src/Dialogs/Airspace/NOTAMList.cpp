// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMList.hpp"
#include "LogFile.hpp"

#ifdef HAVE_HTTP

#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Button.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Language/Language.hpp"
#include "Language/FormatText.hpp"
#include "UIGlobals.hpp"
#include "NetComponents.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "NOTAM/NOTAM.hpp"
#include "NOTAM/Filter.hpp"
#include "NOTAM/NOTAMGlue.hpp"
#include "NOTAM/Converter.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace.hpp"
#include "BackendComponents.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Protection.hpp"
#include "util/StringFormat.hpp"
#include "util/UTF8.hpp"
#include "util/StringAPI.hxx"

#include <algorithm>
#include <ctime>
#include <string>
#include <string_view>

// Use full struct name to avoid collision with AirspaceClass::NOTAM enum
using NOTAMStruct = struct NOTAM;

static void
AppendFilterReason(std::string &text, const char *reason)
{
  if (!text.empty())
    text += ", ";

  text += reason;
}

[[nodiscard]]
static std::string
FormatFilterReasons(const NOTAMStruct &notam,
                    const NOTAMSettings &settings,
                    std::chrono::system_clock::time_point now)
{
  std::string reasons;
  const auto filter_reasons = NOTAMFilter::Evaluate(notam, settings, now);

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::IFR))
    AppendFilterReason(reasons, C_("Status", "IFR-only"));

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::TIME))
    AppendFilterReason(reasons, C_("Status", "not effective"));

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::RADIUS))
    AppendFilterReason(reasons, C_("Status", "radius"));

  if (NOTAMFilter::HasFilterReason(filter_reasons,
                                   NOTAMFilter::FilterReason::QCODE))
    AppendFilterReason(reasons, C_("Setting", "Q-Code"));

  return reasons;
}

// Return UTF-8 text safe for UI, or a placeholder if invalid.
[[nodiscard]]
static std::string
SafeString(const std::string &input)
{
  if (input.empty()) {
    return {};
  }
  if (!ValidateUTF8(input)) {
    return std::string(_("[Invalid text]"));
  }
  return input;
}

[[nodiscard]]
static bool
HasHiddenQCodeToken(std::string_view hidden_list,
                    std::string_view qcode) noexcept
{
  if (qcode.empty())
    return false;

  size_t start = hidden_list.find_first_not_of(" \t,");
  while (start != std::string_view::npos) {
    const size_t end = hidden_list.find_first_of(" \t,", start);
    const size_t token_len = end == std::string_view::npos
      ? hidden_list.size() - start
      : end - start;

    if (token_len == qcode.size() &&
        StringIsEqualIgnoreCase(hidden_list.data() + start,
                                qcode.data(), qcode.size()))
      return true;

    if (end == std::string_view::npos)
      break;

    start = hidden_list.find_first_not_of(" \t,", end + 1);
  }

  return false;
}

[[nodiscard]]
static unsigned
CountNOTAMsWithQCodePrefix(std::string_view qcode) noexcept
{
  if (qcode.empty() || net_components == nullptr ||
      net_components->notam == nullptr)
    return 0;

  unsigned count = 0;
  try {
    const auto snapshot = net_components->notam->GetSnapshot();
    for (const auto &notam : snapshot.notams)
      if (NOTAMFilter::IsQCodeHidden(notam.feature_type, qcode))
        ++count;
  } catch (...) {
    LogError(std::current_exception(), "Failed to count NOTAM Q-code");
  }

  return count;
}

static bool
AddHiddenQCode(NOTAMSettings &settings, std::string_view qcode) noexcept
{
  if (qcode.empty() || HasHiddenQCodeToken(settings.hidden_qcodes, qcode))
    return true;

  const size_t length = settings.hidden_qcodes.length();
  const size_t separator = length > 0 ? 1 : 0;
  if (length + separator + qcode.size() + 1 >
      settings.hidden_qcodes.capacity())
    return false;

  if (separator > 0)
    settings.hidden_qcodes += ' ';

  settings.hidden_qcodes += qcode;
  return true;
}

static bool
RemoveHiddenQCode(NOTAMSettings &settings, std::string_view qcode) noexcept
{
  if (qcode.empty())
    return false;

  StaticString<256> remaining{};
  bool removed = false;
  std::string_view hidden_list = settings.hidden_qcodes;
  size_t start = hidden_list.find_first_not_of(" \t,");
  while (start != std::string_view::npos) {
    const size_t end = hidden_list.find_first_of(" \t,", start);
    const auto token = hidden_list.substr(start, end == std::string_view::npos
                                          ? hidden_list.size() - start
                                          : end - start);
    const bool matches = token.size() <= qcode.size() &&
      StringIsEqualIgnoreCase(token.data(), qcode.data(), token.size());

    if (matches) {
      removed = true;
    } else {
      if (!remaining.empty())
        remaining += ' ';
      remaining += token;
    }

    if (end == std::string_view::npos)
      break;

    start = hidden_list.find_first_not_of(" \t,", end + 1);
  }

  if (removed)
    settings.hidden_qcodes = remaining;

  return removed;
}

static void
ApplyNOTAMFilterSettings(const NOTAMSettings &settings) noexcept
{
  Profile::Set(ProfileKeys::NOTAMHiddenQCodes, settings.hidden_qcodes);

  if (net_components != nullptr && net_components->notam != nullptr)
    net_components->notam->SetSettings(settings);

  if (net_components != nullptr && net_components->notam != nullptr &&
      data_components != nullptr && data_components->airspaces != nullptr) {
    try {
      const ScopeSuspendAllThreads suspend;
      net_components->notam->UpdateAirspaces(*data_components->airspaces);
      if (data_components->terrain != nullptr)
        SetAirspaceGroundLevels(*data_components->airspaces,
                                *data_components->terrain);
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to apply NOTAM Q-code filter");
    }
  }
}


static StaticString<32>
FormatRelativeNotamTime(std::chrono::system_clock::duration duration)
{
  StaticString<32> result;
  const auto minutes =
    std::chrono::duration_cast<std::chrono::minutes>(duration);
  if (minutes.count() < 60) {
    result.Format(_("%dm"), static_cast<int>(minutes.count()));
    return result;
  }

  const auto rounded_hours = (minutes.count() + 59) / 60;
  if (rounded_hours < 48) {
    result.Format(_("%dh"), static_cast<int>(rounded_hours));
    return result;
  }

  result.Format(_("%dd"), static_cast<int>(rounded_hours / 24));
  return result;
}

static std::chrono::system_clock::time_point
GetCurrentNOTAMTimeUTC() noexcept
{
  const auto &basic = CommonInterface::Basic();
  return basic.time_available && basic.date_time_utc.IsDatePlausible()
    ? basic.date_time_utc.ToTimePoint()
    : std::chrono::system_clock::now();
}

/**
 * The NOTAMs which are loaded, below what is known about the
 * download.  An item shows the number, the location and whether the
 * NOTAM is in effect in its first line, the beginning of the text
 * below it, and why the filter hides it, if it does.  A tap on a
 * NOTAM opens its details; the button of the dialog hides or shows
 * the Q-code of the NOTAM under the cursor.
 */
class NOTAMListWidget final : public GroupedListWidget {
  /** how many lines of the text of a NOTAM the item shows */
  static constexpr unsigned TEXT_LINES = 3;

  /** the NOTAMs the items show, in their order */
  std::vector<NOTAMStruct> notams;

  /** the index of the item of the first NOTAM */
  unsigned first_notam_item = 0;

  Button *filter_qcode_button = nullptr;

  /** show the NOTAMs which the filter hides, too? */
  bool show_all = false;

public:
  NOTAMListWidget() noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()) {}

  void SetFilterQCodeButton(Button *_filter_qcode_button) noexcept {
    filter_qcode_button = _filter_qcode_button;
    UpdateButtons(GetCursorIndex());
  }

  void FilterSelectedQCode() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    SetCursorCallback([this](int index){ UpdateButtons(index); });
    Refresh();
    GroupedListWidget::Prepare(parent, rc);
  }

private:
  /** Fill the list from the NOTAMs which are loaded now. */
  void Fill();

  /** Fill() again, and show an empty list if that fails. */
  void Refresh() noexcept;

  void AddNOTAMItem(const NOTAMStruct &notam, const NOTAMSettings &settings,
                    std::chrono::system_clock::time_point now) noexcept;

  [[gnu::pure]]
  const NOTAMStruct *GetSelectedNOTAM(int index) const noexcept {
    return index >= (int)first_notam_item &&
      index < (int)(first_notam_item + notams.size())
      ? &notams[index - first_notam_item]
      : nullptr;
  }

  void UpdateButtons(int index) noexcept;
  void ShowDetails(const NOTAMStruct &notam) noexcept;
};

void
NOTAMListWidget::UpdateButtons(int index) noexcept
{
  if (filter_qcode_button == nullptr)
    return;

  const auto *notam = GetSelectedNOTAM(index);
  const bool has_qcode = notam != nullptr && !notam->feature_type.empty();
  filter_qcode_button->SetEnabled(has_qcode);
  if (has_qcode) {
    const auto &settings =
      CommonInterface::GetComputerSettings().airspace.notam;
    filter_qcode_button->SetCaption(
      NOTAMFilter::IsQCodeHidden(notam->feature_type, settings.hidden_qcodes)
      ? C_("Button", "Show Q-code")
      : C_("Button", "Hide Q-code"));
  }
}

void
NOTAMListWidget::AddNOTAMItem(const NOTAMStruct &notam,
                              const NOTAMSettings &settings,
                              const std::chrono::system_clock::time_point now)
  noexcept
{
  const unsigned i = notams.size();
  notams.push_back(notam);

  /* the badge says whether the NOTAM is in effect */
  StaticString<64> badge;
  BadgeStyle badge_style;
  if (now < notam.start_time) {
    FormatStartsIn(badge,
                   FormatRelativeNotamTime(notam.start_time - now).c_str());
    badge_style = BadgeStyle::PRIMARY;
  } else if (!notam.end_time_permanent && now > notam.end_time) {
    badge = C_("Status", "Expired");
    badge_style = BadgeStyle::DANGER;
  } else {
    badge = _("Active");
    badge_style = BadgeStyle::SUCCESS;
  }

  /* the description: the Q-code and the text, cut to a few lines */
  std::string description;
  if (!notam.feature_type.empty()) {
    description = SafeString(notam.feature_type);
    description += ": ";
  }

  if (!notam.text.empty()) {
    std::string text = SafeString(notam.text);
    for (auto &ch : text)
      if (ch == '\n' || ch == '\r')
        ch = ' ';

    description += text;
  }

  /* the second line says why the filter hides the NOTAM */
  std::string subtitle;
  if (!NOTAMFilter::ShouldDisplay(notam, settings, now, false)) {
    subtitle = C_("Status", "Filtered");

    const auto reasons = FormatFilterReasons(notam, settings, now);
    if (!reasons.empty()) {
      subtitle += ": ";
      subtitle += reasons;
    }
  }

  const std::string caption =
    SafeString(notam.number.empty() ? notam.id : notam.number);
  const std::string location = SafeString(notam.location);

  AddItem(caption.c_str(), [this, i](){
    ShowDetails(notams[i]);
  }, {.subtitle = subtitle.empty() ? nullptr : subtitle.c_str(),
      .value = location.empty() ? nullptr : location.c_str(),
      .value_font = TextFont::MONO,
      .description = description.empty() ? nullptr : description.c_str(),
      .description_font = TextFont::MONO,
      .description_size = TextSize::SMALL,
      .description_max_lines = TEXT_LINES,
      .badge = badge.c_str(),
      .badge_style = badge_style,
      .chevron = true});
}

void
NOTAMListWidget::Fill()
{
  Clear();
  notams.clear();

  NOTAMGlue::Snapshot snapshot;
  if (net_components != nullptr && net_components->notam != nullptr)
    snapshot = net_components->notam->GetSnapshot();
  else
    LogFmt("NOTAM: UpdateList - net_components or notam is null");

  const auto &all = snapshot.notams;
  const auto now = GetCurrentNOTAMTimeUTC();
  const auto &settings =
    CommonInterface::GetComputerSettings().airspace.notam;
  const unsigned visible_count =
    static_cast<unsigned>(std::count_if(all.begin(), all.end(),
      [&](const auto &notam) {
        return NOTAMFilter::ShouldDisplay(notam, settings, now, false);
      }));

  /* what is known about the download */
  AddGroup();

  StaticString<64> value;
  const std::time_t last_update = snapshot.last_update_time;
  if (last_update > 0) {
    struct tm tm_buf;
#ifdef _WIN32
    const auto *tm =
      (localtime_s(&tm_buf, &last_update) == 0) ? &tm_buf : nullptr;
#else
    const auto *tm = localtime_r(&last_update, &tm_buf);
#endif
    if (tm != nullptr)
      value.Format("%04d-%02d-%02d %02d:%02d",
                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                   tm->tm_hour, tm->tm_min);
    else
      value = _("Unknown");
  } else
    value = C_("Status", "Never");

  AddItem(_("Last Update (local)"), {.value = value.c_str()});

  const auto &basic = CommonInterface::Basic();
  const GeoPoint last_loc = snapshot.last_update_location;
  if (basic.location_available && basic.location.IsValid() &&
      last_loc.IsValid())
    value = FormatUserDistanceSmart(basic.location.Distance(last_loc)).c_str();
  else
    value = _("Unknown");

  AddItem(_("Distance"), {.value = value.c_str()});

  value.Format(_("%u total"), static_cast<unsigned>(all.size()));
  AddItem(C_("Menu", "NOTAMs"), {.value = value.c_str()});

  value.Format(_("%u visible"), visible_count);
  AddItem(_("After Filtering"), {.value = value.c_str()});

  const unsigned show_all_item = GetItemCount();
  AddItem(C_("Button", "Show All"), [this, show_all_item](){
    show_all = IsItemChecked(show_all_item);
    Refresh();
  }, {.toggle = true,
      .checked = show_all,
      .help = _("List the NOTAMs which the filter hides, too, and say why they are hidden.")});

  /* the NOTAMs */
  AddGroup();
  first_notam_item = GetItemCount();

  for (const auto &notam : all)
    if (show_all || NOTAMFilter::ShouldDisplay(notam, settings, now, false))
      AddNOTAMItem(notam, settings, now);

  if (notams.empty())
    AddItem(_("No NOTAMs available"), {.disabled = true});

  UpdateLayout();
}

void
NOTAMListWidget::Refresh() noexcept
{
  try {
    Fill();
  } catch (...) {
    LogError(std::current_exception(), "Failed to update NOTAM list");

    Clear();
    notams.clear();
    AddGroup();
    first_notam_item = GetItemCount();
    AddItem(_("No NOTAMs available"), {.disabled = true});
    UpdateLayout();
  }

  UpdateButtons(GetCursorIndex());
}

void
NOTAMListWidget::ShowDetails(const NOTAMStruct &notam) noexcept
{
  try {
    auto airspace = NOTAMConverter::BuildNOTAMAirspace(notam, true);
    if (!airspace)
      return;

    dlgAirspaceDetails(std::move(airspace),
                       backend_components != nullptr
                       ? backend_components->GetAirspaceWarnings()
                       : nullptr);
  } catch (...) {
    LogError(std::current_exception(), "Failed to show NOTAM details");
  }
}

void
NOTAMListWidget::FilterSelectedQCode() noexcept
{
  const auto *notam = GetSelectedNOTAM(GetCursorIndex());
  if (notam == nullptr || notam->feature_type.empty())
    return;

  const auto qcode = std::string_view{notam->feature_type};
  auto &settings = CommonInterface::SetComputerSettings().airspace.notam;

  if (NOTAMFilter::IsQCodeHidden(qcode, settings.hidden_qcodes)) {
    StaticString<512> message;
    message.Format(_("Show Q-code %s?\n"
                     "This will remove matching Q-code filters, which may "
                     "also show other NOTAMs."),
                   notam->feature_type.c_str());

    if (ShowMessageBox(message.c_str(), C_("Menu", "NOTAM"),
                       MB_YESNO | MB_ICONQUESTION) != IDYES)
      return;

    if (!RemoveHiddenQCode(settings, qcode))
      return;

    ApplyNOTAMFilterSettings(settings);
    Refresh();
    return;
  }

  const unsigned affected_count = CountNOTAMsWithQCodePrefix(qcode);
  StaticString<512> message;
  message.Format(_("Hide Q-code %s?\n"
                   "This will hide %u currently loaded NOTAM(s) with a "
                   "matching Q-code prefix from the map and mark them as "
                   "filtered in the NOTAM list."),
                 notam->feature_type.c_str(), affected_count);

  if (ShowMessageBox(message.c_str(), C_("Menu", "NOTAM"),
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  if (!AddHiddenQCode(settings, qcode)) {
    ShowMessageBox(_("The hidden Q-code list is full. Remove unused filters "
                     "in NOTAM settings first."),
                   C_("Menu", "NOTAM"), MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  ApplyNOTAMFilterSettings(settings);
  Refresh();
}

void
ShowNOTAMListDialog(UI::SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto list_widget = std::make_unique<NOTAMListWidget>();
  NOTAMListWidget *const list = list_widget.get();
  WidgetDialog dialog(WidgetDialog::Full{}, parent, look,
                      C_("Menu", "NOTAMs"));
  dialog.FinishPreliminary(std::move(list_widget));
  list->SetFilterQCodeButton(
    dialog.AddButton(C_("Button", "Hide Q-code"),
                     [list](){ list->FilterSelectedQCode(); }));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}

#else // !HAVE_HTTP

void
ShowNOTAMListDialog(UI::SingleWindow &)
{
  // NOTAM support not compiled in - do nothing
  LogFormat("NOTAM: ShowNOTAMListDialog called, but NOTAM support is not "
            "available");
}

#endif
