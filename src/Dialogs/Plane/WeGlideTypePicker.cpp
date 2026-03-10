// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideTypePicker.hpp"

#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Renderer/TextRowListItemRenderer.hpp"
#include "UIGlobals.hpp"
#include "net/client/WeGlide/AircraftList.hpp"
#ifdef HAVE_HTTP
#include "net/http/Init.hpp"
#endif

#include <cinttypes>
#include <algorithm>
#include <vector>

class WeGlideAircraftRenderer final : public TextRowListItemRenderer {
  const std::vector<WeGlide::AircraftType> &list;

public:
  explicit WeGlideAircraftRenderer(
      const std::vector<WeGlide::AircraftType> &_list) noexcept
    :list(_list) {}

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned i) noexcept override {
    StaticString<128> label;
    label.Format("%s (%" PRIu32 ")", list[i].name.c_str(), list[i].id);
    row_renderer.DrawTextRow(canvas, rc, label);
  }
};

static std::vector<WeGlide::AircraftType>
DownloadAircraftTypes([[maybe_unused]] const WeGlideSettings &settings)
{
#ifdef HAVE_HTTP
  if (Net::curl == nullptr)
    return {};

  PluggableOperationEnvironment env;
  const auto value = ShowCoFunctionDialog(UIGlobals::GetMainWindow(),
                                          UIGlobals::GetDialogLook(),
                                          _("Download"),
                                          WeGlide::DownloadAircraftList(
                                            *Net::curl, settings, env),
                                          &env);
  if (!value)
    return {};

  return *value;
#else
  return {};
#endif
}

bool
LookupWeGlideAircraftTypeName(unsigned aircraft_id,
                              StaticString<96> &name)
{
  return WeGlide::LookupAircraftTypeName(aircraft_id, name);
}

bool
SelectWeGlideAircraftType(unsigned &aircraft_id,
                          const WeGlideSettings &settings)
{
  auto list = WeGlide::LoadAircraftListCache();
  if (list.empty())
    list = DownloadAircraftTypes(settings);

  if (list.empty()) {
    ShowMessageBox(_("Could not load WeGlide aircraft list."),
                   _("WeGlide Type"), MB_OK | MB_ICONEXCLAMATION);
    return false;
  }

  const unsigned old_aircraft_id = aircraft_id;

  const auto find_index = [&list](unsigned id) -> unsigned {
    const auto i = std::find_if(list.begin(), list.end(),
                                [id](const auto &item){
                                  return item.id == id;
                                });
    return i != list.end()
      ? static_cast<unsigned>(std::distance(list.begin(), i))
      : 0;
  };

  unsigned initial_index = find_index(aircraft_id);

  WeGlideAircraftRenderer renderer(list);
  while (true) {
    const int result = ListPicker(_("Select WeGlide Type"),
                                  list.size(), initial_index,
                                  renderer.CalculateLayout(
                                    UIGlobals::GetDialogLook()),
                                  renderer, false,
                                  nullptr,
                                  nullptr,
                                  _("Refresh"));

    if (result >= 0) {
      aircraft_id = list[result].id;
      return aircraft_id != old_aircraft_id;
    }

    if (result == -2) {
      auto refreshed = DownloadAircraftTypes(settings);
      if (refreshed.empty()) {
        ShowMessageBox(_("Could not load WeGlide aircraft list."),
                       _("WeGlide Type"), MB_OK | MB_ICONEXCLAMATION);
        continue;
      }

      list = std::move(refreshed);
      initial_index = find_index(aircraft_id);
      continue;
    }

    return false;
  }
}
