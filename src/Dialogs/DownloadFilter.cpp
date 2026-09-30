// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFilter.hpp"
#include "WidgetDialog.hpp"
#include "Widget/MultiSelectListWidget.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Form/CheckBox.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"
#include "Profile/Profile.hpp"
#include "Repository/AreaName.hpp"
#include "Repository/AvailableFile.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Glue.hpp"
#include "ui/canvas/Canvas.hpp"
#include "util/StringAPI.hxx"
#include "util/StringBuilder.hxx"
#include "util/CharUtil.hxx"

#include <algorithm>
#include <string>
#include <vector>

/** the ticked area codes, lowercase; empty = no filter */
static std::vector<std::string> selected_areas;

/** the session's search text */
static std::string search_text;

static bool profile_loaded = false;

[[gnu::pure]]
static std::string
NormalizeArea(const char *area) noexcept
{
  std::string code(area != nullptr ? area : "");
  for (auto &ch : code)
    ch = ToLowerASCII(ch);
  return code;
}

/**
 * The name the list and the filter row show for an area code.
 */
[[gnu::pure]]
static const char *
AreaDisplayName(const std::string &code, std::span<char> buffer) noexcept
{
  if (const char *name = GetAreaDisplayName(code.c_str()); name != nullptr)
    return gettext(name);

  /* an unknown code: show it in capitals, "DE" style */
  const std::size_t n = std::min(code.length(), buffer.size() - 1);
  for (std::size_t i = 0; i < n; ++i)
    buffer[i] = ToUpperASCII(code[i]);
  buffer[n] = '\0';
  return buffer.data();
}

bool
DownloadFilter::AppliesTo(FileType type) noexcept
{
  switch (type) {
  case FileType::AIRSPACE:
  case FileType::RASP:
  case FileType::WAYPOINT:
  case FileType::WAYPOINTDETAILS:
  case FileType::MAP:
    return true;

  default:
    return false;
  }
}

void
DownloadFilter::LoadFromProfile() noexcept
{
  if (profile_loaded)
    return;

  profile_loaded = true;
  selected_areas.clear();

  const char *value = Profile::Get(ProfileKeys::DownloadAreaFilter);
  if (value == nullptr)
    return;

  for (const char *i = value; *i != '\0';) {
    const char *comma = StringFind(i, ',');
    const std::string_view code{i, comma != nullptr
        ? std::size_t(comma - i) : StringLength(i)};
    if (!code.empty())
      selected_areas.emplace_back(NormalizeArea(std::string(code).c_str()));

    if (comma == nullptr)
      break;
    i = comma + 1;
  }

  std::sort(selected_areas.begin(), selected_areas.end());
  selected_areas.erase(std::unique(selected_areas.begin(),
                                   selected_areas.end()),
                       selected_areas.end());
}

static void
SaveToProfile() noexcept
{
  std::string value;
  for (const auto &code : selected_areas) {
    if (!value.empty())
      value.push_back(',');
    value += code;
  }

  Profile::Set(ProfileKeys::DownloadAreaFilter, value.c_str());
}

bool
DownloadFilter::IsAllAreas() noexcept
{
  return selected_areas.empty();
}

[[gnu::pure]]
static bool
IsSelectedArea(const std::string &code) noexcept
{
  return std::find(selected_areas.begin(), selected_areas.end(),
                   code) != selected_areas.end();
}

bool
DownloadFilter::MatchesArea(const AvailableFile &file) noexcept
{
  if (selected_areas.empty())
    return true;

  const char *area = file.GetArea();
  if (area == nullptr || *area == '\0')
    /* a global file concerns every country */
    return true;

  return IsSelectedArea(NormalizeArea(area));
}

const char *
DownloadFilter::GetSearchText() noexcept
{
  return search_text.c_str();
}

void
DownloadFilter::SetSearchText(const char *text) noexcept
{
  search_text = text != nullptr ? text : "";
}

[[gnu::pure]]
static bool
ContainsIgnoreCase(const char *haystack, const std::string &needle) noexcept
{
  if (haystack == nullptr)
    return false;

  std::string lower(haystack);
  for (auto &ch : lower)
    ch = ToLowerASCII(ch);

  return lower.find(needle) != std::string::npos;
}

bool
DownloadFilter::MatchesSearch(const AvailableFile &file) noexcept
{
  if (search_text.empty())
    return true;

  std::string needle = search_text;
  for (auto &ch : needle)
    ch = ToLowerASCII(ch);

  return ContainsIgnoreCase(file.GetName(), needle) ||
    ContainsIgnoreCase(file.GetDescription(), needle);
}

const char *
DownloadFilter::FormatAreas(std::span<char> buffer) noexcept
{
  if (selected_areas.empty())
    return _("All");

  if (buffer.empty())
    return "";

  buffer.front() = '\0';
  BasicStringBuilder<char> builder{buffer};
  bool first = true;
  char name_buffer[16];

  try {
    for (const auto &code : selected_areas) {
      if (!first)
        builder.Append(", ");
      first = false;
      builder.Append(AreaDisplayName(code, name_buffer));
    }
  } catch (BasicStringBuilder<char>::Overflow) {
    /* the buffer is full: what fits is enough for a row */
  }

  return buffer.data();
}

/*
 * the checkbox dialog
 */

/**
 * One row per area announced by the repository, a checkbox in front
 * of the country's name.
 */
class DownloadAreaListWidget final : public MultiSelectListWidget {
  TextRowRenderer row_renderer;

  /** the codes on display, sorted by their display name */
  std::vector<std::string> codes;

public:
  explicit DownloadAreaListWidget(std::vector<std::string> &&_codes) noexcept
    :codes(std::move(_codes)) {}

  const std::vector<std::string> &GetCodes() const noexcept {
    return codes;
  }

  std::vector<std::string> GetSelection() const noexcept {
    std::vector<std::string> result;
    for (unsigned i = 0; i < codes.size(); ++i)
      if (IsSelected(i))
        result.emplace_back(codes[i]);
    return result;
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc,
               row_renderer.CalculateLayout(*look.list.font));
    SetLengthWithSelection(codes.size());

    for (unsigned i = 0; i < codes.size(); ++i)
      SetSelected(i, IsSelectedArea(codes[i]));

    MultiSelectListWidget::Prepare(parent, rc);
  }

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (idx >= codes.size())
      return;

    const unsigned padding = Layout::GetTextPadding();
    const unsigned box_size = rc.GetHeight() > 2 * padding
      ? rc.GetHeight() - 2 * padding
      : 0;

    PixelRect box_rc;
    box_rc.left = rc.left + int(padding);
    box_rc.top = rc.top + int(padding);
    box_rc.right = box_rc.left + int(box_size);
    box_rc.bottom = box_rc.top + int(box_size);

    const bool focused = !HasCursorKeys() || GetList().HasFocus();
    DrawCheckBox(canvas, UIGlobals::GetDialogLook(), box_rc,
                 IsSelected(idx), focused, false, true);

    char name_buffer[16];
    PixelRect text_rc = rc;
    text_rc.left = box_rc.right + 2 * int(padding);
    row_renderer.DrawTextRow(canvas, text_rc,
                             AreaDisplayName(codes[idx], name_buffer));
  }
};

/**
 * The codes offered for ticking: what the repository announces, plus
 * whatever is ticked already (a stored selection shall not disappear
 * just because the repository index is missing), sorted by display
 * name.
 */
static std::vector<std::string>
CollectAreaCodes() noexcept
{
  std::vector<std::string> codes = selected_areas;

  FileRepository repository;
  LoadAllRepositories(repository);

  for (const auto &file : repository) {
    const char *area = file.GetArea();
    if (area == nullptr || *area == '\0')
      continue;

    auto code = NormalizeArea(area);
    if (std::find(codes.begin(), codes.end(), code) == codes.end())
      codes.emplace_back(std::move(code));
  }

  /* countries the name table knows come first, sorted by name; the
     unknown codes follow at the bottom, so they do not clutter the
     familiar part of the list */
  std::sort(codes.begin(), codes.end(),
            [](const std::string &a, const std::string &b){
              const bool known_a = GetAreaDisplayName(a.c_str()) != nullptr;
              const bool known_b = GetAreaDisplayName(b.c_str()) != nullptr;
              if (known_a != known_b)
                return known_a;

              char buffer_a[16], buffer_b[16];
              return StringCollate(AreaDisplayName(a, buffer_a),
                                   AreaDisplayName(b, buffer_b)) < 0;
            });

  return codes;
}

bool
DownloadFilter::EditAreas() noexcept
{
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), _("Countries"));

  auto widget = std::make_unique<DownloadAreaListWidget>(CollectAreaCodes());
  DownloadAreaListWidget *const list = widget.get();

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("All"), [list](){ list->ClearSelection(); });
  dialog.AddButton(_("Cancel"), mrCancel);

  /* the widget is prepared by ShowModal(), so the ticks are set in
     Prepare() from the current selection */
  dialog.FinishPreliminary(std::move(widget));

  if (dialog.ShowModal() != mrOK)
    return false;

  selected_areas = list->GetSelection();
  std::sort(selected_areas.begin(), selected_areas.end());
  SaveToProfile();
  return true;
}
