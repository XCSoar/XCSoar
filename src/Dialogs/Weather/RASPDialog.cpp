// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/Rasp/RaspStyle.hpp"
#include "Weather/Rasp/ColorMap.hpp"
#include "Terrain/RasterRenderer.hpp"
#include "ui/canvas/RawBitmap.hpp"
#include "Math/Angle.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/ConstantAlpha.hpp"
#endif
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Repository/FileType.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "Interface.hpp"
#include "UIState.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "net/http/Features.hpp"
#ifdef HAVE_DOWNLOAD_MANAGER
#include "Dialogs/FileManager.hpp"
#include "net/http/DownloadManager.hpp"
#endif

#include <fmt/format.h>
#include <algorithm>

class RaspColorbarWindow : public PaintWindow {
  const DialogLook &look;
  const RaspStyle *style = nullptr;
  ContourDensity contour_density = ContourDensity::OFF;

public:
  explicit RaspColorbarWindow(const DialogLook &_look) noexcept
    :look(_look) {}

  void SetStyle(const RaspStyle *_style,
                ContourDensity _contour_density) noexcept {
    style = _style;
    contour_density = _contour_density;
    Invalidate();
  }

  void OnPaint(Canvas &canvas) noexcept override;
};

void
RaspColorbarWindow::OnPaint(Canvas &canvas) noexcept
{
  const auto rc = canvas.GetRect();

  if (style == nullptr) {
    canvas.Clear(look.background_color);
    return;
  }

  const bool use_alpha = style->HasAlpha();
  const auto &map = use_alpha
    ? style->color_map_alpha : style->color_map;
  const float min_v = map.points[0].value;
  const float max_v = map.points[map.num_points - 1].value;
  const unsigned height_scale = style->height_scale;

  // Compute rendering-domain bounds from the physical
  // color map range
  const int16_t min_h = (int16_t)std::clamp(
    (int)(min_v * style->scale + style->offset),
    0, (int)INT16_MAX);
  const int16_t max_h = (int16_t)std::clamp(
    (int)(max_v * style->scale + style->offset),
    0, (int)INT16_MAX);

  // Build the color table using the same code path
  // as the map renderer
  auto materialized =
    MaterializeColorRamp(style->color_map,
                         style->color_map_alpha,
                         style->scale, style->offset,
                         height_scale, style->do_water);
  auto ramp = materialized.GetColorRamp();

  RasterRenderer renderer;
  constexpr int interp_levels = 5;
  if (use_alpha)
    renderer.PrepareColorTableAlpha(
      &ramp, style->do_water,
      height_scale, interp_levels);
  else
    renderer.PrepareColorTable(
      &ramp, style->do_water,
      height_scale, interp_levels);

  canvas.Select(look.text_font);
  const unsigned font_h = canvas.CalcTextSize("0").height;
  const int bar_bottom = rc.bottom - font_h - 2;
  const unsigned width = rc.right - rc.left;
  const unsigned bar_height = std::max(0, bar_bottom - rc.top);

  if (width == 0 || bar_height == 0)
    return;

  // Fill a synthetic height matrix with a horizontal
  // gradient and render through the full pipeline
  renderer.FillGradient({width, bar_height},
                        min_h, max_h);
  const unsigned contour_spacing =
    ContourSpacing(contour_density, height_scale);
  renderer.GenerateImage(false, height_scale,
                         0, 0, Angle::Zero(), contour_spacing);

  if (use_alpha) {
    // Draw checkerboard background for alpha styles
    constexpr unsigned CHECK_SIZE = 8;
    const Color light_color(230, 230, 230);
    const Color dark_color(26, 26, 26);

    for (unsigned y = 0; y < bar_height;
         y += CHECK_SIZE) {
      for (unsigned x = 0; x < width;
           x += CHECK_SIZE) {
        const bool light =
          ((x / CHECK_SIZE)
           + (y / CHECK_SIZE)) % 2 == 0;
        canvas.DrawFilledRectangle(
          PixelRect{(int)x, (int)y,
            std::min((int)(x + CHECK_SIZE),
                     (int)width),
            std::min((int)(y + CHECK_SIZE),
                     (int)bar_height)},
          light ? light_color : dark_color);
      }
    }
  }

#ifdef ENABLE_OPENGL
  const ScopeTextureConstantAlpha blend(use_alpha, 1.0f);
#endif
  renderer.GetImage().StretchTo(
    PixelSize{width, bar_height}, canvas,
    PixelSize{width, bar_height}, false, use_alpha);

  // Draw min/max text labels
  auto fmt_value = [](float v) -> std::string {
    if (v >= 100.0f || v <= -100.0f)
      return fmt::format("{:.0f}", v);
    return fmt::format("{:.1f}", v);
  };

  canvas.SetTextColor(look.text_color);
  canvas.SetBackgroundTransparent();

  const auto min_text = fmt_value(min_v);
  const auto max_text = fmt_value(max_v);

  canvas.DrawText({rc.left + 1, bar_bottom + 1},
                  min_text);
  const auto max_size = canvas.CalcTextSize(max_text);
  canvas.DrawText({rc.right - (int)max_size.width - 1,
                   bar_bottom + 1},
                  max_text);
}

class RASPSettingsPanel final
  : public RowFormWidget {

  enum Controls {
    FILE,
    MODIFIED,
#ifdef HAVE_DOWNLOAD_MANAGER
    UPDATE,
#endif
    SPACER,
    PREVIEW_FIELD,
    CONTOURS,
    COLORBAR,
  };

  std::shared_ptr<RaspStore> rasp;

#ifdef HAVE_DOWNLOAD_MANAGER
  Button *update_button = nullptr;
#endif

  ContourDensity contour_density = ContourDensity::OFF;

  void ReloadRasp();
  void UpdateModifiedDisplay();
  void UpdateClicked();

public:
  explicit RASPSettingsPanel(std::shared_ptr<RaspStore> &&_rasp) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     rasp(std::move(_rasp)) {}

private:
  void FillPreviewField() noexcept;
  void UpdateColorbar() noexcept;

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RASPSettingsPanel::ReloadRasp()
{
  rasp = LoadConfiguredRasp(false);
  DataGlobals::SetRasp(rasp);
  RaspFileChanged = true;
  Profile::Save();
  UpdateModifiedDisplay();
  FillPreviewField();
  UpdateColorbar();
}

void
RASPSettingsPanel::UpdateModifiedDisplay()
{
  StaticString<32> buffer;

  if (rasp != nullptr) {
    const BrokenDateTime modified = rasp->GetFileModifiedTime();
    if (modified.IsPlausible()) {
      buffer.Format("%04u-%02u-%02u %02u:%02u",
                      modified.year, modified.month, modified.day,
                      modified.hour, modified.minute);
    }
  }

  if (buffer.empty())
    buffer = _("Unknown");

  SetText(MODIFIED, buffer.c_str());
}

void
RASPSettingsPanel::UpdateClicked()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  ShowFileManager();

  const AllocatedPath profile_path = Profile::GetPath(ProfileKeys::RaspFile);
  if (profile_path != nullptr)
    LoadValue(FILE, Path(profile_path));

  ReloadRasp();
#endif
}

void
RASPSettingsPanel::FillPreviewField() noexcept
{
  auto &control = GetControl(PREVIEW_FIELD);
  auto &df = (DataFieldEnum &)*control.GetDataField();

  /* preserve the current selection across refills; on the first call
     the field is still empty, so default to "None" (-1) */
  const int previous = df.Count() > 0 ? (int)df.GetValue() : -1;
  Rasp::FillFieldChoices(df, rasp.get(), {.include_none = true});
  df.SetValue(previous);
  control.RefreshDisplay();
}

void
RASPSettingsPanel::UpdateColorbar() noexcept
{
  /* The colorbar previews the field selected in the "Preview field"
     control.  The "None" choice has the id -1. */
  const int item_index = (int)GetValueEnum(PREVIEW_FIELD);

  const RaspStyle *s = nullptr;
  if (item_index >= 0 && rasp &&
      unsigned(item_index) < rasp->GetItemCount()) {
    const auto &mi = rasp->GetItemInfo(item_index);
    s = &LookupWeatherTerrainStyle(mi.name);
  }

  ((RaspColorbarWindow &)GetRow(COLORBAR)).SetStyle(s, contour_density);
}

void
RASPSettingsPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  WndProperty *wp = AddFile(_("File"), nullptr,
                            ProfileKeys::RaspFile,
                            GetFileTypePatterns(FileType::RASP),
                            FileType::RASP);
  wp->GetDataField()->SetOnModified([this]{
    if (SaveValueFileReader(FILE, ProfileKeys::RaspFile)) {
      ReloadRasp();
      GetControl(FILE).RefreshDisplay();
    }
  });

  AddReadOnly(_("Modified"),
              _("Local date and time of the selected RASP file."));
  UpdateModifiedDisplay();

#ifdef HAVE_DOWNLOAD_MANAGER
  update_button = AddButton(_("Update"), [this]{ UpdateClicked(); });
  if (!Net::DownloadManager::IsAvailable())
    update_button->SetEnabled(false);
#endif

  AddSpacer();

  WndProperty *preview = AddEnum(_("Preview field"),
                                 _("The RASP field whose colour scale "
                                   "is shown below."));
  preview->GetDataField()->SetOnModified([this]{ UpdateColorbar(); });
  FillPreviewField();

  {
    static constexpr StaticEnumChoice contour_density_list[] = {
      { ContourDensity::OFF,       N_("Off") },
      { ContourDensity::WIDE,      N_("Wide") },
      { ContourDensity::REGULAR,   N_("Regular") },
      { ContourDensity::FINE,      N_("Fine") },
      { ContourDensity::SUPERFINE, N_("Superfine") },
      nullptr,
    };
    Profile::GetEnum(ProfileKeys::RaspContours, contour_density);
    WndProperty *cp = AddEnum(_("Contours"), nullptr,
                              contour_density_list,
                              (unsigned)contour_density);
    cp->GetDataField()->SetOnModified([this]{
      contour_density = (ContourDensity)GetValueEnum(CONTOURS);
      UpdateColorbar();
    });
  }

  {
    const auto &dialog_look = UIGlobals::GetDialogLook();
    auto colorbar =
      std::make_unique<RaspColorbarWindow>(dialog_look);

    WindowStyle style;
    style.Border();
    colorbar->Create((ContainerWindow &)GetWindow(),
                     {0, 0, 100, Layout::Scale(40)},
                     style);
    Add(std::move(colorbar));
  }

  UpdateColorbar();
}

bool
RASPSettingsPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
  WeatherUIState &state = CommonInterface::SetUIState().weather;

  state.contour_density = contour_density;
  Profile::SetEnum(ProfileKeys::RaspContours, contour_density);

  ActionInterface::SendUIState(true);

  return true;
}

std::unique_ptr<Widget>
CreateRaspWidget() noexcept
{
  auto rasp = DataGlobals::GetRasp();
  return std::make_unique<RASPSettingsPanel>(std::move(rasp));
}
