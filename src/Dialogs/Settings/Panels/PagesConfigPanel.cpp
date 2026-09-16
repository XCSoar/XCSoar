// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PagesConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "PageActions.hpp"
#include "Language/Language.hpp"
#include "Profile/PageProfile.hpp"
#include "Profile/Current.hpp"
#include "Interface.hpp"
#include "DataGlobals.hpp"
#include "Weather/Features.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#ifdef HAVE_EDL
#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/StateController.hpp"
#endif
#include "UIGlobals.hpp"
#include "Widget/ButtonPanelWidget.hpp"

#ifdef HAVE_HTTP
#include "Weather/SkySight/SkySightClient.hpp"
#endif

#include <vector>

static constexpr StaticEnumChoice main_list[] = {
  { PageLayout::Main::MAP, N_("Map") },
  { PageLayout::Main::MAP_NORTH_UP, N_("Map (north-up)") },
  { PageLayout::Main::FLARM_RADAR, N_("FLARM Radar") },
  { PageLayout::Main::THERMAL_ASSISTANT, N_("Thermal Assistant") },
  { PageLayout::Main::HORIZON, N_("Horizon") },
  nullptr
};

static constexpr StaticEnumChoice bottom_list[] = {
  { PageLayout::Bottom::NOTHING, N_("Nothing") },
  { PageLayout::Bottom::CROSS_SECTION, N_("Cross section") },
  /* Always available: RASP does not require OpenGL, and the shared
     weather cursor bar works for RASP on memory canvas / Kobo. */
  { PageLayout::Bottom::WEATHER_CONTROLS, NC_("Setting", "Weather controls") },
  nullptr
};

static constexpr StaticEnumChoice overlay_list[] = {
  { PageLayout::Overlay::NONE, N_("None") },
  { PageLayout::Overlay::RASP, NC_("Abbreviation", "RASP") },
#ifdef HAVE_EDL
  { PageLayout::Overlay::EDL, NC_("Abbreviation", "EDL") },
#endif
#ifdef HAVE_HTTP
  { PageLayout::Overlay::XCTHERM, "XC Therm" },
  { PageLayout::Overlay::SKYSIGHT, "SkySight" },
#endif
  nullptr
};

static const char *const Caption_MainArea = N_("Main area");
static const char *const Caption_InfoBoxes = N_("InfoBoxes");
static const char *const Caption_BottomArea = N_("Bottom area");
static const char *const Caption_Overlay = NC_("Setting", "Map overlay");

static const char *const Help_Overlay =
  N_("Optional weather overlay on map pages. "
     "Use with Weather controls in the bottom area for in-flight adjustment.");

/**
 * The caption of the choice of the InfoBoxes of a page: the
 * automatic set, none at all, or the name of one set.
 */
static const char *
GetInfoBoxesCaption(const PageLayout::InfoBoxConfig &config) noexcept
{
  if (!config.enabled)
    return _("None");

  if (config.auto_switch || config.panel >= InfoBoxSettings::MAX_PANELS)
    return C_("Setting", "Auto");

  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;
  return gettext(info_box_settings.panels[config.panel].name);
}

/**
 * The layer or the level the weather overlay of a page shows, for
 * the item which picks it and for the list of the pages: the RASP
 * field, the EDL level or the SkySight layer.
 *
 * @return the text, or nullptr if the overlay has none
 */
static const char *
GetOverlayDetail(const PageLayout &value, StaticString<64> &buffer) noexcept
{
  if (!value.IsMapMain())
    return nullptr;

  switch (value.overlay) {
  case PageLayout::Overlay::RASP: {
    const auto rasp = DataGlobals::GetRasp();
    if (rasp == nullptr || value.rasp_field < 0 ||
        unsigned(value.rasp_field) >= rasp->GetItemCount())
      return nullptr;

    const auto &item = rasp->GetItemInfo(value.rasp_field);
    return item.label != nullptr ? gettext(item.label) : item.name.c_str();
  }

#ifdef HAVE_EDL
  case PageLayout::Overlay::EDL:
    if (value.edl_isobar > 0 &&
        EDL::IsSupportedIsobar(unsigned(value.edl_isobar))) {
      const unsigned isobar = value.edl_isobar;
      const auto altitude =
        FormatUserAltitude(EDL::GetAltitudeForIsobar(isobar));
      buffer.Format("%u hPa (%s)", isobar / 100, altitude.c_str());
      return buffer.c_str();
    }

    return C_("Weather control", "Auto");
#endif

  case PageLayout::Overlay::SKYSIGHT:
#ifdef HAVE_HTTP
    if (const auto skysight = DataGlobals::GetSkySight();
        skysight != nullptr && !value.skysight_overlay.empty()) {
      /* the name of the layer, or its identifier while it is not
         among the selected layers */
      if (const auto *layer =
            skysight->GetSelectedLayer(value.skysight_overlay.c_str());
          layer != nullptr)
        return layer->name.c_str();

      return value.skysight_overlay.c_str();
    }
#endif
    return nullptr;

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::XCTHERM:
#ifndef HAVE_EDL
  case PageLayout::Overlay::EDL:
#endif
  case PageLayout::Overlay::MAX:
    break;
  }

  return nullptr;
}

/**
 * The layout of one page in a dialog: what the main area, the
 * InfoBoxes and the bottom area show, and the weather overlay of a
 * map page.  It edits the page in place, and it deletes the page
 * from the settings.
 */
class PageLayoutEditWidget final : public GroupedListWidget {
  PageSettings &settings;

  /** the index of the page which is edited */
  unsigned index;

  WidgetDialog &dialog;

  /**
   * The choice of the InfoBoxes of a page: the automatic set and
   * none, then the sets of the user.
   */
  static constexpr unsigned IBP_AUTO = 0, IBP_NONE = 1, IBP_FIRST_PANEL = 2;

public:
  PageLayoutEditWidget(PageSettings &_settings, unsigned _index,
                       WidgetDialog &_dialog) noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()),
     settings(_settings), index(_index), dialog(_dialog) {}

  /**
   * The index of the page, or the index of the page which takes its
   * place after it was deleted.
   */
  unsigned GetIndex() const noexcept {
    return index;
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  PageLayout &GetValue() noexcept {
    return settings.pages[index];
  }

  void Fill() noexcept;
  void Refresh() noexcept;

  void AddOverlayItems() noexcept;
  void AddOverlayDetailItem() noexcept;

  void PickMain() noexcept;
  void PickInfoBoxes() noexcept;
  void PickBottom() noexcept;
  void PickOverlay() noexcept;
  void PickRaspField(const char *caption) noexcept;
#ifdef HAVE_EDL
  void PickEdlLevel(const char *caption) noexcept;
#endif
#ifdef HAVE_HTTP
  void PickSkySightLayer(const char *caption) noexcept;
#endif

  void DeletePage() noexcept;
};

void
PageLayoutEditWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  Refresh();

  GroupedListWidget::Prepare(parent, rc);
}

void
PageLayoutEditWidget::Refresh() noexcept
{
  StaticString<32> caption;
  caption.Format("%s %u", _("Page"), index + 1);
  dialog.SetCaption(caption);

  Clear();
  Fill();
  UpdateLayout();
}

void
PageLayoutEditWidget::Fill() noexcept
{
  const PageLayout &value = GetValue();

  AddGroup();

  AddItem(gettext(Caption_MainArea), [this](){ PickMain(); },
          {.value = GetEnumCaption(main_list, (unsigned)value.main),
           .chevron = true});

  AddItem(gettext(Caption_InfoBoxes), [this](){ PickInfoBoxes(); },
          {.value = GetInfoBoxesCaption(value.infobox_config),
           .chevron = true});

  AddItem(gettext(Caption_BottomArea), [this](){ PickBottom(); },
          {.value = GetEnumCaption(bottom_list, (unsigned)value.bottom),
           .chevron = true});

  AddOverlayItems();

  /* the last page cannot be deleted */
  AddGroup();
  AddButton(C_("Button", "Delete"), [this](){ DeletePage(); },
            {.disabled = settings.n_pages < 2});
}

void
PageLayoutEditWidget::AddOverlayItems() noexcept
{
  const PageLayout &value = GetValue();

  /* the explanation stays on the page: it says why the items are
     not available on a page which shows no map */
  AddGroup(_("Weather"), {.footer = gettext(Help_Overlay)});

  AddItem(gettext(Caption_Overlay), [this](){ PickOverlay(); },
          {.value = GetEnumCaption(overlay_list, (unsigned)value.overlay),
           .chevron = true,
           .disabled = !value.IsMapMain()});

  AddOverlayDetailItem();
}

void
PageLayoutEditWidget::AddOverlayDetailItem() noexcept
{
  const PageLayout &value = GetValue();

  StaticString<64> buffer;
  const char *detail = GetOverlayDetail(value, buffer);

  if (value.IsMapMain()) {
    switch (value.overlay) {
    case PageLayout::Overlay::RASP: {
      const char *caption = _("RASP Layer");

      const auto rasp = DataGlobals::GetRasp();
      if (rasp == nullptr || rasp->GetItemCount() == 0) {
        AddItem(caption, {.value = _("No RASP file loaded"),
                          .disabled = true});
        return;
      }

      AddItem(caption, [this, caption](){ PickRaspField(caption); },
              {.value = detail != nullptr ? detail : _("None"),
               .chevron = true});
      return;
    }

#ifdef HAVE_EDL
    case PageLayout::Overlay::EDL: {
      const char *caption = _("EDL Level");
      AddItem(caption, [this, caption](){ PickEdlLevel(caption); },
              {.value = detail, .chevron = true});
      return;
    }
#endif

    case PageLayout::Overlay::SKYSIGHT: {
      const char *caption = C_("Setting", "SkySight layer");

#ifdef HAVE_HTTP
      const auto skysight = DataGlobals::GetSkySight();
      if (skysight != nullptr &&
          (skysight->NumSelectedLayers() > 0 || detail != nullptr)) {
        AddItem(caption, [this, caption](){ PickSkySightLayer(caption); },
                {.value = detail, .chevron = true});
        return;
      }
#endif

      AddItem(caption, {.value = _("No SkySight layers selected"),
                        .disabled = true});
      return;
    }

    case PageLayout::Overlay::NONE:
    case PageLayout::Overlay::XCTHERM:
#ifndef HAVE_EDL
    case PageLayout::Overlay::EDL:
#endif
    case PageLayout::Overlay::MAX:
      break;
    }
  }

  AddItem(C_("Setting", "Layer / Level"),
          {.value = _("N/A"), .disabled = true});
}

void
PageLayoutEditWidget::PickMain() noexcept
{
  PageLayout &value = GetValue();

  unsigned main = (unsigned)value.main;
  if (!PickEnum(gettext(Caption_MainArea),
                _("Specifies what should be displayed in the main area."),
                main_list, main))
    return;

  value.main = (PageLayout::Main)main;
  value.Normalise();
  Refresh();
}

void
PageLayoutEditWidget::PickInfoBoxes() noexcept
{
  const InfoBoxSettings &info_box_settings =
    CommonInterface::GetUISettings().info_boxes;

  static constexpr const char *panel_help[] = {
    N_("For circling mode. Displayed when 'Auto' is selected and glider is circling."),
    N_("For cruise mode. Displayed when 'Auto' is selected and glider is below final glide altitude."),
    N_("For final glide mode. Displayed when 'Auto' is selected and glider is above final glide altitude."),
  };

  std::vector<PickerChoice> choices{
    {C_("Setting", "Auto"),
     _("Displays either the Circling, Cruise, or Final glide InfoBoxes.")},
    {_("None"), _("Show fullscreen (no InfoBoxes)")},
  };

  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; ++i)
    choices.push_back({gettext(info_box_settings.panels[i].name),
                       i < std::size(panel_help)
                       ? gettext(panel_help[i])
                       : _("A custom InfoBox set")});

  PageLayout &value = GetValue();
  auto &config = value.infobox_config;

  unsigned current = IBP_NONE;
  if (config.enabled) {
    if (config.auto_switch)
      current = IBP_AUTO;
    else if (config.panel < InfoBoxSettings::MAX_PANELS)
      current = IBP_FIRST_PANEL + config.panel;
    else
      /* fix up illegal value */
      current = IBP_FIRST_PANEL;
  }

  const int picked =
    PickChoice(gettext(Caption_InfoBoxes),
               _("Specifies which InfoBoxes should be displayed on this page."),
               choices, current);
  if (picked < 0 || unsigned(picked) == current)
    return;

  const unsigned choice = picked;
  if (choice == IBP_AUTO) {
    config.enabled = true;
    config.auto_switch = true;
    config.panel = 0;
  } else if (choice == IBP_NONE)
    config.enabled = false;
  else {
    config.enabled = true;
    config.auto_switch = false;
    config.panel = choice - IBP_FIRST_PANEL;
  }

  value.Normalise();
  Refresh();
}

void
PageLayoutEditWidget::PickBottom() noexcept
{
  PageLayout &value = GetValue();

  unsigned bottom = (unsigned)value.bottom;
  if (!PickEnum(gettext(Caption_BottomArea),
                _("Specifies what should be displayed below the main area. "
                  "Weather controls require a weather map "
                  "overlay."),
                bottom_list, bottom))
    return;

  value.bottom = (PageLayout::Bottom)bottom;

  if (value.bottom == PageLayout::Bottom::WEATHER_CONTROLS &&
      value.IsMapMain() &&
      !value.UsesWeatherOverlay()) {
#ifdef HAVE_EDL
    value.overlay = PageLayout::Overlay::EDL;
#else
    const auto rasp = DataGlobals::GetRasp();
    if (rasp != nullptr && rasp->GetItemCount() > 0)
      value.overlay = PageLayout::Overlay::RASP;
    else
      value.bottom = PageLayout::Bottom::NOTHING;
#endif
  }

  value.Normalise();
  Refresh();
}

void
PageLayoutEditWidget::PickOverlay() noexcept
{
  PageLayout &value = GetValue();

  unsigned _overlay = (unsigned)value.overlay;
  if (!PickEnum(gettext(Caption_Overlay), gettext(Help_Overlay),
                overlay_list, _overlay))
    return;

  const auto overlay = (PageLayout::Overlay)_overlay;
  if (overlay == PageLayout::Overlay::SKYSIGHT) {
#ifdef HAVE_HTTP
    const auto skysight = DataGlobals::GetSkySight();
    const SkySight::Layer *layer = nullptr;
    if (skysight != nullptr) {
      if (!value.skysight_overlay.empty() &&
          skysight->IsSelectedLayer(value.skysight_overlay.c_str()))
        layer = skysight->GetSelectedLayer(value.skysight_overlay.c_str());

      for (std::size_t i = 0; layer == nullptr &&
             i < skysight->NumSelectedLayers(); ++i)
        layer = skysight->GetSelectedLayer(i);
    }

    if (layer != nullptr) {
      value.skysight_overlay = layer->id;
    } else if (value.skysight_overlay.empty()) {
      const char *message;
      if (skysight == nullptr)
        message = _("SkySight is unavailable.");
      else if (skysight->IsThrottled())
        message = _("SkySight API rate-limited. Retrying shortly.");
      else if (!skysight->HasForecastLayers())
        message = _("Loading SkySight catalog...");
      else
        message = _("No SkySight layers selected");

      ShowMessageBox(message, "SkySight", MB_OK | MB_ICONINFORMATION);
      return;
    } else
      value.skysight_overlay.clear();
#else
    value.skysight_overlay.clear();
#endif
  }

  value.overlay = overlay;
  value.Normalise();
  Refresh();
}

void
PageLayoutEditWidget::PickRaspField(const char *caption) noexcept
{
  const auto rasp = DataGlobals::GetRasp();
  if (rasp == nullptr)
    return;

  PageLayout &value = GetValue();
  if (!Rasp::PickField(caption,
                       _("RASP weather layer to display on this map page."),
                       *rasp, value.rasp_field))
    return;

  value.Normalise();
  Refresh();
}

#ifdef HAVE_EDL

void
PageLayoutEditWidget::PickEdlLevel(const char *caption) noexcept
{
  StaticString<64> captions[EDL::NUM_ISOBARS];
  std::vector<PickerChoice> choices{
    {C_("Weather control", "Auto"),
     _("Follow altitude on page enter (auto level).")},
  };

  for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i) {
    const unsigned isobar = EDL::ISOBARS[i];
    const auto altitude =
      FormatUserAltitude(EDL::GetAltitudeForIsobar(isobar));

    if (!altitude.empty())
      captions[i].Format("%u hPa (%s)", isobar / 100, altitude.c_str());
    else
      captions[i].Format("%u hPa", isobar / 100);

    choices.push_back({captions[i].c_str()});
  }

  PageLayout &value = GetValue();

  int current = 0;
  for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i)
    if (value.edl_isobar > 0 &&
        unsigned(value.edl_isobar) == EDL::ISOBARS[i])
      current = i + 1;

  const int picked =
    PickChoice(caption,
               _("EDL pressure level / altitude band for this map page. "
                 "Auto follows aircraft altitude when the page is opened."),
               choices, current);
  if (picked < 0 || picked == current)
    return;

  value.edl_isobar = picked == 0 ? 0 : EDL::ISOBARS[picked - 1];
  value.Normalise();
  Refresh();
}

#endif

#ifdef HAVE_HTTP

void
PageLayoutEditWidget::PickSkySightLayer(const char *caption) noexcept
{
  const auto skysight = DataGlobals::GetSkySight();
  if (skysight == nullptr)
    return;

  PageLayout &value = GetValue();

  /* the selected layers, and the stored one first while it is not
     among them */
  std::vector<PickerChoice> choices;
  std::vector<const SkySight::Layer *> layers;
  int current = -1;

  if (!value.skysight_overlay.empty() &&
      !skysight->IsSelectedLayer(value.skysight_overlay.c_str())) {
    choices.push_back({value.skysight_overlay.c_str()});
    layers.push_back(nullptr);
    current = 0;
  }

  for (std::size_t i = 0; i < skysight->NumSelectedLayers(); ++i) {
    const auto *layer = skysight->GetSelectedLayer(i);
    if (layer == nullptr)
      continue;

    if (layer->id == value.skysight_overlay.c_str())
      current = choices.size();

    choices.push_back({layer->name.c_str()});
    layers.push_back(layer);
  }

  const int picked =
    PickChoice(caption,
               _("SkySight layer used when this page overlay is SkySight."),
               choices, current);
  if (picked < 0 || picked == current || layers[picked] == nullptr)
    return;

  value.skysight_overlay = layers[picked]->id;
  value.skysight_time = PageLayout::SKYSIGHT_TIME_AUTO;
  value.Normalise();
  Refresh();
}

#endif

void
PageLayoutEditWidget::DeletePage() noexcept
{
  const unsigned n = settings.n_pages;
  if (n < 2)
    return;

  std::copy(settings.pages.begin() + index + 1,
            settings.pages.begin() + n,
            settings.pages.begin() + index);
  settings.pages[n - 1] = PageLayout::Undefined();
  settings.n_pages = n - 1;

  if (index == settings.n_pages)
    --index;

  dialog.SetModalResult(mrOK);
}

/**
 * The pages the user switches between in flight: their order, and
 * what each of them shows.  The buttons below the list act on the
 * page the cursor is on: a tap on a page only selects it, the second
 * tap or Enter opens it, and Left and Right lead to the buttons.
 */
class PagesConfigPanel final : public ConfigListPanel {
  PageSettings settings;

  /** the widget which holds the buttons below the list */
  ButtonPanelWidget *button_panel = nullptr;

  Button *move_up_button = nullptr, *move_down_button = nullptr,
    *add_button = nullptr;

public:
  PagesConfigPanel() noexcept {
    SetCursorCallback([this](int){ UpdateButtons(); });
  }

  void SetButtonPanel(ButtonPanelWidget &_button_panel) noexcept {
    button_panel = &_button_panel;
  }

private:
  /** Open the dialog which edits the page @p index. */
  void EditPage(unsigned index) noexcept;

  /** Append a page and open the dialog which edits it. */
  void AddPage() noexcept;

  /**
   * Swap the page the cursor is on with the one above (@p delta -1)
   * or below it (@p delta 1); the cursor follows the page.
   */
  void MovePage(int delta) noexcept;

  /** Enable the buttons which make sense for the page under the cursor. */
  void UpdateButtons() noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
PagesConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  ButtonPanel &buttons = button_panel->GetButtonPanel();

  move_up_button = buttons.Add(_("Move up"), [this](){ MovePage(-1); });
  move_down_button = buttons.Add(_("Move down"), [this](){ MovePage(1); });
  add_button = buttons.Add(C_("Button", "Add"), [this](){ AddPage(); });

  ConfigListPanel::Prepare(parent, rc);

  /* Left and Right lead from the list to the buttons, and Up and
     Down back to it */
  SetActionBar(buttons);
  UpdateButtons();
}

void
PagesConfigPanel::LoadSettings() noexcept
{
  settings = CommonInterface::GetUISettings().pages;

  for (unsigned i = 0; i < settings.n_pages; ++i)
    settings.pages[i].Normalise();
}

void
PagesConfigPanel::Fill() noexcept
{
  /* a tap selects a page for the buttons below; the second tap
     opens it */
  AddGroup(_("Pages"), {.enter_action = EnterAction::ACTION_BAR});

  for (unsigned i = 0; i < settings.n_pages; ++i) {
    const PageLayout &page = settings.pages[i];

    StaticString<4> number;
    number.Format("%u", i + 1);

    /* the layer of the overlay and the bottom area, over the whole
       width below the caption */
    StaticString<64> detail_buffer;
    StaticString<128> description;
    description.clear();
    if (const char *detail = GetOverlayDetail(page, detail_buffer);
        detail != nullptr)
      description = detail;

    const char *bottom = nullptr;
    if (page.bottom == PageLayout::Bottom::CROSS_SECTION)
      bottom = _("Cross section");
    else if (page.bottom == PageLayout::Bottom::WEATHER_CONTROLS)
      bottom = C_("Setting", "Weather controls");

    if (bottom != nullptr) {
      if (!description.empty())
        description += ", ";
      description += bottom;
    }

    const bool overlay = page.IsMapMain() &&
      page.overlay != PageLayout::Overlay::NONE;

    AddItem(GetEnumCaption(main_list, (unsigned)page.main),
            [this, i](){ EditPage(i); },
            {.icon_text = number.c_str(),
             .value = GetInfoBoxesCaption(page.infobox_config),
             .description = description.empty()
             ? nullptr
             : description.c_str(),
             .description_size = TextSize::SMALL,
             .badge = overlay
             ? GetEnumCaption(overlay_list, (unsigned)page.overlay)
             : nullptr,
             .chevron = true});
  }
}

void
PagesConfigPanel::UpdateButtons() noexcept
{
  if (move_up_button == nullptr)
    return;

  /* the pages are the only items of the list, so the index of a page
     is its item index */
  const int cursor = GetCursorIndex();
  const bool on_page = cursor >= 0 && unsigned(cursor) < settings.n_pages;

  move_up_button->SetEnabled(on_page && cursor > 0);
  move_down_button->SetEnabled(on_page &&
                               unsigned(cursor) + 1 < settings.n_pages);
  add_button->SetEnabled(settings.n_pages < PageSettings::MAX_PAGES);
}

void
PagesConfigPanel::AddPage() noexcept
{
  const unsigned n = settings.n_pages;
  if (n >= PageSettings::MAX_PAGES)
    return;

  settings.pages[n] = PageLayout::Default();
  settings.n_pages = n + 1;
  EditPage(n);
}

void
PagesConfigPanel::MovePage(int delta) noexcept
{
  const int cursor = GetCursorIndex();
  if (cursor < 0 || unsigned(cursor) >= settings.n_pages)
    return;

  const unsigned other = cursor + delta;
  if (other >= settings.n_pages)
    return;

  std::swap(settings.pages[cursor], settings.pages[other]);
  Refresh();
  SetCursorIndex(other);
  UpdateButtons();
}

void
PagesConfigPanel::EditPage(unsigned index) noexcept
{
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), _("Page"));

  auto editor =
    std::make_unique<PageLayoutEditWidget>(settings, index, dialog);
  const PageLayoutEditWidget &_editor = *editor;

  dialog.FinishPreliminary(std::move(editor));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();

  /* the page may have been deleted */
  Refresh();
  SetCursorIndex(_editor.GetIndex());
  UpdateButtons();
}

bool
PagesConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  std::fill(settings.pages.begin() + settings.n_pages,
            settings.pages.end(),
            PageLayout::Undefined());

  PageSettings &_settings = CommonInterface::SetUISettings().pages;
  for (unsigned int i = 0; i < PageSettings::MAX_PAGES; ++i) {
    PageLayout &dest = _settings.pages[i];
    const PageLayout &src = settings.pages[i];
    if (src != dest) {
      Profile::Save(Profile::map, src, i);
      changed = true;
    }
  }

  if (changed) {
    _settings = settings;
    PageActions::Update();
  }

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreatePagesConfigPanel()
{
  auto panel = std::make_unique<PagesConfigPanel>();
  auto &_panel = *panel;

  auto buttons =
    std::make_unique<ButtonPanelWidget>(std::move(panel),
                                        ButtonPanelWidget::Alignment::BOTTOM);
  _panel.SetButtonPanel(*buttons);
  return buttons;
}
