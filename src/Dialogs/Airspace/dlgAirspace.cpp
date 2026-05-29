// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/WindowWidget.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Draw.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "MainWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Look/DialogLook.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <cassert>

/**
 * Geometry of the right-hand checkbox columns shared by the list
 * renderer and the column-header widget.
 */
struct AirspaceFilterColumns {
  /** Total number of checkbox columns. */
  static constexpr unsigned COUNT = 3;

  /** Header text for each column. */
  const char *labels[COUNT];

  /** Width of one column cell, including padding. */
  unsigned col_width = 0;

  void SetColWidth(unsigned w) noexcept { col_width = w; }

  [[gnu::pure]]
  PixelRect ColRect(const PixelRect &rc, unsigned col) const noexcept {
    assert(col < COUNT);
    PixelRect r = rc;
    r.left = rc.right - (int)((COUNT - col) * col_width);
    r.right = r.left + (int)col_width;
    return r;
  }

  /**
   * Returns the leftmost X coordinate that belongs to the rightmost
   * checkbox column (i.e. the left edge of column 0).
   */
  [[gnu::pure]]
  int ColumnsLeft(const PixelRect &rc) const noexcept {
    return rc.right - (int)(COUNT * col_width);
  }

  /**
   * Returns the index of the column hit by the given X coordinate
   * within @p rc, or -1 if no column was hit.
   */
  [[gnu::pure]]
  int HitTest(const PixelRect &rc, int x) const noexcept {
    if (col_width == 0)
      return -1;
    const int rel = rc.right - x;
    if (rel < 0)
      return -1;
    const int col_from_right = rel / (int)col_width;
    if (col_from_right < 0 || col_from_right >= (int)COUNT)
      return -1;
    return (int)COUNT - 1 - col_from_right;
  }
};

class AirspaceSettingsListWidget;

/**
 * Header widget displaying column titles above the list rows.
 */
class AirspaceFilterHeaderWidget final : public WindowWidget {
  const AirspaceFilterColumns &columns;
  const AirspaceSettingsListWidget &list_widget;
  unsigned row_height;

public:
  AirspaceFilterHeaderWidget(const AirspaceFilterColumns &_columns,
                             const AirspaceSettingsListWidget &_list_widget,
                             unsigned _row_height) noexcept
    :columns(_columns), list_widget(_list_widget),
     row_height(_row_height) {}

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override {
    return {0u, row_height};
  }

  PixelSize GetMaximumSize() const noexcept override {
    return {4096u, row_height};
  }

  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique<WndOwnerDrawFrame>();
    w->Create(parent, rc, style,
              [this](Canvas &canvas, const PixelRect &paint_rc){
                Paint(canvas, paint_rc);
              });

    SetWindow(std::move(w));
  }

  bool SetFocus() noexcept override { return false; }

private:
  void Paint(Canvas &canvas, const PixelRect &rc) const noexcept;
};

/**
 * Vertical container: fixed-height header at the top, body widget
 * filling the rest of the available area.
 */
class HeaderAboveListWidget final : public NullWidget {
  const std::unique_ptr<Widget> header, body;
  const unsigned header_height;
  PixelRect rc;

  [[gnu::pure]]
  std::pair<PixelRect, PixelRect>
  Split(const PixelRect &r) const noexcept {
    PixelRect h = r, b = r;
    h.bottom = std::min(r.bottom,
                        r.top + (int)header_height);
    b.top = h.bottom;
    return {h, b};
  }

public:
  HeaderAboveListWidget(std::unique_ptr<Widget> &&_header,
                        std::unique_ptr<Widget> &&_body,
                        unsigned _header_height) noexcept
    :header(std::move(_header)), body(std::move(_body)),
     header_height(_header_height) {}

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override {
    const PixelSize bmin = body->GetMinimumSize();
    return {bmin.width, header_height + bmin.height};
  }

  PixelSize GetMaximumSize() const noexcept override {
    const PixelSize bmax = body->GetMaximumSize();
    return {bmax.width, header_height + bmax.height};
  }

  void Initialise(ContainerWindow &parent,
                  const PixelRect &r) noexcept override {
    rc = r;
    const auto layout = Split(r);
    header->Initialise(parent, layout.first);
    body->Initialise(parent, layout.second);
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &r) noexcept override {
    rc = r;
    const auto layout = Split(r);
    header->Prepare(parent, layout.first);
    body->Prepare(parent, layout.second);
  }

  void Unprepare() noexcept override {
    header->Unprepare();
    body->Unprepare();
  }

  void Show(const PixelRect &r) noexcept override {
    rc = r;
    const auto layout = Split(r);
    header->Show(layout.first);
    body->Show(layout.second);
  }

  void Hide() noexcept override {
    header->Hide();
    body->Hide();
  }

  void Move(const PixelRect &r) noexcept override {
    rc = r;
    const auto layout = Split(r);
    header->Move(layout.first);
    body->Move(layout.second);
  }

  bool SetFocus() noexcept override {
    return body->SetFocus() || header->SetFocus();
  }

  bool HasFocus() const noexcept override {
    return header->HasFocus() || body->HasFocus();
  }

  bool KeyPress(unsigned key_code) noexcept override {
    return body->KeyPress(key_code) || header->KeyPress(key_code);
  }
};

class AirspaceSettingsListWidget : public ListWidget {
  const bool color_mode;
  bool changed = false;

  TextRowRenderer row_renderer;

  AirspaceFilterColumns columns;

public:
  explicit AirspaceSettingsListWidget(bool _color_mode) noexcept
    :color_mode(_color_mode) {
    columns.labels[0] = _("Show");
    columns.labels[1] = _("Warn");
    columns.labels[2] = _("Clrnc");
  }

  bool IsModified() const noexcept {
    return changed;
  }

  const AirspaceFilterColumns &GetColumns() const noexcept {
    return columns;
  }

  using ListWidget::GetList;

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();
    const unsigned row_height =
      row_renderer.CalculateLayout(*look.list.font);

    /* size column cells to fit the header label */
    unsigned col_width = row_height;
    const unsigned padding = 2 * Layout::GetTextPadding();
    for (unsigned i = 0; i < AirspaceFilterColumns::COUNT; ++i) {
      const unsigned w =
        look.list.font->TextSize(columns.labels[i]).width + padding;
      if (w > col_width)
        col_width = w;
    }
    columns.SetColWidth(col_width);

    ListControl &list = CreateList(parent, look, rc, row_height);
    list.SetLength(AIRSPACECLASSCOUNT);

    if (!color_mode)
      list.SetActivateOnFirstClick(true);
  }

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
  void OnActivateItem(unsigned index, int x) noexcept override;

private:
  void ToggleColumn(unsigned class_index, unsigned col) noexcept;
};

void
AirspaceFilterHeaderWidget::Paint(Canvas &canvas,
                                  const PixelRect &rc) const noexcept
{
  const auto &look = UIGlobals::GetDialogLook();
  canvas.Select(*look.list.font);
  canvas.SetTextColor(look.text_color);
  canvas.SetBackgroundTransparent();

  /* align column right edge with the list's item-area right edge */
  PixelRect aligned_rc = rc;
  aligned_rc.right = (int)list_widget.GetList().GetItemAreaRight();

  for (unsigned i = 0; i < AirspaceFilterColumns::COUNT; ++i) {
    const PixelRect col_rc = columns.ColRect(aligned_rc, i);
    const char *text = columns.labels[i];
    const PixelSize size = canvas.CalcTextSize(text);
    const int x =
      col_rc.left + ((int)col_rc.GetWidth() - (int)size.width) / 2;
    const int y =
      col_rc.top + ((int)col_rc.GetHeight() - (int)size.height) / 2;
    canvas.DrawText({x, y}, text);
  }
}

void
AirspaceSettingsListWidget::OnPaintItem(Canvas &canvas, PixelRect rc,
                                        unsigned i) noexcept
{
  assert(i < AIRSPACECLASSCOUNT);

  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::GetMapSettings().airspace;
  const AirspaceLook &look =
    CommonInterface::main_window->GetLook().map.airspace;
  const auto &dialog_look = UIGlobals::GetDialogLook();

  const char *const name = AirspaceFormatter::GetClass((AirspaceClass)i);

  if (color_mode) {
    int second_x = row_renderer.NextColumn(canvas, rc, name);

    const int padding = Layout::GetTextPadding();

    if (AirspacePreviewRenderer::PrepareFill(
        canvas, (AirspaceClass)i, look, renderer)) {
      canvas.DrawRectangle({second_x, rc.top + padding,
                            rc.right - padding, rc.bottom - padding});
      AirspacePreviewRenderer::UnprepareFill(canvas);
    }
    if (AirspacePreviewRenderer::PrepareOutline(
        canvas, (AirspaceClass)i, look, renderer)) {
      canvas.DrawRectangle({second_x, rc.top + padding,
                            rc.right - padding, rc.bottom - padding});
    }

    row_renderer.DrawTextRow(canvas, rc, name);
    return;
  }

  const bool checks[AirspaceFilterColumns::COUNT] = {
    renderer.classes[i].display,
    computer.warnings.class_warnings[i],
    computer.warnings.class_clearance_allowed[i],
  };

  const int padding = Layout::GetTextPadding();
  const int box_size = std::max(0, (int)rc.GetHeight() - 2 * padding);
  for (unsigned c = 0; c < AirspaceFilterColumns::COUNT; ++c) {
    const PixelRect col_rc = columns.ColRect(rc, c);
    const int cx = (col_rc.left + col_rc.right) / 2;
    const int cy = (col_rc.top + col_rc.bottom) / 2;
    const PixelRect box_rc{cx - box_size / 2, cy - box_size / 2,
                           cx - box_size / 2 + box_size,
                           cy - box_size / 2 + box_size};
    if (box_size > 0)
      DrawCheckBox(canvas, dialog_look, box_rc,
                   checks[c], false, false, true);
  }

  PixelRect name_rc = rc;
  name_rc.right = columns.ColumnsLeft(rc);
  row_renderer.DrawTextRow(canvas, name_rc, name);
}

void
AirspaceSettingsListWidget::ToggleColumn(unsigned class_index,
                                         unsigned col) noexcept
{
  assert(class_index < AIRSPACECLASSCOUNT);
  assert(col < AirspaceFilterColumns::COUNT);

  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  AirspaceRendererSettings &renderer =
    CommonInterface::SetMapSettings().airspace;

  switch (col) {
  case 0:
    renderer.classes[class_index].display =
      !renderer.classes[class_index].display;
    Profile::SetAirspaceMode(Profile::map, class_index,
                             renderer.classes[class_index].display,
                             computer.warnings.class_warnings[class_index]);
    break;
  case 1:
    computer.warnings.class_warnings[class_index] =
      !computer.warnings.class_warnings[class_index];
    Profile::SetAirspaceMode(Profile::map, class_index,
                             renderer.classes[class_index].display,
                             computer.warnings.class_warnings[class_index]);
    break;
  case 2:
    computer.warnings.class_clearance_allowed[class_index] =
      !computer.warnings.class_clearance_allowed[class_index];
    Profile::SetAirspaceClearance(Profile::map, class_index,
                                  computer.warnings.class_clearance_allowed[class_index]);
    break;
  }

  changed = true;
  ActionInterface::SendMapSettings();
  GetList().Invalidate();
}

void
AirspaceSettingsListWidget::OnActivateItem(unsigned index) noexcept
{
  assert(index < AIRSPACECLASSCOUNT);

  if (color_mode) {
    AirspaceRendererSettings &renderer =
      CommonInterface::SetMapSettings().airspace;
    AirspaceLook &look =
      CommonInterface::main_window->SetLook().map.airspace;

    if (!ShowAirspaceClassRendererSettingsDialog((AirspaceClass)index))
      return;

    ActionInterface::SendMapSettings();
    look.Reinitialise(renderer);
    GetList().Invalidate();
    return;
  }

  /* keyboard activation (no x coordinate) defaults to toggling the
     "Show" column */
  ToggleColumn(index, 0);
}

void
AirspaceSettingsListWidget::OnActivateItem(unsigned index, int x) noexcept
{
  assert(index < AIRSPACECLASSCOUNT);

  if (color_mode) {
    OnActivateItem(index);
    return;
  }

  /* match the geometry used by OnPaintItem(), use GetItemAreaRight() 
     rather than the full client rect for clicks to hit the right column */
  PixelRect list_rc = GetList().GetClientRect();
  list_rc.right = (int)GetList().GetItemAreaRight();
  const int hit = columns.HitTest(list_rc, x);
  if (hit < 0) {
    /* tap was on the name area: do nothing for now */
    return;
  }

  ToggleColumn(index, (unsigned)hit);
}

void
dlgAirspaceShowModal(bool color_mode)
{
  if (color_mode) {
    TWidgetDialog<AirspaceSettingsListWidget>
      dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
             UIGlobals::GetDialogLook(),
             _("Airspace"));
    dialog.AddButton(_("Close"), mrOK);
    dialog.SetWidget(color_mode);
    dialog.ShowModal();

    if (dialog.GetWidget().IsModified())
      Profile::Save();
    return;
  }

  auto list = std::make_unique<AirspaceSettingsListWidget>(color_mode);
  AirspaceSettingsListWidget *list_ptr = list.get();

  const auto &dialog_look = UIGlobals::GetDialogLook();
  const unsigned row_height = dialog_look.list.font->GetHeight()
    + 2 * Layout::GetTextPadding();

  auto header =
    std::make_unique<AirspaceFilterHeaderWidget>(list_ptr->GetColumns(),
                                                 *list_ptr,
                                                 row_height);

  auto pair =
    std::make_unique<HeaderAboveListWidget>(std::move(header),
                                            std::move(list),
                                            row_height);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _("Airspace"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.FinishPreliminary(std::move(pair));
  dialog.ShowModal();

  if (list_ptr->IsModified())
    Profile::Save();
}
