/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "MapOverlayWidget.hpp"
#include "Dialogs/Error.hpp"
#include "UIGlobals.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/ActionListener.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/TextListWidget.hpp"
#include "Widget/ViewImageWidget.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "OS/Path.hpp"
#include "OS/FileUtil.hpp"
#include "Util/StaticString.hxx"
#include "Util/StringAPI.hxx"
#include "Util/StringCompare.hxx"

#include <vector>

class WeatherMapOverlayListWidget final
  : public TextListWidget, ActionListener {

  enum Buttons {
    USE,
    DISABLE,
  };

  struct Item {
    StaticString<80> name;
    AllocatedPath path;

    Item(const TCHAR *_name, Path _path)
      :name(_name), path(_path) {}

    bool operator<(const Item &other) const {
      return StringCollate(name, other.name) < 0;
    }
  };

  ViewImageWidget *preview_widget;
  Bitmap preview_bitmap;

  ButtonPanelWidget *buttons_widget;

  Button *use_button, *disable_button;

  std::vector<Item> items;

public:
  void SetPreview(ViewImageWidget &_preview_widget) {
    preview_widget = &_preview_widget;
  }

  void SetButtonPanel(ButtonPanelWidget &_buttons) {
    buttons_widget = &_buttons;
  }

  void CreateButtons(ButtonPanel &buttons);

private:
  void UpdateList();

  void UpdatePreview(Path path) {
    preview_widget->SetBitmap(nullptr);

    preview_bitmap.Reset();
    try {
      if (!preview_bitmap.LoadFile(path))
        return;
    } catch (const std::exception &e) {
      return;
    }

    preview_widget->SetBitmap(preview_bitmap);
  }

  void UpdatePreview() {
    if (items.empty()) {
      preview_widget->SetBitmap(nullptr);
      return;
    }

    UpdatePreview(items[GetList().GetCursorIndex()].path);
  }

protected:
  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override {
    CreateButtons(buttons_widget->GetButtonPanel());
    TextListWidget::Prepare(parent, rc);
    UpdateList();
  }

  void Show(const PixelRect &rc) override {
    TextListWidget::Show(rc);
    UpdatePreview();
  }

  /* virtual methods from TextListWidget */
  const TCHAR *GetRowText(unsigned i) const override {
    return items[i].name.c_str();
  }

  /* virtual methods from ListCursorHandler */
  virtual void OnCursorMoved(unsigned i) override {
    UpdatePreview(items[i].path);
  }

  virtual bool CanActivateItem(unsigned i) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned i) override {
    UseClicked(i);
  }

private:
  void UseClicked(unsigned i);

  void DisableClicked() {
    auto *map = UIGlobals::GetMap();
    if (map != nullptr)
      map->SetOverlayBitmap(nullptr);
  }

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;
};

void
WeatherMapOverlayListWidget::CreateButtons(ButtonPanel &buttons)
{
  use_button = buttons.Add(_("Use"), *this, USE);
  disable_button = buttons.Add(_("Disable"), *this, DISABLE);
}

void
WeatherMapOverlayListWidget::UpdateList()
{
  items.clear();

  struct Visitor : public File::Visitor {
    std::vector<Item> &items;

    explicit Visitor(std::vector<Item> &_items):items(_items) {}

    void Visit(Path path, Path filename) override {
      items.emplace_back(filename.c_str(), path);
    }
  } visitor(items);

  const auto weather_path = LocalPath(_T("weather"));
  const auto overlay_path = AllocatedPath::Build(weather_path, _T("overlay"));
  Directory::VisitSpecificFiles(overlay_path, _T("*.tif"), visitor);
  Directory::VisitSpecificFiles(overlay_path, _T("*.tiff"), visitor);

  const unsigned n = items.size();

  if (n > 0)
    std::sort(items.begin(), items.end());

  ListControl &list_control = GetList();
  list_control.SetLength(n);
  list_control.Invalidate();

  const bool empty = items.empty();
  use_button->SetEnabled(!empty);
}

/**
 * Set up reasonable defaults for the given overlay.
 */
static void
SetupOverlay(MapOverlayBitmap &bmp, Path::const_pointer name)
{
  /* File name convention according to DWD paper:
   *
   * nb_mod_met_chart_lv_level_p_step_run
   *
   * nb = product (NinJo batch)
   * mod = model ("icon", "coseu", "cosde")
   * met = contents ("hwx", "ome", "nptw", "eis", ...)
   * chart = area
   * lb = level code
   * level = level (m or hPa)
   * p = code for predicted time
   * step = predicted time
   * run = model run [HHmm]
   */

  /* configure a default, just in case this overlay type is unknown */
  bmp.SetAlpha(0.5);

  if (StringStartsWithIgnoreCase(name, _T("nb_"))) {
    name += 3;

    /* skip "model", go to "met" */
    auto underscore = StringFind(name, '_');
    if (underscore != nullptr) {
      name = underscore + 1;

      if (StringStartsWithIgnoreCase(name, _T("ome_"))) {
        /* vertical wind */
        bmp.SetAlpha(0.5);
      } else if (StringStartsWithIgnoreCase(name, _T("w_"))) {
        /* horizontal wind */
        bmp.SetAlpha(0.7);
      }
    }
  } else if (StringStartsWithIgnoreCase(name, _T("sat_"))) {
    bmp.IgnoreBitmapAlpha();
    bmp.SetAlpha(0.9);
  } else if (StringStartsWithIgnoreCase(name, _T("pg_"))) {
    /* precipitation */
    bmp.SetAlpha(0.4);
  } else if (StringStartsWithIgnoreCase(name, _T("Vertikalwind"))) {
    /* name of a draft file I got from DWD */
    // TODO: remove obsolete prefix
    bmp.IgnoreBitmapAlpha();
    bmp.SetAlpha(0.4);
  }
}

void
WeatherMapOverlayListWidget::UseClicked(unsigned i)
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  const Path path = items[i].path;

  std::unique_ptr<MapOverlayBitmap> bmp;
  try {
    bmp.reset(new MapOverlayBitmap(items[i].path));
  } catch (const std::exception &e) {
    ShowError(e, _("Weather"));
    return;
  }

  SetupOverlay(*bmp, path.GetBase().c_str());

  map->SetOverlayBitmap(std::move(bmp));
}

void
WeatherMapOverlayListWidget::OnAction(int id)
{
  switch ((Buttons)id) {
  case USE:
    UseClicked(GetList().GetCursorIndex());
    break;

  case DISABLE:
    DisableClicked();
    break;
  }
}

Widget *
CreateWeatherMapOverlayWidget()
{
  auto *list = new WeatherMapOverlayListWidget();
  auto *view = new ViewImageWidget();
  auto *two = new TwoWidgets(list, view, false);
  auto *buttons = new ButtonPanelWidget(two,
                                        ButtonPanelWidget::Alignment::BOTTOM);
  list->SetPreview(*view);
  list->SetButtonPanel(*buttons);
  return buttons;
}
