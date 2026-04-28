// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FrequencyBrowserDialog.hpp"
#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TabWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Look/DialogLook.hpp"
#include "Look/Look.hpp"
#include "Look/MapLook.hpp"
#include "UIGlobals.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Language/Language.hpp"
#include "ActionInterface.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "system/Path.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Airspace/ActivePredicate.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "FLARM/Details.hpp"
#include "Interface.hpp"
#include "MapSettings.hpp"
#include "Engine/Airspace/Ptr.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "FLARM/Color.hpp"
#include "FLARM/Friends.hpp"
#include "FLARM/Id.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Renderer/WaypointIconRenderer.hpp"
#include "Renderer/TrafficRenderer.hpp"
#include "XML/Parser.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Node.hpp"
#include "Dialogs/Message.hpp"
#include "UtilsSettings.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "RadioFrequency.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Icon.hpp"
#include "Resources.hpp"
#include "util/Macros.hpp"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"
#include "Formatter/UserUnits.hpp"
#include "ui/event/KeyCode.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>
#include <string>

namespace {

constexpr double NEAREST_RANGE_METERS = 100000;

class FrequencyBrowserWidget;

struct FrequencyEntry {
  enum class Kind : uint8_t {
    AIRSPACE,
    WAYPOINT,
    TRAFFIC,
    FILE,
  };

  Kind kind;
  double distance_m;
  std::string title;
  std::string subtitle;
  RadioFrequency frequency;
  unsigned squawk = 0;
  uint8_t file_type_icon = 0;

  /** Nearest tab: map-style row icons (unused for FILE / favorites). */
  AirspacePtr airspace;
  WaypointPtr waypoint;
  FlarmId traffic_id{};
  FlarmColor traffic_color = FlarmColor::NONE;
};

[[gnu::const]]
static const char *
KindToXmlString(FrequencyEntry::Kind k) noexcept
{
  switch (k) {
  case FrequencyEntry::Kind::AIRSPACE:
    return "airspace";
  case FrequencyEntry::Kind::WAYPOINT:
    return "waypoint";
  case FrequencyEntry::Kind::TRAFFIC:
    return "traffic";
  case FrequencyEntry::Kind::FILE:
    return nullptr;
  }

  return nullptr;
}

static FrequencyEntry::Kind
KindFromXmlString(const char *s) noexcept
{
  if (s == nullptr)
    return FrequencyEntry::Kind::FILE;
  if (StringIsEqual(s, "airspace"))
    return FrequencyEntry::Kind::AIRSPACE;
  if (StringIsEqual(s, "waypoint"))
    return FrequencyEntry::Kind::WAYPOINT;
  if (StringIsEqual(s, "traffic"))
    return FrequencyEntry::Kind::TRAFFIC;
  return FrequencyEntry::Kind::FILE;
}

static void
WriteKindAttributes(XMLNode &st, const FrequencyEntry &e) noexcept
{
  const char *ks = KindToXmlString(e.kind);
  if (ks != nullptr)
    st.AddAttribute("kind", ks);

  if (e.kind == FrequencyEntry::Kind::TRAFFIC && e.traffic_id.IsDefined()) {
    char idbuf[20];
    if (e.traffic_id.Format(idbuf) != nullptr)
      st.AddAttribute("flarm_id", idbuf);
  }
}

/**
 * Write all attributes for one Station node in the favorites XML file.
 */
static void
WriteStationAttributes(XMLNode &st, const FrequencyEntry &e) noexcept
{
  st.AddAttribute("name", e.title);
  char freqbuf[24];
  e.frequency.Format(freqbuf, ARRAY_SIZE(freqbuf));
  st.AddAttribute("frequency", freqbuf);
  WriteKindAttributes(st, e);
  if (!e.subtitle.empty())
    st.AddAttribute("comment", e.subtitle);
  if (e.squawk > 0) {
    StaticString<16> sq;
    sq.Format("%u", e.squawk);
    st.AddAttribute("squawk", sq.c_str());
  }
  if (e.file_type_icon != 0) {
    StaticString<8> t;
    t.Format("%u", unsigned(e.file_type_icon));
    st.AddAttribute("type", t.c_str());
  }
}

/**
 * After loading favorites from XML, attach live airspace / waypoint pointers
 * and FLARM colors so list icons match the Nearest tab.
 */
static void
ResolveFavoriteEntry(FrequencyEntry &e) noexcept
{
  switch (e.kind) {
  case FrequencyEntry::Kind::FILE:
    return;

  case FrequencyEntry::Kind::AIRSPACE:
    if (e.airspace != nullptr)
      return;
    if (data_components == nullptr || data_components->airspaces == nullptr ||
        !e.frequency.IsDefined())
      return;
    for (const auto &i : data_components->airspaces->QueryAll()) {
      const AbstractAirspace &as = i.GetAirspace();
      if (as.GetRadioFrequency() != e.frequency)
        continue;
      if (!StringIsEqual(as.GetName(), e.title.c_str()))
        continue;
      e.airspace = i.GetAirspacePtr();
      return;
    }
    return;

  case FrequencyEntry::Kind::WAYPOINT:
    if (e.waypoint != nullptr)
      return;
    if (data_components == nullptr || data_components->waypoints == nullptr ||
        !e.frequency.IsDefined())
      return;
    {
      const WaypointPtr wp =
        data_components->waypoints->LookupName(e.title);
      if (wp != nullptr && wp->radio_frequency == e.frequency)
        e.waypoint = wp;
    }
    return;

  case FrequencyEntry::Kind::TRAFFIC:
    e.traffic_color = FlarmFriends::GetFriendColor(e.traffic_id);
    return;
  }
}

[[gnu::pure]]
static bool
FavoritesXmlContainsFrequency(const XMLNode &root,
                              RadioFrequency needle) noexcept
{
  if (!needle.IsDefined())
    return false;

  for (const XMLNode &node : root) {
    if (!StringIsEqual(node.GetName(), "Station"))
      continue;
    const char *fs = node.GetAttribute("frequency");
    if (fs == nullptr)
      continue;
    if (RadioFrequency::Parse(fs) == needle)
      return true;
  }

  return false;
}

static void
WriteFrequencyListFile(const XMLNode &doc, Path path)
{
  FileOutputStream file(path);
  BufferedOutputStream buffered(file);
  doc.Serialise(buffered, true);
  buffered.Flush();
  file.Sync();
  file.Commit();
}

enum class SaveFavoriteResult : uint8_t {
  SUCCESS,
  NO_PATH,
  DUPLICATE,
  BAD_FILE,
  IO_ERROR,
};

static SaveFavoriteResult
SaveNewFavoriteStation(const FrequencyEntry &e) noexcept
{
  const auto path = Profile::GetPath(ProfileKeys::FrequenciesFile);
  if (path == nullptr)
    return SaveFavoriteResult::NO_PATH;

  try {
    XMLNode doc = [&path]() -> XMLNode {
      try {
        return XML::ParseFile(path);
      } catch (...) {
        return XMLNode::CreateRoot("FrequencyList");
      }
    }();

    if (!StringIsEqual(doc.GetName(), "FrequencyList"))
      return SaveFavoriteResult::BAD_FILE;

    if (FavoritesXmlContainsFrequency(doc, e.frequency))
      return SaveFavoriteResult::DUPLICATE;

    XMLNode &st = doc.AddChild("Station");
    WriteStationAttributes(st, e);

    WriteFrequencyListFile(doc, path);
    return SaveFavoriteResult::SUCCESS;
  } catch (...) {
    return SaveFavoriteResult::IO_ERROR;
  }
}

/**
 * Rewrites the whole frequency database from the in-memory favorites list.
 */
static SaveFavoriteResult
WriteFavoritesVectorToFile(const std::vector<FrequencyEntry> &v) noexcept
{
  const auto path = Profile::GetPath(ProfileKeys::FrequenciesFile);
  if (path == nullptr)
    return SaveFavoriteResult::NO_PATH;

  try {
    XMLNode doc = XMLNode::CreateRoot("FrequencyList");
    for (const auto &e : v) {
      if (!e.frequency.IsDefined())
        continue;

      XMLNode &st = doc.AddChild("Station");
      WriteStationAttributes(st, e);
    }

    WriteFrequencyListFile(doc, path);
    return SaveFavoriteResult::SUCCESS;
  } catch (...) {
    return SaveFavoriteResult::IO_ERROR;
  }
}

static bool
LoadFrequenciesFromFile(std::vector<FrequencyEntry> &out) noexcept
{
  const auto path = Profile::GetPath(ProfileKeys::FrequenciesFile);
  if (path == nullptr)
    return false;

  try {
    const auto xml_root = XML::ParseFile(path);
    const ConstDataNodeXML root(xml_root);

    if (!StringIsEqual(root.GetName(), "FrequencyList"))
      return false;

    const auto children = root.ListChildrenNamed("Station");
    for (const auto &i : children) {
      const char *name = i->GetAttribute("name");
      if (name == nullptr)
        continue;

      FrequencyEntry e;
      e.kind = FrequencyEntry::Kind::FILE;
      e.distance_m = 0;
      e.title = name;

      const char *frequency = i->GetAttribute("frequency");
      if (frequency != nullptr)
        e.frequency = RadioFrequency::Parse(frequency);

      const char *squawk = i->GetAttribute("squawk");
      if (squawk != nullptr)
        e.squawk = ParseUnsigned(squawk);

      const char *type = i->GetAttribute("type");
      if (type != nullptr)
        e.file_type_icon = static_cast<uint8_t>(ParseUnsigned(type));

      const char *comment = i->GetAttribute("comment");
      if (comment != nullptr)
        e.subtitle = comment;

      e.kind = KindFromXmlString(i->GetAttribute("kind"));

      const char *flarm = i->GetAttribute("flarm_id");
      if (flarm != nullptr) {
        char *endptr = nullptr;
        e.traffic_id = FlarmId::Parse(flarm, &endptr);
      }

      out.push_back(std::move(e));
    }
  } catch (...) {
    return false;
  }

  return !out.empty();
}

static void
BuildNearestList(std::vector<FrequencyEntry> &out) noexcept
{
  out.clear();

  const auto &basic = CommonInterface::Basic();
  if (!basic.location_available)
    return;

  const GeoPoint loc = basic.location;
  auto *warnings = backend_components != nullptr
    ? backend_components->GetAirspaceWarnings()
    : nullptr;
  const ActiveAirspacePredicate active_pred(warnings);

  if (data_components != nullptr && data_components->airspaces != nullptr) {
    const auto &airspaces = *data_components->airspaces;
    const FlatProjection &proj = airspaces.GetProjection();

    for (const auto &i : airspaces.QueryWithinRange(loc, NEAREST_RANGE_METERS)) {
      const AbstractAirspace &as = i.GetAirspace();
      if (!as.GetRadioFrequency().IsDefined())
        continue;
      if (!active_pred(as))
        continue;

      const double d = as.Inside(loc)
        ? 0
        : loc.DistanceS(as.ClosestPoint(loc, proj));

      FrequencyEntry e;
      e.kind = FrequencyEntry::Kind::AIRSPACE;
      e.distance_m = d;
      e.title = as.GetName();
      const char *st = as.GetStationName();
      if (st != nullptr && st[0] != '\0')
        e.subtitle = st;
      else
        e.subtitle = _("Airspace");
      e.frequency = as.GetRadioFrequency();
      e.airspace = i.GetAirspacePtr();
      out.push_back(std::move(e));
    }
  }

  if (data_components != nullptr && data_components->waypoints != nullptr) {
    data_components->waypoints->VisitWithinRange(
      loc, NEAREST_RANGE_METERS,
      [&out, loc](const WaypointPtr &wp) {
        if (!wp->radio_frequency.IsDefined())
          return;

        FrequencyEntry e;
        e.kind = FrequencyEntry::Kind::WAYPOINT;
        e.distance_m = loc.DistanceS(wp->location);
        e.title = wp->name;
        if (!wp->comment.empty())
          e.subtitle = wp->comment;
        else
          e.subtitle = _("Waypoint");
        e.frequency = wp->radio_frequency;
        e.waypoint = wp;
        out.push_back(std::move(e));
      });
  }

  for (const auto &traffic : basic.flarm.traffic.list) {
    if (!traffic.location_available)
      continue;

    const auto info = FlarmDetails::ResolveInfo(traffic.id);
    if (!info.frequency.IsDefined())
      continue;

    FrequencyEntry e;
    e.kind = FrequencyEntry::Kind::TRAFFIC;
    e.distance_m = loc.DistanceS(traffic.location);
    if (traffic.HasName())
      e.title = traffic.name.c_str();
    else {
      char idbuf[16];
      e.title = traffic.id.Format(idbuf) != nullptr ? idbuf : _("Traffic");
    }
    e.subtitle = _("Traffic");
    e.frequency = info.frequency;
    e.traffic_id = traffic.id;
    e.traffic_color = FlarmFriends::GetFriendColor(traffic.id);
    out.push_back(std::move(e));
  }

  std::sort(out.begin(), out.end(),
            [](const FrequencyEntry &a, const FrequencyEntry &b) {
              if (a.distance_m != b.distance_m)
                return a.distance_m < b.distance_m;
              return a.title < b.title;
            });
}

class FrequencyListTabWidget final : public ListWidget {
  const DialogLook &dialog_look;
  TwoTextRowsRenderer row_renderer;
  std::vector<FrequencyEntry> *const entries;
  FrequencyBrowserWidget *const browser;

public:
  FrequencyListTabWidget(const DialogLook &_look,
                         std::vector<FrequencyEntry> *_entries,
                         FrequencyBrowserWidget &_browser) noexcept
    : dialog_look(_look), entries(_entries), browser(&_browser) {}

  void RefreshList() noexcept {
    GetList().SetLength(entries->size());
    GetList().Invalidate();
  }

  void RefreshListAfterRemove(unsigned removed_index) noexcept {
    const unsigned new_size = entries->size();
    GetList().SetLength(new_size);
    if (new_size > 0) {
      const unsigned ni = removed_index >= new_size ? new_size - 1 : removed_index;
      GetList().SetCursorIndex(ni);
    }
    GetList().Invalidate();
  }

  [[gnu::pure]]
  unsigned GetCursorIndex() const noexcept {
    return GetList().GetCursorIndex();
  }

  const FrequencyEntry *GetSelected() const noexcept {
    const unsigned i = GetCursorIndex();
    return i < entries->size() ? &(*entries)[i] : nullptr;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    CreateList(parent, dialog_look, rc,
               row_renderer.CalculateLayout(*dialog_look.list.font_bold,
                                            dialog_look.small_font));
    RefreshList();
  }

  void OnPaintItem(Canvas &canvas, PixelRect rc, unsigned index) noexcept override {
    assert(index < entries->size());
    const FrequencyEntry &e = (*entries)[index];

    const unsigned padding = Layout::GetTextPadding();
    const unsigned line_height = rc.GetHeight();
    const PixelPoint pt(rc.left + line_height / 2, rc.top + line_height / 2);

    switch (e.kind) {
    case FrequencyEntry::Kind::AIRSPACE:
      if (e.airspace != nullptr) {
        const unsigned radius = line_height / 2 - padding;
        AirspacePreviewRenderer::Draw(
          canvas, *e.airspace, pt, radius,
          CommonInterface::GetMapSettings().airspace,
          UIGlobals::GetMapLook().airspace);
      } else {
        MaskedIcon icon;
        icon.LoadResource(IDB_BEACON, IDB_BEACON_HD);
        icon.Draw(canvas, pt);
      }
      break;
    case FrequencyEntry::Kind::WAYPOINT:
      if (e.waypoint != nullptr) {
        const unsigned icon_size =
          line_height > 4 * padding ? line_height - 4 * padding : 0;
        WaypointIconRenderer wir(CommonInterface::GetMapSettings().waypoint,
                                 UIGlobals::GetMapLook().waypoint, canvas);
        wir.SetIconSize(icon_size);
        wir.Draw(*e.waypoint, pt);
      } else {
        MaskedIcon icon;
        icon.LoadResource(IDB_PLANE, IDB_PLANE_HD);
        icon.Draw(canvas, pt);
      }
      break;
    case FrequencyEntry::Kind::TRAFFIC: {
      const auto *traffic =
        CommonInterface::Basic().flarm.traffic.FindTraffic(e.traffic_id);
      if (traffic != nullptr)
        TrafficRenderer::Draw(canvas, UIGlobals::GetLook().traffic, false,
                              *traffic, traffic->track, e.traffic_color, pt);
      else {
        MaskedIcon icon;
        icon.LoadResource(IDB_PLANE, IDB_PLANE_HD);
        icon.Draw(canvas, pt);
      }
      break;
    }
    case FrequencyEntry::Kind::FILE: {
      MaskedIcon icon;
      if (e.file_type_icon == 1)
        icon.LoadResource(IDB_PLANE, IDB_PLANE_HD);
      else if (e.file_type_icon == 2)
        icon.LoadResource(IDB_CLOUD, IDB_CLOUD_HD);
      else if (e.file_type_icon == 3)
        icon.LoadResource(IDB_BEACON, IDB_BEACON_HD);
      else
        icon.LoadResource(IDB_RADIO, IDB_RADIO_HD);
      icon.Draw(canvas, pt);
      break;
    }
    }

    rc.left += line_height + padding;

    row_renderer.DrawFirstRow(canvas, rc, e.title.c_str());
    row_renderer.DrawSecondRow(canvas, rc, e.subtitle.c_str());

    if (e.frequency.IsDefined()) {
      StaticString<32> buffer;
      char radio[20];
      e.frequency.Format(radio, ARRAY_SIZE(radio));
      buffer.Format("%s MHz", radio);
      row_renderer.DrawRightFirstRow(canvas, rc, buffer);
    }

    if (e.kind != FrequencyEntry::Kind::FILE && e.distance_m > 0.5) {
      StaticString<64> dist;
      char dbuf[32];
      FormatUserDistanceSmart(e.distance_m, dbuf);
      dist.Format("%s: %s", _("Distance"), dbuf);
      row_renderer.DrawRightSecondRow(canvas, rc, dist);
    } else if (e.kind == FrequencyEntry::Kind::FILE && e.squawk > 0) {
      StaticString<32> sq;
      sq.Format("Squawk %u", e.squawk);
      row_renderer.DrawRightSecondRow(canvas, rc, sq);
    }
  }

  bool CanActivateItem(unsigned) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned) noexcept override;

  void OnCursorMoved(unsigned) noexcept override;
};

class ManualFrequencyWidget final : public RowFormWidget {
  enum Rows : unsigned { FREQ, NAME };

public:
  explicit ManualFrequencyWidget(const DialogLook &look) noexcept
    :RowFormWidget(look) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    AddText(_("Frequency (MHz)"), nullptr, "", nullptr);
    AddText(_("Name"), nullptr, "", nullptr);
    RowFormWidget::Prepare(parent, rc);
  }

  [[gnu::pure]]
  RadioFrequency GetParsedFrequency() const noexcept {
    return RadioFrequency::Parse(GetValueString(FREQ));
  }

  [[gnu::pure]]
  const char *GetStationName() const noexcept {
    return GetValueString(NAME);
  }
};

static constexpr StaticEnumChoice add_favorite_icon_choices[] = {
  {0u, N_("Radio"), nullptr},
  {1u, N_("Airplane"), nullptr},
  {2u, N_("Cloud"), nullptr},
  {3u, N_("Tower / beacon"), nullptr},
  nullptr,
};

class AddFavoriteEntryWidget final : public RowFormWidget {
  enum Rows : unsigned { FREQ, NAME, ICON };

public:
  explicit AddFavoriteEntryWidget(const DialogLook &look) noexcept
    : RowFormWidget(look) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    AddText(_("Frequency (MHz)"), nullptr, "", nullptr);
    AddText(_("Name"), nullptr, "", nullptr);
    AddEnum(_("Icon"), nullptr, add_favorite_icon_choices, 0);
    RowFormWidget::Prepare(parent, rc);
  }

  [[gnu::pure]]
  RadioFrequency GetParsedFrequency() const noexcept {
    return RadioFrequency::Parse(GetValueString(FREQ));
  }

  [[gnu::pure]]
  const char *GetStationName() const noexcept {
    return GetValueString(NAME);
  }

  [[gnu::pure]]
  uint8_t GetFileTypeIcon() const noexcept {
    return static_cast<uint8_t>(GetValueEnum(ICON));
  }
};

class FrequencyBrowserWidget final : public TabWidget {
  static constexpr unsigned TAB_NEAREST = 0;
  static constexpr unsigned TAB_FAVORITES = 1;
  static constexpr unsigned TAB_MANUAL = 2;

  WndForm *form = nullptr;
  std::vector<FrequencyEntry> nearest, favorites;
  FrequencyListTabWidget *nearest_list = nullptr;
  FrequencyListTabWidget *favorites_list = nullptr;
  ManualFrequencyWidget *manual = nullptr;
  Button *standby_button = nullptr;
  Button *active_button = nullptr;
  Button *favorites_tool_button = nullptr;
  Button *add_favorite_entry_button = nullptr;

public:
  explicit FrequencyBrowserWidget(Orientation o,
                                  std::unique_ptr<Widget> &&extra) noexcept
    : TabWidget(o, std::move(extra)) {
    BuildNearestList(nearest);
    LoadFrequenciesFromFile(favorites);
    for (auto &fe : favorites)
      ResolveFavoriteEntry(fe);
  }

  void CreateButtons(WidgetDialog &dialog,
                     FrequencyBrowserPrimary primary) noexcept;

  void AddTabs(const DialogLook &look);

  void UpdateActionButtons() noexcept;

  void HandleListActivate() noexcept { (void)ApplyListStandby(); }

private:
  bool KeyPress(unsigned key_code) noexcept override;

  void CloseAfterApply(bool applied) noexcept;
  bool ApplyListStandby() noexcept;
  bool ApplyListActive() noexcept;
  bool ApplyManualStandby() noexcept;
  bool ApplyManualActive() noexcept;
  void ApplyAddToFavorites() noexcept;
  void ApplyRemoveFavorite() noexcept;
  void ApplyAddFavoriteEntry() noexcept;
};

bool
FrequencyBrowserWidget::KeyPress(unsigned key_code) noexcept
{
  if (GetSize() >= 2 && TabDisplayHasFocus()) {
    switch (key_code) {
    case KEY_LEFT:
      if (Previous(true))
        return true;
      break;
    case KEY_RIGHT:
      if (Next(true))
        return true;
      break;
    default:
      break;
    }
  }

  return TabWidget::KeyPress(key_code);
}

void
FrequencyBrowserWidget::CloseAfterApply(bool applied) noexcept
{
  if (!applied || form == nullptr)
    return;
  /* Bypass WidgetDialog::SetModalResult so TabWidget::Save is not run. */
  form->WndForm::SetModalResult(mrOK);
}

bool
FrequencyBrowserWidget::ApplyListStandby() noexcept
{
  const unsigned tab = GetCurrentIndex();
  const FrequencyEntry *e = nullptr;
  if (tab == TAB_NEAREST)
    e = nearest_list != nullptr ? nearest_list->GetSelected() : nullptr;
  else if (tab == TAB_FAVORITES)
    e = favorites_list != nullptr ? favorites_list->GetSelected() : nullptr;

  if (e == nullptr || !e->frequency.IsDefined())
    return false;
  ActionInterface::SetStandbyFrequency(e->frequency, e->title.c_str());
  return true;
}

bool
FrequencyBrowserWidget::ApplyListActive() noexcept
{
  const unsigned tab = GetCurrentIndex();
  const FrequencyEntry *e = nullptr;
  if (tab == TAB_NEAREST)
    e = nearest_list != nullptr ? nearest_list->GetSelected() : nullptr;
  else if (tab == TAB_FAVORITES)
    e = favorites_list != nullptr ? favorites_list->GetSelected() : nullptr;

  if (e == nullptr || !e->frequency.IsDefined())
    return false;
  ActionInterface::SetActiveFrequency(e->frequency, e->title.c_str());
  return true;
}

bool
FrequencyBrowserWidget::ApplyManualStandby() noexcept
{
  if (manual == nullptr)
    return false;
  const RadioFrequency f = manual->GetParsedFrequency();
  if (!f.IsDefined())
    return false;
  const char *n = manual->GetStationName();
  if (n != nullptr && n[0] == '\0')
    n = nullptr;
  ActionInterface::SetStandbyFrequency(f, n);
  return true;
}

bool
FrequencyBrowserWidget::ApplyManualActive() noexcept
{
  if (manual == nullptr)
    return false;
  const RadioFrequency f = manual->GetParsedFrequency();
  if (!f.IsDefined())
    return false;
  const char *n = manual->GetStationName();
  if (n != nullptr && n[0] == '\0')
    n = nullptr;
  ActionInterface::SetActiveFrequency(f, n);
  return true;
}

void
FrequencyBrowserWidget::ApplyAddToFavorites() noexcept
{
  if (nearest_list == nullptr)
    return;

  if (GetCurrentIndex() != TAB_NEAREST)
    return;

  const FrequencyEntry *e = nearest_list->GetSelected();
  if (e == nullptr || !e->frequency.IsDefined())
    return;

  for (const auto &f : favorites) {
    if (f.frequency == e->frequency) {
      ShowMessageBox(_("This frequency is already in favorites."),
                     _("Frequencies"), MB_OK);
      return;
    }
  }

  const SaveFavoriteResult r = SaveNewFavoriteStation(*e);
  switch (r) {
  case SaveFavoriteResult::SUCCESS:
    break;
  case SaveFavoriteResult::NO_PATH:
    ShowMessageBox(
      _("No radio frequency database is configured.\n"
        "Set \"Radio Frequency Database\" in Site settings, Files."),
      _("Frequencies"), MB_OK);
    return;
  case SaveFavoriteResult::DUPLICATE:
    ShowMessageBox(_("This frequency is already in favorites."),
                   _("Frequencies"), MB_OK);
    return;
  case SaveFavoriteResult::BAD_FILE:
  case SaveFavoriteResult::IO_ERROR:
    ShowMessageBox(_("Could not save the favorites file."),
                   _("Frequencies"), MB_OK);
    return;
  }

  FrequencyEntry copy = *e;
  copy.distance_m = 0;
  favorites.push_back(std::move(copy));
  if (favorites_list != nullptr)
    favorites_list->RefreshList();
  FrequenciesFileChanged = true;
  UpdateActionButtons();
}

void
FrequencyBrowserWidget::ApplyRemoveFavorite() noexcept
{
  if (favorites_list == nullptr)
    return;

  if (GetCurrentIndex() != TAB_FAVORITES)
    return;

  const unsigned idx = favorites_list->GetCursorIndex();
  if (idx >= favorites.size())
    return;

  std::vector<FrequencyEntry> next = favorites;
  next.erase(next.begin() + idx);

  const SaveFavoriteResult r = WriteFavoritesVectorToFile(next);
  if (r != SaveFavoriteResult::SUCCESS) {
    ShowMessageBox(_("Could not save the favorites file."),
                   _("Frequencies"), MB_OK);
    return;
  }

  favorites = std::move(next);
  favorites_list->RefreshListAfterRemove(idx);
  FrequenciesFileChanged = true;
  UpdateActionButtons();
}

void
FrequencyBrowserWidget::ApplyAddFavoriteEntry() noexcept
{
  if (GetCurrentIndex() != TAB_FAVORITES || favorites_list == nullptr)
    return;

  const auto path = Profile::GetPath(ProfileKeys::FrequenciesFile);
  if (path == nullptr) {
    ShowMessageBox(
      _("No radio frequency database is configured.\n"
        "Set \"Radio Frequency Database\" in Site settings, Files."),
      _("Frequencies"), MB_OK);
    return;
  }

  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<AddFavoriteEntryWidget> editor(
    WidgetDialog::Auto{}, UIGlobals::GetMainWindow(), look, _("Add favorite"));
  editor.SetWidget(look);
  editor.AddButton(_("Add"), mrOK);
  editor.AddButton(_("Cancel"), mrCancel);

  if (editor.ShowModal() != mrOK)
    return;

  AddFavoriteEntryWidget &w = editor.GetWidget();
  const RadioFrequency f = w.GetParsedFrequency();
  if (!f.IsDefined()) {
    ShowMessageBox(_("Please enter a valid frequency."),
                   _("Frequencies"), MB_OK);
    return;
  }

  const char *name_c = w.GetStationName();
  std::string title;
  if (name_c != nullptr && name_c[0] != '\0')
    title.assign(name_c);
  else
    title = _("Station");

  for (const auto &ex : favorites) {
    if (ex.frequency == f) {
      ShowMessageBox(_("This frequency is already in favorites."),
                     _("Frequencies"), MB_OK);
      return;
    }
  }

  FrequencyEntry e;
  e.kind = FrequencyEntry::Kind::FILE;
  e.distance_m = 0;
  e.title = std::move(title);
  e.frequency = f;
  e.file_type_icon = w.GetFileTypeIcon();

  const SaveFavoriteResult r = SaveNewFavoriteStation(e);
  switch (r) {
  case SaveFavoriteResult::SUCCESS:
    break;
  case SaveFavoriteResult::DUPLICATE:
    ShowMessageBox(_("This frequency is already in favorites."),
                   _("Frequencies"), MB_OK);
    return;
  case SaveFavoriteResult::BAD_FILE:
  case SaveFavoriteResult::IO_ERROR:
    ShowMessageBox(_("Could not save the favorites file."),
                   _("Frequencies"), MB_OK);
    return;
  case SaveFavoriteResult::NO_PATH:
    ShowMessageBox(
      _("No radio frequency database is configured.\n"
        "Set \"Radio Frequency Database\" in Site settings, Files."),
      _("Frequencies"), MB_OK);
    return;
  }

  favorites.push_back(std::move(e));
  favorites_list->RefreshList();
  FrequenciesFileChanged = true;
  UpdateActionButtons();
}

void
FrequencyBrowserWidget::CreateButtons(WidgetDialog &dialog,
                                      FrequencyBrowserPrimary primary) noexcept
{
  form = &dialog;

  const auto add_standby = [&]() noexcept {
    standby_button = dialog.AddButton(
      _("Set Standby Frequency"),
      [this]() noexcept {
        const bool ok = GetCurrentIndex() == TAB_MANUAL ? ApplyManualStandby()
                                                        : ApplyListStandby();
        CloseAfterApply(ok);
      });
  };
  const auto add_active = [&]() noexcept {
    active_button = dialog.AddButton(
      _("Set Active Frequency"),
      [this]() noexcept {
        const bool ok = GetCurrentIndex() == TAB_MANUAL ? ApplyManualActive()
                                                        : ApplyListActive();
        CloseAfterApply(ok);
      });
  };

  if (primary == FrequencyBrowserPrimary::ActiveFirst) {
    add_active();
    add_standby();
  } else {
    add_standby();
    add_active();
  }

  favorites_tool_button =
    dialog.AddButton(_("Add to Favorites"),
                     [this]() noexcept {
                       const unsigned t = GetCurrentIndex();
                       if (t == TAB_NEAREST)
                         ApplyAddToFavorites();
                       else if (t == TAB_FAVORITES)
                         ApplyRemoveFavorite();
                     });

  add_favorite_entry_button =
    dialog.AddButton(_("Add Entry"),
                     [this]() noexcept { ApplyAddFavoriteEntry(); });
}

void
FrequencyBrowserWidget::AddTabs(const DialogLook &look)
{
  auto n = std::make_unique<FrequencyListTabWidget>(look, &nearest, *this);
  nearest_list = n.get();
  auto f = std::make_unique<FrequencyListTabWidget>(look, &favorites, *this);
  favorites_list = f.get();
  auto m = std::make_unique<ManualFrequencyWidget>(look);
  manual = m.get();
  AddTab(std::move(n), _("Nearest"));
  AddTab(std::move(f), _("Favorites"));
  AddTab(std::move(m), _("Manual"));
}

void
FrequencyBrowserWidget::UpdateActionButtons() noexcept
{
  if (form == nullptr || active_button == nullptr || standby_button == nullptr)
    return;

  const unsigned tab = GetCurrentIndex();
  if (tab == TAB_MANUAL) {
    active_button->SetEnabled(true);
    standby_button->SetEnabled(true);
    if (favorites_tool_button != nullptr) {
      favorites_tool_button->SetCaption(_("Add to Favorites"));
      favorites_tool_button->SetEnabled(false);
    }
    if (add_favorite_entry_button != nullptr)
      add_favorite_entry_button->SetEnabled(false);
    return;
  }

  const FrequencyEntry *e = nullptr;
  if (tab == TAB_NEAREST)
    e = nearest_list != nullptr ? nearest_list->GetSelected() : nullptr;
  else
    e = favorites_list != nullptr ? favorites_list->GetSelected() : nullptr;

  const bool ok = e != nullptr && e->frequency.IsDefined();
  active_button->SetEnabled(ok);
  standby_button->SetEnabled(ok);

  if (favorites_tool_button != nullptr) {
    if (tab == TAB_NEAREST) {
      favorites_tool_button->SetCaption(_("Add to Favorites"));
      const bool add_ok = ok && e->kind != FrequencyEntry::Kind::FILE;
      favorites_tool_button->SetEnabled(add_ok);
    } else {
      favorites_tool_button->SetCaption(_("Remove"));
      favorites_tool_button->SetEnabled(ok);
    }
  }

  if (add_favorite_entry_button != nullptr)
    add_favorite_entry_button->SetEnabled(tab == TAB_FAVORITES);
}

void
FrequencyListTabWidget::OnActivateItem(unsigned) noexcept
{
  browser->HandleListActivate();
}

void
FrequencyListTabWidget::OnCursorMoved(unsigned) noexcept
{
  browser->UpdateActionButtons();
}

} // namespace

void
ShowFrequencyBrowserDialog(const char *initial_tab,
                           FrequencyBrowserPrimary primary) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<FrequencyBrowserWidget> dialog(
    WidgetDialog::Full{}, UIGlobals::GetMainWindow(), look,
    _("Frequencies"));

  dialog.SetWidget(
    TabWidget::Orientation::AUTO,
    std::make_unique<ButtonWidget>(
      look.button, _("Close"), dialog.MakeModalResultCallback(mrOK)));

  auto &tabs = dialog.GetWidget();
  tabs.CreateButtons(dialog, primary);

  tabs.SetPageFlippedCallback([&dialog, &tabs]() noexcept {
    StaticString<128> title;
    title.Format("%s: %s", _("Frequencies"),
                 tabs.GetButtonCaption(tabs.GetCurrentIndex()));
    dialog.SetCaption(title);
    tabs.UpdateActionButtons();
  });

  dialog.PrepareWidget();
  tabs.AddTabs(look);

  unsigned start_tab = 0;
  if (initial_tab != nullptr) {
    if (StringIsEqual(initial_tab, "favorites"))
      start_tab = 1;
    else if (StringIsEqual(initial_tab, "manual"))
      start_tab = 2;
  }

  tabs.SetCurrent(start_tab);
  {
    StaticString<128> title;
    title.Format("%s: %s", _("Frequencies"),
                 tabs.GetButtonCaption(tabs.GetCurrentIndex()));
    dialog.SetCaption(title);
  }

  tabs.UpdateActionButtons();
  dialog.EnableCursorSelection();
  dialog.ShowModal();
}

std::unique_ptr<Widget>
LoadFrequencyBrowserPanel(unsigned) noexcept
{
  return std::make_unique<ButtonWidget>(
    UIGlobals::GetDialogLook().button, _("Browse"),
    []() {
      dlgInfoBoxAccessClose();
      ShowFrequencyBrowserDialog(nullptr);
    });
}
