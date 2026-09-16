// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GroupedListTestDialog.hpp"
#include "Message.hpp"
#include "UIGlobals.hpp"
#include "WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Resources.hpp"
#include "Version.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Widget/ButtonWidget.hpp"
#include "Widget/WindowWidget.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Look/Look.hpp"
#include "NMEA/Attitude.hpp"
#include "Renderer/HorizonRenderer.hpp"
#include "Screen/Layout.hpp"
#include "time/Stamp.hpp"
#include "util/StaticString.hxx"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/PaintWindow.hpp"

#include <memory>
#include <span>

using SelectionMode = GroupedListWidget::SelectionMode;
using CheckPosition = GroupedListWidget::CheckPosition;
using EnterAction = GroupedListWidget::EnterAction;
using ToggleHitArea = GroupedListWidget::ToggleHitArea;
using BadgeStyle = GroupedListWidget::BadgeStyle;
using TextFont = GroupedListWidget::TextFont;
using TextSize = GroupedListWidget::TextSize;
using ExpandMode = GroupedListWidget::ExpandMode;
using ExpandTrigger = GroupedListWidget::ExpandTrigger;
using GroupOptions = GroupedListWidget::GroupOptions;

/**
 * The horizon of the project, banked as if the aircraft were turning:
 * no sensor feeds this dialog, and an empty horizon would say nothing
 * about the view below the list.
 */
class HorizonPreviewWindow final : public PaintWindow {
  const HorizonLook &look;

public:
  explicit HorizonPreviewWindow(const HorizonLook &_look) noexcept
    :look(_look) {}

protected:
  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.Clear(UIGlobals::GetDialogLook().background_color);

    AttitudeState attitude;
    attitude.Reset();

    attitude.bank_angle = Angle::Degrees(20);
    attitude.pitch_angle = Angle::Degrees(5);
    attitude.bank_angle_available.Update(TimeStamp{FloatDuration{1}});
    attitude.pitch_angle_available.Update(TimeStamp{FloatDuration{1}});

    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude);
  }
};

class HorizonPreviewWidget final : public WindowWidget {
public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique<HorizonPreviewWindow>(UIGlobals::GetLook().horizon);
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

/**
 * The row which the Advanced File Explorer shows above its list: a
 * read-only #WndProperty which names the directory the list is in.
 */
class LocationWidget final : public WindowWidget {
  const char *const label;
  const char *const path;

public:
  LocationWidget(const char *_label, const char *_path) noexcept
    :label(_label), path(_path) {}

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override {
    return {0u, Layout::GetMinimumControlHeight()};
  }

  PixelSize GetMaximumSize() const noexcept override {
    return {4096u, Layout::GetMinimumControlHeight()};
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique<WndProperty>(UIGlobals::GetDialogLook());
    w->Create(parent, rc, label, 0, style);
    w->SetReadOnly(true);
    w->SetText(path);
    SetWindow(std::move(w));
  }
};

struct DemoItem {
  const char *caption;

  ResourceId icon = ResourceId::Null();

  const char *icon_text = nullptr;

  const char *help = nullptr;

  const char *subtitle = nullptr;

  const char *value = nullptr;

  const char *badge = nullptr;

  BadgeStyle badge_style = BadgeStyle::PRIMARY;

  bool chevron = false;

  bool toggle = false;

  bool checked = false;

  bool disabled = false;

  const char *disabled_badge_label = nullptr;
};

static constexpr DemoItem quick_access[] = {
  {.caption = "Search"},
  {.caption = "Recently Used", .value = "12"},
};

static constexpr DemoItem online_services[] = {
  {.caption = "SkyLines", .badge = "active", .chevron = true},
  {.caption = "WeGlide", .badge = "synced",
   .badge_style = BadgeStyle::SUCCESS, .chevron = true},
  {.caption = "LiveTrack24", .value = "not signed in", .badge = "failed",
   .badge_style = BadgeStyle::DANGER, .chevron = true},
  {.caption = "XCSoar Cloud", .badge = "beta",
   .badge_style = BadgeStyle::WARNING},

  /* an item which is currently not available; it says why on its
     badge, because a disabled item shows no value */
  {.caption = "OGN Live", .disabled = true,
   .disabled_badge_label = "no network"},
};

static constexpr DemoItem weather[] = {
  /* a help text which needs about five lines, with a link into the
     manual at the end */
  {.caption = "Thermal Information Map",
   .help = "Thermals which other pilots have reported while they were "
   "flying, collected on a server and sent back to everyone who is "
   "online. The map is updated every few minutes as long as there is "
   "a network; without one, the last thermals stay on the screen and "
   "grow older until the flight ends. See "
   "[the manual](https://xcsoar.readthedocs.io/en/latest/users/"
   "weather.html) for the details.",
   .badge = "new", .chevron = true},

  /* three words */
  {.caption = "RASP", .help = "Computed last night.",
   .value = "2 h ago"},

  /* two lines of help, and everything an item can carry: a value, a
     badge, an arrow and a check mark at the left edge */
  {.caption = "SkySight",
   .help = "A commercial forecast with a fine grid; the subscription "
   "of this installation has ended. Renew it at "
   "[skysight.io](https://skysight.io/).",
   .value = "expired", .badge = "trial",
   .badge_style = BadgeStyle::WARNING, .chevron = true, .checked = true},

  /* no help: the footer of the group stays */
  {.caption = "Flugwetter", .chevron = true},
  /* not available, and therefore not checkable either */
  {.caption = "XC Therm", .badge = "offline", .disabled = true},
};

/* items which are nothing but a boolean: the switch shows the state,
   and tapping the item flips it */
static constexpr DemoItem switches[] = {
  {.caption = "Auto Zoom", .toggle = true, .checked = true},
  {.caption = "Circling Zoom",
   .help = "Zoom in while the aircraft circles, and out again when it "
   "leaves the thermal.",
   .toggle = true, .checked = true},
  {.caption = "Trail Drift",
   .subtitle = "Correct the trail for the wind",
   .toggle = true},
  {.caption = "FLARM Radar", .badge = "no device",
   .badge_style = BadgeStyle::WARNING, .toggle = true},

  /* an item which is not available shows no switch: its state is not
     in effect anyway */
  {.caption = "Thermal Assistant", .toggle = true, .checked = true,
   .disabled = true},
};

static constexpr DemoItem aircraft[] = {
  {.caption = "D-6886", .checked = true},
  {.caption = "D-4010"},
  {.caption = "D-KIZA", .checked = true},
  {.caption = "D-1089"},
};

static constexpr DemoItem devices[] = {
  {.caption = "Vega", .value = "COM1", .badge = "connected",
   .badge_style = BadgeStyle::SUCCESS},
  {.caption = "FLARM", .badge = "connected",
   .badge_style = BadgeStyle::SUCCESS, .chevron = true},
  /* an item which says on its badge why it is not available */
  {.caption = "LX Navigation", .disabled = true,
   .disabled_badge_label = "no port"},
  {.caption = "CAI 302", .badge = "no data",
   .badge_style = BadgeStyle::DANGER},
};

/* items with an icon: the same symbols which the map draws */
static constexpr DemoItem map_items[] = {
  {.caption = "Team mate", .icon = IDB_TEAMMATE_POS, .value = "12 km"},
  {.caption = "Thermal", .icon = IDB_THERMALSOURCE, .value = "2.4 m/s"},
  {.caption = "Mountain top", .icon = IDB_MOUNTAIN_TOP, .value = "1482 m"},
  {.caption = "Obstacle", .icon = IDB_OBSTACLE, .badge = "close",
   .badge_style = BadgeStyle::WARNING},
  {.caption = "Traffic", .icon = IDB_TRAFFIC_ALARM, .badge = "alarm",
   .badge_style = BadgeStyle::DANGER, .chevron = true},

  /* an item without an icon keeps the column, so that all captions
     stay aligned */
  {.caption = "Mark", .value = "none"},
};

/* items whose icon is a character: a color emoji where the platform
   can draw one, the glyph of the font where it cannot, and nothing
   where the font has no glyph either */
static constexpr DemoItem checklist[] = {
  {.caption = "Canopy", .icon_text = "🧹"},
  {.caption = "Water ballast", .icon_text = "💧", .value = "60 l"},
  {.caption = "Batteries", .icon_text = "🔋", .badge = "charged",
   .badge_style = BadgeStyle::SUCCESS},
  {.caption = "Barograph", .icon_text = "📈", .chevron = true},
};

/* items with a second line, the shape the WiFi list uses today */
static constexpr DemoItem networks[] = {
  /* a second line, a value and a badge on one item */
  {.caption = "Alpensegler", .subtitle = "WPA2, channel 36",
   .value = "82 %", .badge = "connected",
   .badge_style = BadgeStyle::SUCCESS},

  /* a second line which needs three of them */
  {.caption = "Flugplatz-Gast",
   .subtitle = "Open network without a password. The tower switches it "
   "off after the last landing, and XCSoar falls back to the mobile "
   "network until the next morning.",
   .chevron = true},

  {.caption = "Hangar", .subtitle = "WPA2, saved", .value = "47 %",
   .chevron = true},

  /* a second line which needs two of them */
  {.caption = "Segelflugplatz",
   .subtitle = "WPA2, saved last summer; the router is switched off "
   "outside the season.",
   .chevron = true},

  /* a value and a badge next to two lines */
  {.caption = "Werkstatt",
   .subtitle = "WPA2, behind two walls and the hangar door.",
   .value = "12 %", .badge = "weak",
   .badge_style = BadgeStyle::WARNING},

  /* a value and a badge next to three lines */
  {.caption = "Vereinsheim",
   .subtitle = "Open network with a login page which XCSoar cannot "
   "show; sign in on a phone first.",
   .value = "63 %", .badge = "captive",
   .badge_style = BadgeStyle::DANGER},

  {.caption = "Nachbar-Hotspot", .subtitle = "seen 2 days ago",
   .disabled = true, .disabled_badge_label = "out of range"},
};

static constexpr DemoItem data_files[] = {
  {.caption = "Waypoints", .value = "3 files", .chevron = true,
   .checked = true},
  /* one item with everything: a value, a badge, an arrow and a check
     mark at the right edge */
  {.caption = "Airspace", .value = "5 files", .badge = "outdated",
   .badge_style = BadgeStyle::WARNING, .chevron = true, .checked = true},
  {.caption = "Terrain", .value = "1.2 GB", .checked = true},
  {.caption = "Airfield Details", .chevron = true},
};

static void
AddItems(GroupedListWidget &widget, std::span<const DemoItem> items,
         bool callbacks) noexcept
{
  for (const auto &item : items) {
    const char *caption = item.caption;

    const GroupedListWidget::ItemOptions options{
      .icon = item.icon, .icon_text = item.icon_text,
      .subtitle = item.subtitle, .value = item.value,
      .badge = item.badge, .badge_style = item.badge_style,
      .chevron = item.chevron, .toggle = item.toggle,
      .checked = item.checked,
      .help = item.help, .disabled = item.disabled,
      .disabled_badge_label = item.disabled_badge_label,
    };

    if (callbacks)
      widget.AddItem(caption, [caption]{
        ShowMessageBox(caption, _("Grouped List Test"), MB_OK);
      }, options);
    else
      widget.AddItem(caption, options);
  }
}

static void
AddGroup(GroupedListWidget &widget, const char *caption,
         std::span<const DemoItem> items,
         const GroupOptions &options={}, bool callbacks=true) noexcept
{
  widget.AddGroup(caption, options);
  AddItems(widget, items, callbacks);
}

/**
 * A page which reproduces a tap that the layout can swallow: while
 * the cursor is on the item of the first group, its help fills the
 * footer of that group.  Tapping an item of the second group moves
 * the cursor away, the footer shrinks back to the text of the group
 * and everything below it slides up under the finger.
 */
static void
ShowTapTestDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Tap Test",
                  "Move the cursor to the item below, then tap an item of "
                  "the second group. The message box has to appear on the "
                  "first tap.");

  widget->AddGroup("Cursor",
                   {.footer = "Two lines of nothing, which the help of the "
                    "item above replaces while the cursor is on it."});

  widget->AddItem("Explain something",
                  {.help = "Three lines of help, which the footer of this "
                   "group shows while the cursor is on this item, and which "
                   "it loses again as soon as the cursor moves on: the group "
                   "below it moves up by about two lines."});

  widget->AddGroup("Targets");

  for (const char *caption : {"Tap me", "Tap me too", "And me"})
    widget->AddItem(caption, [caption]{
      ShowMessageBox(caption, _("Tap Test"), MB_OK);
    });

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Tap Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page which gives the check mark of a single select group to an
 * item which is hidden.  The mark belongs to one group, so it has to
 * leave the group of the hidden item alone and the group above it
 * untouched.
 */
static void
ShowHiddenCheckDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);
  GroupedListWidget &list = *widget;

  widget->AddHero("Hidden Check Test",
                  "The first item of the second group is hidden. Give the "
                  "check mark to it: the mark of that group has to "
                  "disappear, and the mark of the first group has to stay "
                  "where it is.");

  widget->AddGroup("Group 1", {.selection_mode = SelectionMode::SINGLE});
  widget->AddItem("Alpha", {.checked = true});
  widget->AddItem("Bravo", GroupedListWidget::ItemOptions{});

  widget->AddGroup("Group 2",
                   {.footer = "One item of this group is hidden, and it is "
                    "the first one.",
                    .selection_mode = SelectionMode::SINGLE});

  const unsigned hidden = widget->GetItemCount();
  widget->AddItem("Hidden", {.hidden = true});

  const unsigned charlie = widget->GetItemCount();
  widget->AddItem("Charlie", {.checked = true});
  widget->AddItem("Delta", GroupedListWidget::ItemOptions{});

  widget->AddGroup(nullptr);

  widget->AddItem("Check the hidden item", [&list, hidden](){
    list.SetItemChecked(hidden);
  });

  widget->AddItem("Check Charlie again", [&list, charlie](){
    list.SetItemChecked(charlie);
  });

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Hidden Check Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A row with a boolean value, for the page below: a view which edits
 * something and writes it back in Save().
 */
class BooleanRowWidget final : public RowFormWidget {
  bool &value;

public:
  BooleanRowWidget(const DialogLook &_look, bool &_value) noexcept
    :RowFormWidget(_look), value(_value) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    RowFormWidget::Prepare(parent, rc);
    AddBoolean("Editable", nullptr, value);
  }

  bool Save(bool &changed) noexcept override {
    /* SaveValue() says whether it has written a new value; its third
       parameter negates, it is not the flag of the dialog */
    changed |= SaveValue(0, value);
    return true;
  }
};

/** the value which the page below edits */
static bool save_probe = false;

/**
 * A page which puts a view that edits something into a widget group.
 * Closing it with Save has to write the value back, which only works
 * if the list hands Save() to the views inside it.
 */
static void
ShowWidgetSaveDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Widget Save Test",
                  "Flip the row in the group below and close the page with "
                  "Save. The message box has to show the new value.");

  widget->AddGroup(nullptr);
  widget->AddItem("Value before",
                  {.value = save_probe ? "on" : "off"});

  widget->AddWidgetGroup("Editable View",
                         std::make_unique<BooleanRowWidget>(look,
                                                            save_probe),
                         {.footer = "The row above belongs to a widget "
                          "group, not to the list."});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Widget Save Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Save"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  if (dialog.ShowModal() == mrOK)
    ShowMessageBox(save_probe ? "The value is now on" : "The value is now off",
                   dialog.GetChanged() ? _("Saved") : _("Nothing changed"),
                   MB_OK);
}

/**
 * Fill the page below.  It begins with an item which has no group of
 * its own, and it is built again from one of its items: the options
 * of the group which was open at the end of the previous build must
 * not reach that first item.
 */
static void
FillGroupOptionsPage(GroupedListWidget &list) noexcept
{
  list.Clear();

  list.AddItem("Before any group",
               {.subtitle = "This item has no group. It must stay without "
                "a check mark, and the footer of the group below must "
                "not appear under it."});

  list.AddGroup("Multi Select",
                {.footer = "This footer belongs to this group.",
                 .selection_mode = SelectionMode::MULTIPLE});

  list.AddItem("Alpha", {.checked = true});
  list.AddItem("Bravo", GroupedListWidget::ItemOptions{});

  list.AddGroup(nullptr);

  list.AddItem("Build the list again", [&list](){
    FillGroupOptionsPage(list);
    list.UpdateLayout();
  });
}

/**
 * A page which builds itself again, to show that a rebuild starts
 * from the same state as the first build.
 */
static void
ShowGroupOptionsDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Group Options Test",
                  "Build the list again with the last item. The page has "
                  "to look exactly as it does now.");

  FillGroupOptionsPage(*widget);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Group Options Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page with two cards which have to look the same.  The first one
 * holds a hidden item which carries a help text; that text can never
 * be shown, because the cursor cannot reach a hidden item, so it must
 * not open a footer below the card either.
 */
static void
ShowHiddenHelpDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Hidden Help Test",
                  "The two cards below have to keep the same distance to "
                  "the caption which follows them.");

  widget->AddGroup("With a hidden item");
  widget->AddItem("Alpha", GroupedListWidget::ItemOptions{});
  widget->AddItem("Bravo", GroupedListWidget::ItemOptions{});
  widget->AddItem("Hidden",
                  {.help = "A help text which nobody can bring onto the "
                   "screen.",
                   .hidden = true});

  widget->AddGroup("Without one");
  widget->AddItem("Alpha", GroupedListWidget::ItemOptions{});
  widget->AddItem("Bravo", GroupedListWidget::ItemOptions{});

  widget->AddGroup("The end");
  widget->AddItem("Nothing to do here", GroupedListWidget::ItemOptions{});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Hidden Help Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page for the work which a cursor movement causes.  The list is
 * measured again whenever the footer of a group changes, and that
 * happens only where an item explains itself; the long group below
 * has no such item and must not cost anything.
 */
static void
ShowCursorSpeedDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Cursor Speed Test",
                  "Walk through the long group: the cursor has to follow "
                  "without a delay. In the group below it the footer has "
                  "to change with every step.");

  widget->AddGroup("A hundred items, none of which explains itself");

  for (unsigned i = 1; i <= 100; ++i) {
    char caption[32], value[16];
    sprintf(caption, "%s %u", "Item", i);
    sprintf(value, "%u", i * 7);

    widget->AddItem(caption,
                    {.subtitle = "A second line, so that measuring this "
                     "item costs something",
                     .value = value});
  }

  widget->AddGroup("Items which do explain themselves",
                   {.footer = "The help of the item under the cursor "
                    "stands here."});

  widget->AddItem("Short", {.help = "One line."});

  widget->AddItem("Long",
                  {.help = "Three lines of help, so that the footer of "
                   "this group has to grow when the cursor arrives here, "
                   "and has to shrink again when it leaves."});

  widget->AddItem("None at all", GroupedListWidget::ItemOptions{});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Cursor Speed Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page which shows the options of a value: how many lines it may
 * use, its font and its size, beside the caption and below it.
 */
static void
ShowValueLinesDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Value Lines Test",
                  "One long line, several lines, and the same in the "
                  "small and the fixed-width font. A value ends after "
                  "four lines unless the item says otherwise, and a "
                  "description shows all of its lines unless the item "
                  "limits them.");

  static constexpr const char *long_line =
    "a value which is one long line without a break in it, and which "
    "the list has to fold into as many lines as it needs, or as many "
    "as the item allows, and then some more words so that four lines "
    "are not enough for it either";

  static constexpr const char *several_lines =
    "DE-WPT-Airports-OpenAIP.cupx\n"
    "schwarz_und_pfaelzerwald_jura_de.cupx\n"
    "CH-WPT-Airfields.cup\n"
    "AT-WPT-Outlandings.cup\n"
    "FR-WPT-Alpes.cup\n"
    "IT-WPT-Dolomiti.cup";

  static constexpr const char *path =
    "/private/var/mobile/Containers/Data/Application/"
    "A195033B-8E4C-4952-A27D-5A8AECB4B5F6/Documents/XCSoarData";

  static constexpr const char *description =
    "a description which says what the item is about: it stands over "
    "the whole width below the caption, it takes as many lines as it "
    "needs, and where the item allows only a few of them the last one "
    "ends with an ellipsis";

  static constexpr const char *subtitle =
    "a second line which needs more than one of them, so that it can "
    "be told apart from the description above it";

  static constexpr const char *notam =
    "A1234/26 NOTAMN\n"
    "Q) LSAS/QMRXX/IV/NBO/A/000/999/4654N00730E005\n"
    "A) LSZB B) 2609160600 C) 2609162000\n"
    "E) RWY 14/32 CLOSED DUE TO WORK IN PROGRESS.\n"
    "   HELICOPTER OPERATIONS UNAFFECTED.";

  widget->AddGroup("One long line",
                   {.footer = "The default folds it into four lines and "
                    "ends with an ellipsis."});

  widget->AddItem("Default", {.value = long_line});
  widget->AddItem("One line", {.value = long_line, .value_max_lines = 1});
  widget->AddItem("Two lines", {.value = long_line, .value_max_lines = 2});
  widget->AddItem("All lines", {.value = long_line, .value_all_lines = true});

  widget->AddGroup("Several lines",
                   {.footer = "Six file names, one per line; the limit "
                    "counts them like folded lines."});

  widget->AddItem("Default", {.value = several_lines});
  widget->AddItem("One line", {.value = several_lines, .value_max_lines = 1});
  widget->AddItem("All lines", {.value = several_lines,
                                .value_all_lines = true});

  widget->AddGroup("Small",
                   {.footer = "The same in the small font, which fits "
                    "more into a line."});

  widget->AddItem("One long line", {.value = long_line,
                                    .value_size = TextSize::SMALL,
                                    .value_all_lines = true});
  widget->AddItem("Several lines", {.value = several_lines,
                                    .value_size = TextSize::SMALL,
                                    .value_all_lines = true});
  widget->AddItem("Small, one line", {.value = several_lines,
                                      .value_size = TextSize::SMALL,
                                      .value_max_lines = 1});

  widget->AddGroup("Fixed width");

  widget->AddItem("Path", {.value = path, .value_font = TextFont::MONO});
  widget->AddItem("Path, small", {.value = path,
                                  .value_font = TextFont::MONO,
                                  .value_size = TextSize::SMALL,
                                  .value_all_lines = true});
  widget->AddItem("Path, one line", {.value = path,
                                     .value_font = TextFont::MONO,
                                     .value_max_lines = 1});

  widget->AddGroup("Below the caption");

  widget->AddItem("Path", {.value = path,
                           .value_below = true,
                           .value_font = TextFont::MONO});
  widget->AddItem("Path, small, all lines", {.value = path,
                                             .value_below = true,
                                             .value_font = TextFont::MONO,
                                             .value_size = TextSize::SMALL,
                                             .value_all_lines = true});
  widget->AddItem("Path, one line", {.value = path,
                                     .value_below = true,
                                     .value_font = TextFont::MONO,
                                     .value_max_lines = 1});
  widget->AddItem("Files, small", {.value = several_lines,
                                   .value_below = true,
                                   .value_size = TextSize::SMALL,
                                   .value_all_lines = true});

  widget->AddGroup("With a subtitle");

  widget->AddItem("Waypoints", {.subtitle = "the files which are loaded",
                                .value = several_lines,
                                .value_size = TextSize::SMALL,
                                .value_all_lines = true});
  widget->AddItem("Waypoints, below", {.subtitle = "the files which are "
                                       "loaded",
                                       .value = several_lines,
                                       .value_below = true,
                                       .value_size = TextSize::SMALL,
                                       .value_all_lines = true});
  widget->AddItem("Data path", {.subtitle = path,
                                .subtitle_font = TextFont::MONO});
  widget->AddItem("Data path, both", {.subtitle = path,
                                      .subtitle_font = TextFont::MONO,
                                      .value = "12 MB",
                                      .value_size = TextSize::SMALL});

  widget->AddGroup("With a description",
                   {.footer = "The description has the whole width below "
                    "the caption; a value beside the caption, the badge "
                    "and the arrow stay in the first line, and the "
                    "subtitle follows the description."});

  widget->AddItem("Default", {.description = description});
  widget->AddItem("Two lines", {.description = description,
                                .description_max_lines = 2});
  widget->AddItem("Several lines", {.description = several_lines});
  widget->AddItem("Small", {.description = description,
                            .description_size = TextSize::SMALL});
  widget->AddItem("Path, small", {.description = path,
                                  .description_font = TextFont::MONO,
                                  .description_size = TextSize::SMALL});
  widget->AddItem("Path, one line", {.description = path,
                                     .description_font = TextFont::MONO,
                                     .description_max_lines = 1});

  widget->AddGroup("Description and the rest");

  widget->AddItem("Waypoints", []{}, {.subtitle = "the files which are "
                                      "loaded",
                                      .value = "12 MB",
                                      .value_size = TextSize::SMALL,
                                      .description = several_lines,
                                      .description_size = TextSize::SMALL,
                                      .badge = "6 files",
                                      .chevron = true});
  widget->AddItem("Data path", []{}, {.subtitle = "where the files are",
                                      .value = "12 MB",
                                      .description = path,
                                      .description_font = TextFont::MONO,
                                      .description_size = TextSize::SMALL,
                                      .chevron = true});
  widget->AddItem("Files below", {.subtitle = subtitle,
                                  .value = several_lines,
                                  .value_below = true,
                                  .value_size = TextSize::SMALL,
                                  .value_all_lines = true,
                                  .description = description,
                                  .description_max_lines = 2,
                                  .badge = "below"});
  widget->AddItem("Toggle", []{}, {.description = path,
                                   .description_font = TextFont::MONO,
                                   .description_size = TextSize::SMALL,
                                   .toggle = true,
                                   .checked = true});
  widget->AddItem("Disabled", {.subtitle = "cannot be reached",
                               .value = "12 MB",
                               .description = description,
                               .description_max_lines = 2,
                               .badge = "off",
                               .disabled = true});

  widget->AddGroup("Without a caption",
                   {.footer = "An item whose caption is nullptr is its "
                    "description alone, the shape which the detail view "
                    "of a NOTAM needs. What stands at the right edge "
                    "keeps its own row above the text."});

  widget->AddItem(nullptr, {.description = notam,
                            .description_font = TextFont::MONO,
                            .description_size = TextSize::SMALL});
  widget->AddItem(nullptr, {.description = notam,
                            .description_font = TextFont::MONO,
                            .description_size = TextSize::SMALL,
                            .badge = "active"});
  widget->AddItem(nullptr, {.description = notam,
                            .description_font = TextFont::MONO,
                            .description_size = TextSize::SMALL,
                            .description_max_lines = 2});
  widget->AddItem(nullptr, {.description = description});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Value Lines Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/* the texts which the Item Padding Test combines: every one of them
   is there in a short and in a long version, and the page shows what
   the room above and below the text does as the item grows */
static constexpr const char *padding_demo_captions[] = {
  "Short caption",
  "A caption which is long enough that the list has to fold it into "
  "more than one line of its own",
};

static constexpr const char *padding_demo_values[] = {
  nullptr,
  "12 MB",
  "a value which leaves the caption too little room beside it, so that "
  "it moves below the caption and wraps there",
};

static constexpr const char *padding_demo_subtitles[] = {
  nullptr,
  "a second line",
  "a second line which is long enough to need more than one of them, "
  "and which says what the item is about",
};

static constexpr const char *padding_demo_descriptions[] = {
  nullptr,
  "a description of one line",
  "a description which runs over several lines: it has the whole width "
  "of the item, it says what the item is about, and it is the text "
  "which makes an item tall",
};

/**
 * A page which shows what the room above and below the text of an
 * item does as the item grows: first one text which grows line by
 * line, then every combination of a short and a long caption, value,
 * second line and description.
 */
static void
ShowItemPaddingDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Item Padding Test",
                  "A row of one line is a target for a finger and keeps "
                  "the room which makes it easy to hit. The more text an "
                  "item has, the tighter it sits: twice the text is half "
                  "the room, down to the padding which keeps a text off "
                  "the line below it. A group whose tall items are "
                  "targets too turns that off with "
                  "shrink_vertical_padding.");

  /* the same items twice: once with the room which follows the text,
     once with the room of a one-line row */
  for (const bool shrink : {true, false}) {
    widget->AddGroup(shrink
                     ? "A text which grows"
                     : "The same, without shrinking",
                     {.footer = shrink
                      ? "The same description, one line longer every "
                      "time. The room above and below it shrinks from "
                      "item to item and stops where it is least."
                      : "shrink_vertical_padding = false: every item "
                      "keeps the room of a one-line row, however tall "
                      "it is. Hold it against the group above.",
                      .shrink_vertical_padding = shrink});

    for (const unsigned lines : {1u, 2u, 3u, 5u, 8u, 16u}) {
      StaticString<32> caption;
      caption.Format("%u lines", lines);

      StaticString<512> text;
      text.clear();

      for (unsigned j = 1; j <= lines; ++j) {
        if (j > 1)
          text.push_back('\n');

        text.AppendFormat("line %u of %u", j, lines);
      }

      widget->AddItem(caption, {.description = text});
    }
  }

  /* every combination of the four texts: each of them is missing,
     short or long, with a short and with a long caption */
  for (unsigned c = 0; c < std::size(padding_demo_captions); ++c) {
    widget->AddGroup(c == 0 ? "A short caption" : "A long caption",
                     {.footer = c == 0
                      ? "The value, the second line and the description "
                      "are missing, short or long, in every combination "
                      "of the three."
                      : "The same combinations, with a caption which "
                      "needs more than one line."});

    for (const char *value : padding_demo_values)
      for (const char *subtitle : padding_demo_subtitles)
        for (const char *description : padding_demo_descriptions)
          widget->AddItem(padding_demo_captions[c],
                          {.subtitle = subtitle,
                           .value = value,
                           .description = description});
  }

  widget->AddGroup("With what stands at the right edge",
                   {.footer = "A badge, a switch and an arrow keep the "
                    "first row of the item, which may be taller than "
                    "the caption; the room above and below the block "
                    "follows the same rule."});

  widget->AddItem("Badge", {.value = "12 MB",
                            .description = padding_demo_descriptions[2],
                            .badge = "active"});
  widget->AddItem("Switch", []{}, {.subtitle = padding_demo_subtitles[2],
                                   .description =
                                   padding_demo_descriptions[2],
                                   .toggle = true,
                                   .checked = true});
  widget->AddItem("Arrow", []{}, {.subtitle = padding_demo_subtitles[1],
                                  .value = padding_demo_values[1],
                                  .description =
                                  padding_demo_descriptions[1],
                                  .chevron = true});
  widget->AddItem(padding_demo_captions[1],
                  []{}, {.subtitle = padding_demo_subtitles[2],
                         .value = padding_demo_values[2],
                         .description = padding_demo_descriptions[2],
                         .badge = "all of it",
                         .chevron = true});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Item Padding Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page which shows the texts that are broken into lines: the layout
 * and the paint have to agree about those lines, and a tap on a link
 * has to find the one under the finger.
 */
static void
ShowTextWrapDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Text Wrap Test",
                  "Every text below is broken into lines. Turn the device "
                  "to make the page wider and narrower: the lines have to "
                  "follow, and the links have to open what they say.");

  widget->AddGroup("Boxes which share the room",
                   {.footer = "The first link goes to [the page about "
                    "the map](https://xcsoar.readthedocs.io/en/latest/"
                    "users/map.html), the second one to [the page about "
                    "the weather](https://xcsoar.readthedocs.io/en/latest/"
                    "users/weather.html)."});

  widget->AddItem("A caption which is much too long for one line and has "
                  "to be broken into several of them",
                  {.value = "a value which is too long as well and gets "
                   "its own box beside the caption"});

  widget->AddItem("A short caption",
                  {.value = "/private/var/mobile/Containers/Data/"
                   "Application/A195033B-8E4C-4952-A27D-5A8AECB4B5F6/"
                   "Documents/XCSoarData",
                   .value_font = TextFont::MONO});

  widget->AddItem("A caption which goes on and on and on, so that it "
                  "does not fit into the four lines which an item "
                  "offers, and therefore ends with the sign that says "
                  "that there is more of it, which the reader will not "
                  "get to see here",
                  GroupedListWidget::ItemOptions{});

  widget->AddGroup("Below the caption");

  widget->AddItem("Log files",
                  {.value = "/private/var/mobile/Containers/Data/"
                   "Application/A195033B-8E4C-4952-A27D-5A8AECB4B5F6/"
                   "Documents/XCSoarData/logs",
                   .value_below = true,
                   .value_font = TextFont::MONO});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Text Wrap Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page with the four kinds of icon an item can have.  Looking for
 * an icon which is not there is the expensive case, and it has to
 * happen once, not on every step of the cursor.
 */
static void
ShowIconCacheDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Icon Cache Test",
                  "Walk through the group below. The icons have to stay as "
                  "they are, and the cursor has to move without a delay.");

  widget->AddGroup("Four kinds of icon",
                   {.footer = "The last item carries a character which no "
                    "font has. It gets no icon and keeps no room for one, "
                    "and looking for it is the work which used to happen "
                    "again with every step of the cursor."});

  widget->AddItem("From the resources of XCSoar",
                  {.icon = IDB_MOUNTAIN_TOP, .value = "icon"});

  widget->AddItem("An emoji",
                  {.icon_text = "🧹", .value = "color glyph"});

  widget->AddItem("A character which the font has",
                  {.icon_text = "★", .value = "glyph"});

  widget->AddItem("A character which no font has",
                  {.icon_text = "𓀀", .value = "nothing"});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Icon Cache Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page with icons which are more than one character.  A flag or a
 * family is wider than tall, and the square which the color glyph is
 * rendered into has to hold the whole of it.
 */
static void
ShowWideGlyphDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Wide Glyph Test",
                  "Every icon below has to be complete. A cut off flag or "
                  "a cut off family means that the square is too narrow "
                  "for what is drawn into it.");

  widget->AddGroup("One character",
                   {.footer = "These fit into the square as they are."});

  widget->AddItem("Broom", {.icon_text = "🧹", .value = "1"});
  widget->AddItem("Glider", {.icon_text = "🛩", .value = "1"});

  widget->AddGroup("Several characters",
                   {.footer = "A flag is a pair of letters, a family is a "
                    "chain of people; both are wider than one glyph."});

  widget->AddItem("Flag", {.icon_text = "🇨🇭", .value = "2"});
  widget->AddItem("Family", {.icon_text = "👨‍👩‍👧", .value = "5"});
  widget->AddItem("Two of them", {.icon_text = "🧹🛩", .value = "2"});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Wide Glyph Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page with nothing but switches.  The round ends of a switch are
 * circles, and a circle is painted with the pen of the canvas at its
 * rim: a pen of another color leaves dark pixels there.
 */
static void
ShowToggleEdgeDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Toggle Edge Test",
                  "Look closely at the round ends of the switches below. "
                  "No dark pixel may sit on the rim of one of them, and "
                  "the edge has to be as clean as the one of the track "
                  "between them.");

  widget->AddGroup("Off",
                   {.footer = "The knob is at the left end, on the gray "
                    "track."});

  widget->AddItem("First switch", {.toggle = true});
  widget->AddItem("Second switch", {.toggle = true});

  widget->AddGroup("On",
                   {.footer = "The knob is at the right end, on the green "
                    "track.  Move the cursor onto a row: the switch has "
                    "to look the same on the dark background."});

  widget->AddItem("Third switch", {.toggle = true, .checked = true});
  widget->AddItem("Fourth switch", {.toggle = true, .checked = true});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Toggle Edge Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page which simulates the Alternates dialog: the cursor picks a
 * waypoint, the buttons act on the one it is on, and one of them is
 * only available for a waypoint which has a radio frequency.
 */
static void
ShowActionBarDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Action Bar Test",
                  "Up and Down move the cursor and stay in the list, at "
                  "its ends as well. Right and Left both lead out of the "
                  "list to the button which was used last, whichever of "
                  "the two you press; only once the focus is on a button "
                  "do they walk along the bar, and after the last one it "
                  "comes back to the list. Up or Down brings it back from "
                  "anywhere. Try it: step out, walk to \"Close\", come "
                  "back with Up, and step out again - you are on "
                  "\"Close\". Enter opens the waypoint under the cursor "
                  "while the list has the focus, and presses the button "
                  "which has it otherwise. Page Up and Page Down move the "
                  "cursor by one screen. \"Frequency\" is only there for "
                  "a waypoint which has one, and the focus jumps over it "
                  "while it is not.");

  widget->AddGroup("Alternates",
                   {.footer = "The list is longer than the screen: try "
                    "the buttons from the middle of it, where neither "
                    "end is in reach."});

  struct Alternate {
    const char *name, *icao, *frequency, *distance;
  };

  static constexpr Alternate alternates[] = {
    {"Bern-Belp", "LSZB", "121.150 MHz", "12 km"},
    {"Thun", "LSZW", "123.500 MHz", "18 km"},
    {"Reichenbach", "LSGR", nullptr, "24 km"},
    {"Zweisimmen", "LSTZ", "119.250 MHz", "31 km"},
    {"Saanen", "LSGK", "127.325 MHz", "38 km"},
    {"Gsteig", "LSXG", nullptr, "44 km"},
    {"Lauenen", "LSXL", nullptr, "49 km"},
    {"Boltigen", "LSXB", "118.900 MHz", "55 km"},
    {"Erlenbach", "LSXE", nullptr, "58 km"},
    {"Spiez", "LSXS", "122.075 MHz", "61 km"},
    {"Interlaken", "LSMI", "130.400 MHz", "66 km"},
    {"Meiringen", "LSMM", "134.125 MHz", "72 km"},
    {"Grindelwald", "LSXR", nullptr, "77 km"},
    {"Lauterbrunnen", "LSXU", nullptr, "81 km"},
    {"Kandersteg", "LSXK", nullptr, "86 km"},
    {"Adelboden", "LSXA", "118.375 MHz", "89 km"},
    {"Frutigen", "LSXF", nullptr, "93 km"},
    {"Wimmis", "LSXW", "126.650 MHz", "97 km"},
    {"Gstaad", "LSGG", "125.800 MHz", "104 km"},
    {"Chateau-d'Oex", "LSGD", nullptr, "111 km"},
    {"Bulle", "LSGU", "131.075 MHz", "118 km"},
    {"Gruyeres", "LSXY", nullptr, "124 km"},
    {"Fribourg", "LSGF", "120.225 MHz", "132 km"},
    {"Payerne", "LSMP", "132.300 MHz", "141 km"},
  };

  for (const auto &alternate : alternates)
    widget->AddItem(alternate.name, [name = alternate.name](){
      ShowMessageBox(name, _("Action Bar Test"), MB_OK);
    }, {.subtitle = alternate.frequency != nullptr
        ? alternate.frequency
        : "no radio frequency",
        .value = alternate.distance,
        .badge = alternate.icao,
        /* four letters which are read one by one and compared from
           one item to the next */
        .badge_font = TextFont::MONO,
        .chevron = true,
        .help = alternate.frequency != nullptr
        ? "Enter opens this airfield; \"Frequency\" tunes the radio."
        : "No radio frequency is known for this airfield."});

  GroupedListWidget &list = *widget;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Action Bar Test"));
  dialog.FinishPreliminary(std::move(widget));

  dialog.AddButton(_("GoTo"), [](){
    ShowMessageBox("GoTo", _("Action Bar Test"), MB_OK);
  });

  Button *const frequency_button = dialog.AddButton(_("Frequency"), [](){
    ShowMessageBox("Frequency", _("Action Bar Test"), MB_OK);
  });

  dialog.AddButton(_("Close"), mrOK);

  /* the button is only available for a waypoint which has a
     frequency, as it is in the Alternates dialog */
  list.SetCursorCallback([frequency_button](int index){
    frequency_button->SetEnabled(index >= 0 &&
                                 index < (int)std::size(alternates) &&
                                 alternates[index].frequency != nullptr);
  });

  frequency_button->SetEnabled(alternates[0].frequency != nullptr);

  list.SetActionBar(dialog.GetButtonPanel());
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}

/**
 * A page about what a tap does where.  A switch is a control of its
 * own, and the rest of the item is only its label.
 */
static void
ShowHitAreaDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Hit Area Test",
                  "Tap the caption of an item, then its switch. Tap "
                  "beside the items, on a caption or on this card: the "
                  "cursor goes away, and with it the explanation below "
                  "the group. Enter flips the switch of the item under "
                  "the cursor, no matter which of the two it is.");

  widget->AddGroup("Where a tap flips the switch",
                   {.footer = "The first one is the default."});

  widget->AddItem("Only the switch",
                  {.toggle = true, .checked = true,
                   .help = "A tap on the caption moves the cursor here and "
                   "shows this text; only a tap on the switch itself "
                   "flips it."});

  widget->AddItem("Anywhere on the item",
                  {.toggle = true,
                   .toggle_hit_area = ToggleHitArea::ROW,
                   .help = "A tap anywhere on this item flips the switch."});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Hit Area Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page where Enter means two different things: on the items of one
 * group it runs their own callback, on the items of the other it
 * presses the button which the action bar has marked.
 */
static void
ShowEnterActionDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("With a finger",
                  "In the first group one tap acts: the item answers, or "
                  "its switch flips. In the second group the first tap "
                  "only chooses the item - the buttons below act on the "
                  "one which is chosen - and the second tap on the same "
                  "item opens it. A tap on a switch flips it in both "
                  "groups: a switch is a control, not a choice.");

  widget->AddHero("With a keyboard",
                  "Up and Down move the cursor. Enter runs the item in "
                  "the first group; in the second one it presses the "
                  "button which is marked, and that button names the item "
                  "the cursor is on. Space always runs the item itself. "
                  "Right and Left lead to the buttons and walk along "
                  "them, Up and Down come back to the list.");

  widget->AddGroup("Enter belongs to the item",
                   {.footer = "This is what a page of settings needs; the "
                    "default."});

  widget->AddItem("Show a message", [](){
    ShowMessageBox("The item did it", _("Enter Action Test"), MB_OK);
  }, {.value = "callback",
      .help = "Enter runs the callback of this item, and so does one "
      "tap on it. The buttons still act on it while it is the item "
      "under the cursor."});

  widget->AddItem("A switch", {.toggle = true, .checked = true,
                               .help = "Enter flips it, and so does a tap "
                               "on the switch itself."});

  widget->AddGroup("Enter belongs to the action bar",
                   {.footer = "A group whose items are nothing but a "
                    "choice, as in the Alternates dialog.",
                    .enter_action = EnterAction::ACTION_BAR});

  struct Choice {
    const char *caption, *help;
  };

  static constexpr Choice choices[] = {
    {"Bern-Belp",
     "Enter presses the marked button, which acts on this item. Space "
     "opens the item itself. With a finger: one tap chooses it, the next "
     "one opens it."},
    {"Thun",
     "The same here. Tap this one while the cursor is on Bern-Belp: only "
     "the cursor moves, and the buttons follow it."},
    {"Zweisimmen",
     "And here. The buttons name the item the cursor is on, so they show "
     "which one a tap has chosen."},
  };

  for (const auto &choice : choices)
    widget->AddItem(choice.caption, [caption = choice.caption](){
      ShowMessageBox(caption, _("Enter Action Test"), MB_OK);
    }, {.value = "choice", .help = choice.help});

  GroupedListWidget &list = *widget;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Enter Action Test"));
  dialog.FinishPreliminary(std::move(widget));

  /* the buttons act on the item the cursor is on, whichever group it
     belongs to */
  const auto show = [&list](const char *what){
    StaticString<64> text;
    const int index = list.GetCursorIndex();
    text.Format("%s: item %d", what, index);
    ShowMessageBox(text, _("Enter Action Test"), MB_OK);
  };

  dialog.AddButton(_("GoTo"), [show](){ show("GoTo"); });
  dialog.AddButton(_("Details"), [show](){ show("Details"); });
  dialog.AddButton(_("Close"), mrOK);

  list.SetActionBar(dialog.GetButtonPanel());
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}

/** One glider of the "Disabled Item Test" page. */
struct DisabledDemoItem {
  const char *caption, *value, *help;

  /** may the cursor rest on it while it is not available? */
  bool selectable;

  bool disabled;
};

static DisabledDemoItem disabled_demo_items[] = {
  {"Discus", "8.2 km",
   "This one is available: the cursor starts here and comes back here.",
   false, false},
  {"Vega", "16.7 km",
   "This text cannot be read while the item is not available: the cursor "
   "never gets here.",
   false, true},
  {"Altair", "19.2 km",
   "Neither can this one, for the same reason.",
   false, true},
  {"Ventus", "21.0 km",
   "Available as well, for comparison.",
   true, false},
  {"Deneb", "23.4 km",
   "The cursor may rest on this item, which shows why it is not "
   "available; Enter still does nothing, but \"Enable\" can be aimed at "
   "it.",
   true, true},
  {"Rigel", "27.9 km",
   "This one can be reached too, and \"Enable\" acts on whichever of the "
   "two the cursor is on.",
   true, true},
};

static void
FillDisabledItemPage(GroupedListWidget &list) noexcept
{
  list.Clear();

  list.AddHero("Disabled Item Test",
               "Walk through the list with Up and Down: in the first group "
               "the cursor only stops on the item at the top, in the second "
               "one it stops on all three. Enter does nothing on an item "
               "which is not available, but its explanation can be read, "
               "and \"Enable\" is only there while the cursor is on one of "
               "them. Press it: the item becomes available, and the button "
               "goes away with it.");

  list.AddGroup("The cursor skips these",
                {.footer = "This is what an item which is not available "
                 "does by default."});

  for (std::size_t i = 0; i < std::size(disabled_demo_items); ++i) {
    const auto &item = disabled_demo_items[i];

    if (i == 3)
      list.AddGroup("The cursor stops here",
                    {.footer = "The two below the first one carry "
                     "selectable_when_disabled."});

    list.AddItem(item.caption, {.value = item.value,
                                .help = item.help,
                                .disabled = item.disabled,
                                .selectable_when_disabled = item.selectable});
  }
}

/**
 * A page with items which are not available.  The cursor skips such
 * an item, unless it is told to stop there: a button which makes the
 * item available again could not be aimed at it otherwise.
 */
static void
ShowDisabledItemDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  /* the page is opened again and again while testing; every one of
     them starts from the same state: the first item of each group is
     available, the two below it are not */
  for (std::size_t i = 0; i < std::size(disabled_demo_items); ++i)
    disabled_demo_items[i].disabled = i != 0 && i != 3;

  auto widget = std::make_unique<GroupedListWidget>(look);

  GroupedListWidget &list = *widget;
  FillDisabledItemPage(list);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Disabled Item Test"));
  dialog.FinishPreliminary(std::move(widget));

  Button *const enable_button = dialog.AddButton(_("Enable"), [](){});

  dialog.AddButton(_("Close"), mrOK);

  /* the button is only there for an item which is not available and
     which the cursor can nevertheless reach */
  const auto update = [enable_button](int index){
    enable_button->SetEnabled(index >= 0 &&
                              index < (int)std::size(disabled_demo_items) &&
                              disabled_demo_items[index].disabled &&
                              disabled_demo_items[index].selectable);
  };

  enable_button->SetCallback([&list, update](){
    const int index = list.GetCursorIndex();
    if (index < 0 || index >= (int)std::size(disabled_demo_items))
      return;

    disabled_demo_items[index].disabled = false;

    /* the list is built again; Clear() has saved the cursor, and the
       item it was on keeps it */
    FillDisabledItemPage(list);
    list.UpdateLayout();

    update(list.GetCursorIndex());
  });

  list.SetCursorCallback(update);
  update(list.GetCursorIndex());

  list.SetActionBar(dialog.GetButtonPanel());
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}

/**
 * A page about the second tap.  In a group whose items are a choice,
 * a tap chooses; only a tap on the item which is already chosen
 * activates it.  Every kind of decoration is on it once, because each
 * of them answers a tap in its own way.
 */
static void
ShowSecondTapDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Second Tap Test",
                  "Two groups with the same items. In the first one every "
                  "tap acts at once, in the second one the first tap only "
                  "chooses the item and the second one acts. Enter and "
                  "Space say the same thing in the language of a keyboard: "
                  "in the second group Enter presses the marked button, "
                  "and Space acts on the item. Every item explains below "
                  "the group what to expect from it.");

  widget->AddGroup("Every tap acts",
                   {.footer = "The default: enter_action = ITEM.",
                    .selection_mode = SelectionMode::SINGLE});

  widget->AddItem("Open a page", [](){
    ShowMessageBox("Opened", _("Second Tap Test"), MB_OK);
  }, {.chevron = true,
      .help = "One tap opens it, wherever the cursor was. Enter does the "
      "same, and so does Space."});

  widget->AddItem("Set the check mark",
                  {.checked = true,
                   .help = "One tap moves the check mark here. The check "
                   "mark is set by activating the item, so it follows the "
                   "same rule as the callback above."});

  widget->AddItem("A switch",
                  {.toggle = true,
                   .help = "The switch is a control of its own: a tap on "
                   "it flips it at once. A tap on the caption only moves "
                   "the cursor here, because toggle_hit_area is SWITCH."});

  widget->AddItem("A switch over the whole item",
                  {.toggle = true,
                   .toggle_hit_area = ToggleHitArea::ROW,
                   .checked = true,
                   .help = "With toggle_hit_area = ROW a tap anywhere on "
                   "the item flips the switch, in this group at once."});

  widget->AddGroup("The first tap chooses",
                   {.footer = "enter_action = ACTION_BAR. The buttons act "
                    "on the item the cursor is on, so the finger must be "
                    "able to point at one without setting it off.",
                    .selection_mode = SelectionMode::SINGLE,
                    .enter_action = EnterAction::ACTION_BAR});

  widget->AddItem("Open a page", [](){
    ShowMessageBox("Opened", _("Second Tap Test"), MB_OK);
  }, {.chevron = true,
      .help = "Tap it while the cursor is elsewhere: only the cursor "
      "moves. Tap it again: now it opens. Enter presses the marked "
      "button instead; Space opens it."});

  widget->AddItem("Set the check mark",
                  {.help = "The first tap chooses this item, the second "
                   "one sets the check mark. A group which is filled and "
                   "checked with a finger is easier with the default "
                   "above."});

  widget->AddItem("A switch",
                  {.toggle = true,
                   .help = "The switch does not wait for a second tap: it "
                   "is a control, not a choice. A tap on it flips it and "
                   "moves the cursor here. A tap on the caption only "
                   "chooses the item."});

  widget->AddItem("A switch over the whole item",
                  {.toggle = true,
                   .toggle_hit_area = ToggleHitArea::ROW,
                   .checked = true,
                   .help = "Here the whole item is the switch, and the "
                   "item is a choice: the first tap chooses it, the second "
                   "one flips the switch."});

  GroupedListWidget &list = *widget;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Second Tap Test"));
  dialog.FinishPreliminary(std::move(widget));

  const auto show = [&list](const char *what){
    StaticString<64> text;
    text.Format("%s: item %d", what, list.GetCursorIndex());
    ShowMessageBox(text, _("Second Tap Test"), MB_OK);
  };

  dialog.AddButton(_("GoTo"), [show](){ show("GoTo"); });
  dialog.AddButton(_("Details"), [show](){ show("Details"); });
  dialog.AddButton(_("Close"), mrOK);

  list.SetActionBar(dialog.GetButtonPanel());
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}

/**
 * A page about the margin the view keeps beyond the cursor while the
 * keys move it, over items of every height, and about the two cases
 * in which the margin has to give way: an element taller than the
 * room it may take, and the end of the list.
 */
static void
ShowLookAheadDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Look Ahead Test",
                  "Walk through the list with Up and Down or with the "
                  "stick. The view starts to move before the cursor is at "
                  "the edge: the next items stay on the screen, and with "
                  "them whatever lies between them. How many of them fit "
                  "is a question of the room they ask for - two of them "
                  "on this page, one on a window which shows fewer items, "
                  "such as a landscape one. The items here are of every "
                  "height on purpose. Compare the two directions: the "
                  "margin belongs to the one the cursor is going.");

  widget->AddGroup("Items of every height",
                   {.footer = "One line, two lines, a caption which needs "
                    "three of them, a switch: the margin is measured in "
                    "items and in the room they take, not in rows."});

  widget->AddItem("Bern-Belp", {.subtitle = "122.150 MHz",
                                .value = "12 km",
                                .badge = "LSZB"});

  widget->AddItem("Grenchen", {.value = "24 km"});

  widget->AddItem("A caption long enough to need more than one line of "
                  "its own, which makes this item the tall one of the "
                  "group",
                  {.subtitle = "and a subtitle below it",
                   .value = "31 km"});

  widget->AddItem("Langenthal", {.value = "37 km", .badge = "LSPL"});

  widget->AddItem("A switch", {.toggle = true, .checked = true});

  widget->AddItem("Thun", {.subtitle = "119.800 MHz", .value = "44 km"});

  widget->AddItem("Interlaken", {.value = "52 km", .badge = "LSMI"});

  widget->AddItem("Another caption of the long kind, so that a tall item "
                  "follows a short one and the margin has to grow",
                  {.value = "58 km"});

  widget->AddItem("Reichenbach", {.subtitle = "123.500 MHz",
                                  .value = "63 km"});

  widget->AddItem("Zweisimmen", {.value = "71 km", .badge = "LSTZ"});

  widget->AddGroup("Over the edge of a group",
                   {.footer = "Between the two groups there is a footer, "
                    "a gap and a caption. They are no items and do not "
                    "count, but they come along with the margin."});

  widget->AddItem("Saanen", {.subtitle = "126.500 MHz", .value = "84 km"});

  widget->AddItem("Gruyeres", {.value = "97 km", .badge = "LSTS"});

  widget->AddItem("Ecuvillens", {.subtitle = "119.375 MHz",
                                 .value = "103 km",
                                 .badge = "LSGE"});

  widget->AddHero("A card taller than half of the view",
                  "The first item ahead may take half of the screen, and "
                  "the second one only as much as keeps the two of them "
                  "within a fifth of it. This card is taller than half, "
                  "so the view does not go looking for it and the cursor "
                  "stops at the edge instead, as it did before. Whatever "
                  "happens, the item under the cursor stays on the screen "
                  "as a whole - it is worth more than the look ahead.");

  widget->AddGroup("After the card",
                   {.footer = "At the end of the list there is nothing "
                    "left to look ahead to, and the last item comes to "
                    "rest at the lower edge of the view."});

  widget->AddItem("Bex", {.value = "118 km"});

  widget->AddItem("Sion", {.subtitle = "119.700 MHz",
                           .value = "126 km",
                           .badge = "LSGS"});

  widget->AddItem("Turtmann", {.value = "141 km"});

  widget->AddItem("Raron", {.subtitle = "125.400 MHz", .value = "149 km"});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Look Ahead Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * A page about what a finger does.  A press is not yet a choice: the
 * item goes grey, but the cursor only follows when the finger is
 * lifted where it came down.
 */
static void
ShowTouchDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Touch Test",
                  "Press an item and hold: it turns grey, and the item "
                  "which was selected before keeps its color. Drag up or "
                  "down without letting go: the grey goes away, nothing "
                  "is selected, and the selection you had is still there. "
                  "Let go where you came down: only now does the cursor "
                  "move there, and the item is activated. Tap beside the "
                  "items, on a caption or on this card, and the selection "
                  "goes away altogether.");

  widget->AddGroup("Scroll me",
                   {.footer = "Long enough to need a scroll gesture. "
                    "Select one of them first, then drag from another "
                    "one: the selection must not move."});

  for (unsigned i = 1; i <= 20; ++i) {
    StaticString<32> caption;
    caption.Format("Item %u", i);

    StaticString<32> value;
    value.Format("%u m", i * 137);

    widget->AddItem(caption, {.value = value});
  }

  widget->AddGroup("A switch",
                   {.footer = "The label selects, the switch flips."});

  widget->AddItem("Auto zoom",
                  {.toggle = true, .checked = true,
                   .help = "Press the caption and drag: neither the "
                   "cursor nor the switch moves."});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Touch Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/* the state which the Button Test page works on: the switch decides
   whether the button below it can be pressed */
static bool button_demo_auto_update = true;
static unsigned button_demo_presses = 0;

/* the countries which the Collapsible Test page shows, with the
   files each of them has, as the download list would have them */
static constexpr struct {
  const char *country;
  const char *files;
  struct {
    const char *name;
    const char *size;
    const char *badge;
    bool checked;
  } children[3];
} collapsible_demo_countries[] = {
  {"Switzerland", "3 files", {
    {"Alps 2026", "1.2 MB", nullptr, true},
    {"Outlandings", "0.3 MB", "new", false},
    {"Airfields", "0.1 MB", nullptr, true},
  }},
  {"Germany", "2 files", {
    {"Alpen Ost", "2.1 MB", nullptr, false},
    {"Mitteldeutschland", "1.8 MB", "outdated", false},
    {nullptr, nullptr, nullptr, false},
  }},
  {"France", "2 files", {
    {"Alpes du Sud", "0.9 MB", nullptr, true},
    {"Massif Central", "1.1 MB", nullptr, false},
    {nullptr, nullptr, nullptr, false},
  }},
};

/* the waypoint files which the last section of the Collapsible Test
   page shows: every country is a card of its own, the country is the
   item which opens it, and the files of that country are its
   children.  This is the shape which the waypoint download list
   would have */
static constexpr struct {
  const char *country;
  struct {
    const char *name;
    const char *size;
    const char *badge;
    bool checked;
  } files[12];
} waypoint_demo_countries[] = {
  {"Switzerland", {
    {"swiss_airfields_2026.cup", "96 kB", nullptr, true},
    {"swiss_outlandings_2026.cup", "142 kB", "new", false},
    {"swiss_turnpoints_2026.cup", "88 kB", nullptr, false},
    {"welt2000_ch.cup", "61 kB", nullptr, true},
    {"alps_west.cup", "214 kB", nullptr, false},
    {"jura.cup", "37 kB", nullptr, false},
    {"engadin.cup", "45 kB", nullptr, false},
  }},
  {"Germany", {
    {"dhv_gelaende_2026.cup", "310 kB", nullptr, true},
    {"welt2000_de.cup", "182 kB", nullptr, false},
    {"alpen_ost.cup", "126 kB", "outdated", false},
    {"schwarzwald.cup", "54 kB", nullptr, false},
    {"eifel.cup", "41 kB", nullptr, false},
    {"harz.cup", "38 kB", nullptr, false},
    {"rhoen.cup", "36 kB", nullptr, false},
    {"erzgebirge.cup", "44 kB", nullptr, false},
    {"bayern_nord.cup", "72 kB", nullptr, false},
    {"bayern_sued.cup", "91 kB", nullptr, true},
    {"nordsee.cup", "29 kB", nullptr, false},
    {"ostsee.cup", "31 kB", nullptr, false},
  }},
  {"France", {
    {"ffvp_2026.cup", "268 kB", "new", false},
    {"alpes_du_sud.cup", "118 kB", nullptr, true},
    {"massif_central.cup", "97 kB", nullptr, false},
    {"pyrenees.cup", "83 kB", nullptr, false},
    {"vosges.cup", "46 kB", nullptr, false},
    {"provence.cup", "64 kB", nullptr, false},
    {"bretagne.cup", "39 kB", nullptr, false},
    {"jura_fr.cup", "42 kB", nullptr, false},
    {"welt2000_fr.cup", "157 kB", nullptr, false},
  }},
  {"Austria", {
    {"oesterreich_2026.cup", "204 kB", nullptr, true},
    {"tirol.cup", "88 kB", nullptr, false},
    {"steiermark.cup", "67 kB", nullptr, false},
    {"salzkammergut.cup", "51 kB", nullptr, false},
    {"welt2000_at.cup", "73 kB", nullptr, false},
  }},
};

/**
 * Fill the page about the items which open and close.  Every group is
 * one case: several open at once, one at a time, and a group which
 * the cursor opens as it walks through it.
 */
static void
FillCollapsiblePage(GroupedListWidget &list) noexcept
{
  list.Clear();

  list.AddHero("Collapsible Test",
               "An item which has children opens and closes instead of "
               "acting: Enter, Space or a tap on it. A child wears a bar "
               "in the color of the brand at the left edge of the card "
               "instead of an indent, which leaves its caption the whole "
               "width, and it carries everything an item can carry, the "
               "check mark included. The arrow at the right edge points "
               "down while the item is closed and up while it is open, "
               "so that it is never mistaken for the arrow of an item "
               "which opens another page.");

  list.AddGroup("Several at a time",
                {.footer = "The default: what you open stays open. The "
                 "files can be checked one by one, and a dialog would "
                 "read the check marks when it closes. The countries "
                 "carry none: activating them opens them.",
                 .selection_mode = SelectionMode::MULTIPLE});

  for (const auto &country : collapsible_demo_countries) {
    list.AddItem(country.country, {.value = country.files});

    for (const auto &child : country.children) {
      if (child.name == nullptr)
        break;

      list.AddChildItem(child.name, {.value = child.size,
                                     .badge = child.badge,
                                     .checked = child.checked});
    }
  }

  list.AddGroup("One at a time",
                {.footer = "expand_mode = SINGLE. Opening one closes the "
                 "other two, and the row under the cursor stays where it "
                 "is on the screen while that happens. The children are "
                 "a single choice: checking one unchecks the others, "
                 "across all three cards of this group.",
                 .selection_mode = SelectionMode::SINGLE,
                 .expand_mode = ExpandMode::SINGLE});

  list.AddItem("Metric", {.value = "3 units"});
  list.AddChildItems({
    {"Kilometers", {}, {.value = "km", .checked = true}},
    {"Meters", {}, {.value = "m"}},
    {"Kilometers per hour", {}, {.value = "km/h"}},
  });

  list.AddItem("Imperial", {.value = "3 units"});
  list.AddChildItems({
    {"Statute miles", {}, {.value = "mi"}},
    {"Feet", {}, {.value = "ft"}},
    {"Miles per hour", {}, {.value = "mph"}},
  });

  list.AddItem("Nautical", {.value = "3 units"});
  list.AddChildItems({
    {"Nautical miles", {}, {.value = "NM"}},
    {"Flight levels", {}, {.value = "FL"}},
    {"Knots", {}, {.value = "kt"}},
  });

  list.AddGroup("The cursor opens them",
                {.footer = "expand_trigger = CURSOR, which is what a "
                 "menu of pages wants: Up and Down walk through "
                 "everything, what the cursor arrives at opens, and "
                 "what it leaves closes. Enter or a second tap closes "
                 "the one the cursor is on, and coming back to it "
                 "opens it again.",
                 .expand_mode = ExpandMode::SINGLE,
                 .expand_trigger = ExpandTrigger::CURSOR});

  list.AddItem("Site Files", {.value = "3 files"});
  list.AddChildItems({
    {"Map database", {}, {.value = "alps.xcm"}},
    {"Waypoints", {}, {.value = "alps.cup"}},
    {"Airspaces", {}, {.value = "alps.txt"}},
  });

  list.AddItem("Devices", {.value = "2 of them"});
  list.AddChildItems({
    {"Vega V7", {}, {.value = "connected"}},
    {"FLARM", {}, {.value = "no device"}},
  });

  list.AddItem("A page without children", {.chevron = true});

  list.AddItem("Weather", {.value = "4 files"});
  list.AddChildItems({
    {"RASP", {}, {.chevron = true}},
    {"Skysight", {}, {.chevron = true}},
    {"pc_met", {}, {.chevron = true}},
    {"NOAA", {}, {.chevron = true}},
  });

  list.AddGroup("More than fits",
                {.footer = "This one has so many children that they do "
                 "not fit below it: opening it scrolls as far as it "
                 "must, but never so far that the item itself leaves "
                 "the top of the view."});

  list.AddItem("Turn points of the Alps", {.value = "24 files"});

  for (unsigned i = 1; i <= 24; ++i) {
    StaticString<32> name, size;
    name.Format("Sector %u", i);
    size.Format("%u kB", 40 + i * 7);

    list.AddChildItem(name, {.value = size});
  }

  /* one card per country, the way the waypoint download list would
     have it: the country opens, its files stand below it, and the
     next country begins a card of its own */
  for (unsigned c = 0; c < std::size(waypoint_demo_countries); ++c) {
    const auto &country = waypoint_demo_countries[c];

    unsigned count = 0;
    while (count < std::size(country.files) &&
           country.files[count].name != nullptr)
      ++count;

    StaticString<16> files;
    files.Format("%u files", count);

    const bool last = c + 1 == std::size(waypoint_demo_countries);

    list.AddGroup(c == 0 ? "Waypoint files" : nullptr,
                  {.footer = last
                   ? "Every country is a card of its own, and the "
                   "files which can be downloaded for it are the "
                   "children of the country. The check marks say what "
                   "a Download button would fetch, one country at a "
                   "time."
                   : nullptr,
                   .selection_mode = SelectionMode::MULTIPLE});

    list.AddItem(country.country, {.value = files});

    for (const auto &file : country.files) {
      if (file.name == nullptr)
        break;

      list.AddChildItem(file.name, {.value = file.size,
                                    .badge = file.badge,
                                    .checked = file.checked});
    }
  }
}

static void
ShowCollapsibleDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  GroupedListWidget &list = *widget;
  FillCollapsiblePage(list);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Collapsible Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

/**
 * Fill the page about the buttons which belong to a group.  Every
 * group of it is one case: a button at the end of a card, a button
 * which cuts a card in two, buttons side by side, buttons below each
 * other, a button which cannot be pressed, and one which stands for
 * itself.  A button which changes something fills the page again, the
 * way a settings page would.
 */
static void
FillButtonPage(GroupedListWidget &list) noexcept
{
  list.Clear();

  list.AddHero("Button Test",
               "A button ends the card of its group and stands below it, "
               "because it needs the background around it. It belongs to "
               "the group all the same: the caption is above it and the "
               "footer below it. Every group below is one case to look "
               "at. Walk through them with Up and Down, and along a row "
               "of buttons with Left and Right.");

  list.AddGroup("A button at the end",
                {.footer = "The card keeps its round corners at the "
                 "bottom: nothing of this group follows the button, so "
                 "there is nothing the card leads to."});

  list.AddItem("Terrain", {.toggle = true, .checked = true});
  list.AddItem("Topography", {.toggle = true, .checked = true});

  list.AddButton("Reload the map file", [](){
    ShowMessageBox("The map would be read again now.",
                   _("Button Test"), MB_OK);
  });

  list.AddGroup("A button in the middle",
                {.footer = "Three cards, one group. The corners at the "
                 "two cuts are barely rounded, the first and the last "
                 "corner of the group are fully rounded. The first button "
                 "carries a description, the second one does not: both "
                 "keep the same distance to the card below them."});

  list.AddItem("Airspace", {.value = "alps.txt", .chevron = true});
  list.AddItem("Waypoints", {.value = "alps.cup", .chevron = true});

  list.AddButton("Download again", [](){
    ShowMessageBox("The files would be downloaded now.",
                   _("Button Test"), MB_OK);
  }, {.description = "A button with a description, in the middle of a "
      "group. The text belongs to the button and is always there."});

  list.AddItem("Weather stations", {.toggle = true});
  list.AddItem("Traffic", {.toggle = true, .checked = true});

  list.AddButton("Forget the downloads", [](){
    ShowMessageBox("The files would be deleted now.",
                   _("Button Test"), MB_OK);
  });

  list.AddItem("Thermal hotspots", {.toggle = true});

  list.AddGroup("Two of them side by side",
                {.footer = "They share the width of a card. Where the "
                 "captions do not fit next to each other, they stand "
                 "below each other instead; try it with -small."});

  list.AddButtonRow({
    {"Apply to page", [](){
      ShowMessageBox("Applied.", _("Button Test"), MB_OK);
    }},
    {"Add page", [](){
      ShowMessageBox("Added.", _("Button Test"), MB_OK);
    }},
  });

  list.AddButtonRow({
    {"Yesterday", [](){}},
    {"Today", [](){}},
    {"Tomorrow", [](){}},
  }, {.description = "A description belongs to its row and is always on "
      "the screen, whether the cursor is on it or not. The footer of the "
      "group stands below it."});

  list.AddGroup("One below the other",
                {.footer = "Two buttons which do not belong together are "
                 "added one by one and stand right below each other."});

  StaticString<64> pressed;
  pressed.Format("Pressed %u times", button_demo_presses);

  list.AddButton(pressed, [&list](){
    ++button_demo_presses;
    FillButtonPage(list);
    list.UpdateLayout();
  }, {.description = "This one counts its presses and fills the list "
      "again, which is how a page keeps a button up to date. The cursor "
      "stays where it is, on this button."});

  list.AddButton("Nothing to do here", [](){}, {.disabled = true});

  list.AddGroup("When it cannot be pressed",
                {.footer = "The switch decides whether the button below "
                 "can be pressed. A button which is greyed out says in "
                 "its description why."});

  list.AddItem("Auto update", [&list](){
    button_demo_auto_update = !button_demo_auto_update;
    FillButtonPage(list);
    list.UpdateLayout();
  }, {.toggle = true,
      .toggle_hit_area = ToggleHitArea::ROW,
      .checked = button_demo_auto_update});

  list.AddButton("Precache day", [](){
    ShowMessageBox("The day would be downloaded now.",
                   _("Button Test"), MB_OK);
  }, {.description = "Downloads the whole day at once, so the forecast "
      "is there without a network. Not available while Auto update is "
      "on: flip the switch above and watch the button.",
      .disabled = button_demo_auto_update});

  list.AddGroup(nullptr);
  list.AddButton("A button which stands for itself", [](){
    ShowMessageBox("Nothing above it.", _("Button Test"), MB_OK);
  }, {.description = "A group without a caption is how a button is kept "
      "away from the group above it."});
}

static void
ShowButtonDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  button_demo_auto_update = true;
  button_demo_presses = 0;

  auto widget = std::make_unique<GroupedListWidget>(look);

  GroupedListWidget &list = *widget;
  FillButtonPage(list);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Button Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);

  /* an action bar as well, to show that Left and Right leave a row of
     buttons only at its ends */
  list.SetActionBar(dialog.GetButtonPanel());
  dialog.EnableCursorSelection();

  dialog.ShowModal();
}

/**
 * A page of settings as a real one would look: two groups, a caption
 * above each of them and a line of explanation below it, with as
 * many of the elements of an item on the screen at once as a
 * settings page would ever use.
 */
static void
ShowSettingsPageDialog() noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  widget->AddHero("Map Display",
                  "What the map draws, and which way it points.");

  widget->AddGroup("Layers",
                   {.footer = "The layers are drawn in this order, the "
                    "terrain below everything else."});

  widget->AddItem("Terrain", {.icon = IDB_MOUNTAIN_TOP,
                              .toggle = true,
                              .checked = true});

  widget->AddItem("Topography", {.icon = IDB_TOWN,
                                 .toggle = true,
                                 .checked = true});

  widget->AddItem("Airspace", [](){
    ShowMessageBox("The demo does not open the airspace files.",
                   _("Map Display"), MB_OK);
  }, {.icon = IDB_AIRSPACEI,
      .value = "alps.txt",
      .value_font = TextFont::MONO,
      .chevron = true});

  widget->AddItem("Traffic", [](){
    ShowMessageBox("The demo does not open the traffic settings.",
                   _("Map Display"), MB_OK);
  }, {.icon = IDB_TRAFFIC_SAFE,
      .subtitle = "FLARM and ADS-B",
      .badge = "3 in range",
      .chevron = true});

  widget->AddItem("Weather stations",
                  {.icon = IDB_WEATHER_STATION,
                   .help = "The stations of the weather service are drawn "
                   "once a forecast has been downloaded.",
                   .disabled = true,
                   .selectable_when_disabled = true,
                   .disabled_badge_label = "no data"});

  widget->AddGroup("Orientation",
                   {.footer = "Which way the map points while the glider "
                    "is not circling.",
                    .selection_mode = SelectionMode::SINGLE});

  widget->AddItem("North up", {.subtitle = "The map does not turn"});

  widget->AddItem("Track up", {.subtitle = "The map turns with the glider",
                               .checked = true});

  widget->AddItem("Target up", {.subtitle = "The map turns with the leg"});

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Map Display"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

void
ShowGroupedListTestDialog() noexcept
try {
  const auto &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<GroupedListWidget>(look);

  /* the row which the Advanced File Explorer shows above its list */
  widget->SetTopWidget(std::make_unique<LocationWidget>(_("Location"),
                                                        "XCSoarData/logs"));

  /* a view of the project below the list, where a settings page would
     show a preview of what it changes */
  widget->SetBottomWidget(std::make_unique<HorizonPreviewWidget>(), 60);

  widget->AddHero("Grouped List",
                    "One page with several groups: items which carry a "
                    "value, a badge, an arrow or a check mark, and a "
                    "footer which explains a group.");

  /* the other scratch dialog, which simulates the configuration
     menu of XCSoar */
  widget->AddGroup(nullptr);
  widget->AddItem("Configuration Menu", ShowGroupedListMenuDialog,
                  {.chevron = true});

  /* one page of settings, as a real one would look */
  widget->AddItem("Settings Page", ShowSettingsPageDialog,
                  {.chevron = true});

  widget->AddItem("Tap Test", ShowTapTestDialog, {.chevron = true});

  widget->AddItem("Hidden Check Test", ShowHiddenCheckDialog,
                  {.chevron = true});

  widget->AddItem("Widget Save Test", ShowWidgetSaveDialog,
                  {.chevron = true});

  widget->AddItem("Group Options Test", ShowGroupOptionsDialog,
                  {.chevron = true});

  widget->AddItem("Hidden Help Test", ShowHiddenHelpDialog,
                  {.chevron = true});

  widget->AddItem("Cursor Speed Test", ShowCursorSpeedDialog,
                  {.chevron = true});

  widget->AddItem("Text Wrap Test", ShowTextWrapDialog, {.chevron = true});

  widget->AddItem("Value Lines Test", ShowValueLinesDialog,
                  {.chevron = true});

  widget->AddItem("Item Padding Test", ShowItemPaddingDialog,
                  {.chevron = true});

  widget->AddItem("Icon Cache Test", ShowIconCacheDialog, {.chevron = true});

  widget->AddItem("Wide Glyph Test", ShowWideGlyphDialog, {.chevron = true});

  widget->AddItem("Toggle Edge Test", ShowToggleEdgeDialog,
                  {.chevron = true});

  widget->AddItem("Action Bar Test", ShowActionBarDialog, {.chevron = true});

  widget->AddItem("Hit Area Test", ShowHitAreaDialog, {.chevron = true});

  widget->AddItem("Disabled Item Test", ShowDisabledItemDialog,
                  {.chevron = true});

  widget->AddItem("Enter Action Test", ShowEnterActionDialog,
                  {.chevron = true});

  widget->AddItem("Touch Test", ShowTouchDialog, {.chevron = true});

  widget->AddItem("Second Tap Test", ShowSecondTapDialog, {.chevron = true});

  widget->AddItem("Look Ahead Test", ShowLookAheadDialog, {.chevron = true});

  widget->AddItem("Button Test", ShowButtonDialog, {.chevron = true});

  widget->AddItem("Collapsible Test", ShowCollapsibleDialog,
                  {.chevron = true});

  AddGroup(*widget, "Online Services", online_services,
           {.footer = "Log in to each service on its own page. Flights "
            "are uploaded after landing."});

  /* one item of this group may be checked, and the check mark sits at
     the left edge, next to items which have a value, a badge or an
     arrow */
  AddGroup(*widget, "Weather", weather,
           {.footer = "Overlays are downloaded on demand. Only the "
            "checked source is shown on the map. The "
            "[weather chapter](https://xcsoar.readthedocs.io/en/latest/"
            "users/weather.html) explains every source.",
            .selection_mode = SelectionMode::SINGLE,
            .check_position = CheckPosition::LEFT},
           /* no callbacks: the help below the card can be watched
              without a message box in the way */
           false);

  /* a group without a caption */
  AddGroup(*widget, nullptr, quick_access);

  /* a group of switches: every item carries a boolean */
  AddGroup(*widget, "Map Display", switches,
           {.footer = "A switch shows a setting which is either on or "
            "off. Tapping the item flips it."},
           /* no message box: the switch itself is the answer */
           false);

  /* a group which is nothing but check marks */
  widget->AddGroup("Aircraft",
                   {.footer = "Check every glider you fly.",
                    .selection_mode = SelectionMode::MULTIPLE});

  const unsigned aircraft_begin = widget->GetItemCount();

  for (const auto &item : aircraft)
    widget->AddItem(item.caption, {.checked = item.checked});

  const unsigned aircraft_end = widget->GetItemCount();

  /* a one-line footer, followed by a group with a caption */
  AddGroup(*widget, "Devices", devices,
           {.footer = "Configured on the Devices page."});

  /* a second hero card, in the middle of the page: it introduces the
     groups which follow it */
  widget->AddHero("Icons and Check Marks",
                  "The groups below show what an item can carry beside "
                  "its caption.");

  /* items with an icon from the resources of XCSoar */
  AddGroup(*widget, "Map Items", map_items,
           {.footer = "These are the icons which the map draws."});

  /* items whose icon is a character */
  AddGroup(*widget, "Before Take-off", checklist,
           {.footer = "An emoji is drawn in color where the platform has "
            "color fonts, as a plain glyph where it has not, and is left "
            "out where the font does not know it.",
            .selection_mode = SelectionMode::MULTIPLE,
            .check_position = CheckPosition::RIGHT});

  /* items with a second line below the caption */
  AddGroup(*widget, "Wi-Fi", networks,
           {.footer = "A second line carries what does not fit into the "
            "caption."});

  /* several items may be checked here, and the check mark sits at the
     right edge, behind the value and the arrow */
  AddGroup(*widget, "Data Files", data_files,
           /* a link and a markdown list in one footer */
           {.footer = "Files are read from the "
            "[XCSoarData folder](https://xcsoar.readthedocs.io/en/latest/"
            "users/data_files.html):\n"
            "- waypoints and airspaces\n"
            "- the terrain and the topography\n"
            "- the details of the airfields",
            .selection_mode = SelectionMode::MULTIPLE,
            .check_position = CheckPosition::RIGHT});

  /* a group which shows a view instead of items: one with a caption
     and a footer around a button */
  widget->AddWidgetGroup("Task",
                         std::make_unique<ButtonWidget>(look.button,
                                                        "Declare the task",
                                                        []{
                           ShowMessageBox("Declare the task",
                                          _("Grouped List Test"), MB_OK);
                         }),
                         {.footer = "The button belongs to the group above "
                          "it, like the items of the other groups do."});

  /* and one which is nothing but a view */
  widget->AddWidgetGroup(nullptr,
                         std::make_unique<HorizonPreviewWidget>(), 80);

  widget->AddGroup("Info");
  widget->AddItem("About XCSoar", []{
    ShowMessageBox("About XCSoar", _("Grouped List Test"), MB_OK);
  }, {.value = XCSoar_Version, .chevron = true});

  /* a value which is much wider than the item: it wraps inside the
     box which is left of it */
  widget->AddItem("XCSoar data path",
                  {.value = "/private/var/mobile/Containers/Data/Application/"
                   "A195033B-8E4C-4952-A27D-5A8AECB4B5F6/Documents/"
                   "XCSoarData",
                   .value_font = TextFont::MONO});

  /* the same, but below the caption on purpose */
  widget->AddItem("Log files",
                  {.value = "/private/var/mobile/Containers/Data/Application/"
                   "A195033B-8E4C-4952-A27D-5A8AECB4B5F6/Documents/"
                   "XCSoarData/logs",
                   .value_below = true,
                   .value_font = TextFont::MONO});

  /* a caption which is wider than the item, with a short value beside
     it, with a long one which shares the room with it, and with none */
  widget->AddItem("Download the airspace files of the neighbour countries",
                  {.value = "on"});

  widget->AddItem("Download the airspace files of the neighbour countries",
                  {.value = "/private/var/mobile/Containers/Data/Application/"
                   "A195033B-8E4C-4952-A27D-5A8AECB4B5F6/Documents",
                   .value_font = TextFont::MONO});

  widget->AddItem("Download the airspace files of the neighbour countries",
                  {.badge = "new", .chevron = true});

  GroupedListWidget &list = *widget;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Grouped List Test"));
  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Save"), []{
    ShowMessageBox("Saved", _("Grouped List Test"), MB_OK);
  });

  Button *const launch_button = dialog.AddButton("🚀", []{
    ShowMessageBox("Launched", _("Grouped List Test"), MB_OK);
  });

  dialog.AddButton(_("Cancel"), mrCancel);

  /* the rocket launches the glider the cursor is on, and is therefore
     only available inside the "Aircraft" group */
  const auto update_launch_button = [launch_button, aircraft_begin,
                                     aircraft_end](int index){
    launch_button->SetEnabled(index >= (int)aircraft_begin &&
                              index < (int)aircraft_end);
  };

  list.SetCursorCallback(update_launch_button);
  update_launch_button(list.GetCursorIndex());

  /* Right and Left step between the list and the buttons, so that the
     rocket can be reached from the middle of the list */
  list.SetActionBar(dialog.GetButtonPanel());
  dialog.EnableCursorSelection();

  dialog.ShowModal();
} catch (...) {
  LogError(std::current_exception(), "Grouped List Test");
}
