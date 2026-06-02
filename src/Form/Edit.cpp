// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Edit.hpp"
#include "Look/DialogLook.hpp"
#include "DataField/Base.hpp"
#include "DataField/Boolean.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "Dialogs/DataField.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Asset.hpp"
#include "system/Path.hpp"
#include "system/RunFile.hpp"

#include <cassert>
#include <algorithm>

bool
WndProperty::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_RETURN:
    return true;

  case KEY_LEFT:
  case KEY_RIGHT:
    return !IsReadOnly();

  default:
    return WindowControl::OnKeyCheck(key_code);
  }
}

bool
WndProperty::OnKeyDown(unsigned key_code) noexcept
{
  // If return key pressed (Compaq uses VKF23)
  if (key_code == KEY_RETURN) {
    BeginEditing();
    return true;
  }

  switch (key_code) {
  case KEY_RIGHT:
    if (IsReadOnly())
      break;

    IncValue();
    return true;
  case KEY_LEFT:
    if (IsReadOnly())
      break;

    DecValue();
    return true;
  }

  return WindowControl::OnKeyDown(key_code);
}

void
WndProperty::OnSetFocus() noexcept
{
  WindowControl::OnSetFocus();

  Invalidate();

  ScrollParentTo();
}

void
WndProperty::OnKillFocus() noexcept
{
  WindowControl::OnKillFocus();

  Invalidate();
}

WndProperty::WndProperty(ContainerWindow &parent, const DialogLook &_look,
                         const char *Caption,
                         const PixelRect &rc,
                         int CaptionWidth,
                         const WindowStyle style) noexcept
  :look(_look),
   edit_callback(EditDataFieldDialog)
{
  Create(parent, rc, Caption, CaptionWidth, style);

#if defined(USE_WINUSER) && !defined(NDEBUG)
  ::SetWindowText(hWnd, Caption);
#endif
}

WndProperty::WndProperty(const DialogLook &_look) noexcept
  :look(_look),
   edit_callback(EditDataFieldDialog)
{
}

void
WndProperty::Create(ContainerWindow &parent, const PixelRect &rc,
                    const char *_caption,
                    unsigned _caption_width,
                    const WindowStyle style=WindowStyle()) noexcept
{
  caption = _caption;
  caption_width = _caption_width;

  WindowControl::Create(parent, rc, style);
}

WndProperty::~WndProperty() noexcept
{
  delete data_field;
}

unsigned
WndProperty::GetRecommendedCaptionWidth() const noexcept
{
  return look.text_font.TextSize(caption).width + Layout::GetTextPadding() * 2;
}

void
WndProperty::SetCaptionWidth(int _caption_width) noexcept
{
  if (caption_width == _caption_width)
    return;

  caption_width = _caption_width;
  UpdateLayout();
}

bool
WndProperty::BeginEditing() noexcept
{
  if (IsReadOnly() || data_field == nullptr || edit_callback == nullptr) {
    /* If readonly and has content, show full content dialog */
    if (IsReadOnly() && !value.empty()) {
      ShowFullContent();
      return false;
    }
    
    OnHelp();
    return false;
  }

  if (data_field->GetType() == DataField::Type::BOOLEAN) {
    auto &df = static_cast<DataFieldBoolean &>(*data_field);
    df.ModifyValue(!df.GetValue());
    RefreshDisplay();
    return true;
  }

  if (!edit_callback(GetCaption(), *data_field, GetHelpText()))
    return false;

  RefreshDisplay();
  return true;
}

void
WndProperty::ShowFullContent() noexcept
{
  if (value.empty())
    return;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), GetCaption());
  
  auto *widget = new LargeTextWidget(UIGlobals::GetDialogLook(), value.c_str());
  
  dialog.FinishPreliminary(widget);
  
#if defined(HAVE_RUN_FILE) && !defined(ANDROID)
  /* Only show "Open" button if the content is an absolute path
   * Android handles external files via ContentProvider instead */
  if (Path(value.c_str()).IsAbsolute()) {
    dialog.AddButton(_("Open"), [this](){
      RunFile(value.c_str());
    });
  }
#endif
  
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

void
WndProperty::UpdateLayout() noexcept
{
  edit_rc = GetClientRect();

  const unsigned margin = Layout::VptScale(1u);

  if (caption_width >= 0) {
    edit_rc.left += caption_width + margin;
    edit_rc.top += margin;
    edit_rc.right -= margin;
    edit_rc.bottom -= margin;
  } else {
    const unsigned caption_height = look.text_font.GetHeight();

    edit_rc.left += margin;
    edit_rc.top = margin + caption_height;
    edit_rc.right -= margin;
    edit_rc.bottom -= margin;
  }

  Invalidate();
}

void
WndProperty::OnResize(PixelSize new_size) noexcept
{
  WindowControl::OnResize(new_size);
  UpdateLayout();
}

static PixelRect
GetHelpIconRect(const PixelRect &edit_rc) noexcept
{
  const int h = edit_rc.GetHeight() * 7 / 10;
  const int left = edit_rc.left + Layout::GetTextPadding();
  const int top = edit_rc.top + (edit_rc.GetHeight() - h) / 2;
  return {left, top, left + h, top + h};
}

bool
WndProperty::OnMouseDown([[maybe_unused]] PixelPoint p) noexcept
{
  const bool is_boolean = data_field != nullptr &&
    data_field->GetType() == DataField::Type::BOOLEAN;

  if (is_boolean && HasHelp() && GetHelpIconRect(edit_rc).Contains(p)) {
    help_icon_pressed = true;
    pressed = true;
    Invalidate();
    SetCapture();
    return true;
  }

  if (!IsReadOnly() || HasHelp()) {
    dragging = true;
    pressed = true;
    Invalidate();
    SetCapture();
    return true;
  }

  return false;
}

bool
WndProperty::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  if (help_icon_pressed) {
    help_icon_pressed = false;
    ReleaseCapture();
    if (pressed) {
      pressed = false;
      Invalidate();
      if (GetHelpIconRect(edit_rc).Contains(p))
        OnHelp();
    }
    return true;
  }

  if (dragging) {
    dragging = false;
    ReleaseCapture();

    if (pressed) {
      pressed = false;
      Invalidate();
      BeginEditing();
    }

    return true;
  }

  return false;
}

bool
WndProperty::OnMouseMove(PixelPoint p, [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging || help_icon_pressed) {
    const bool inside = IsInside(p);
    if (inside != pressed) {
      pressed = inside;
      Invalidate();
    }

    return true;
  }

  return false;
}

void
WndProperty::OnCancelMode() noexcept
{
  if (dragging || help_icon_pressed) {
    dragging = false;
    help_icon_pressed = false;
    pressed = false;
    Invalidate();
    ReleaseCapture();
  }

  WindowControl::OnCancelMode();
}

int
WndProperty::IncValue() noexcept
{
  if (data_field != nullptr) {
    data_field->Inc();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::DecValue() noexcept
{
  if (data_field != nullptr) {
    data_field->Dec();
    RefreshDisplay();
  }
  return 0;
}

static void
DrawInfoIcon(Canvas &canvas, const PixelRect &rc,
             bool pressed, bool enabled) noexcept
{
  const int cx = (rc.left + rc.right) / 2;
  const int cy = (rc.top + rc.bottom) / 2;
  const int r = rc.GetWidth() / 2;

  /* solid filled circle */
  Color bg;
  if (!enabled)
    bg = Color(160, 160, 160);
  else if (pressed)
    bg = Color(30, 30, 30);
  else
    bg = Color(50, 50, 50);

  canvas.Select(Brush(bg));
  canvas.SelectNullPen();
  canvas.DrawCircle({cx, cy}, r);

  /* bold dot */
  const int dot_r = std::max(2, r * 2 / 9);
  const int dot_cy = cy - r * 7 / 20;
  canvas.Select(Brush(COLOR_WHITE));
  canvas.DrawCircle({cx, dot_cy}, dot_r);

  /* bold stem with gap below dot */
  const int stem_w = std::max(2, r * 2 / 5);
  const int stem_top = dot_cy + dot_r + std::max(2, r / 5);
  const int stem_bottom = cy + r * 11 / 20;
  canvas.DrawFilledRectangle({cx - stem_w / 2, stem_top,
                              cx - stem_w / 2 + stem_w, stem_bottom},
                             COLOR_WHITE);
}

static void
DrawToggleSwitch(Canvas &canvas, const PixelRect &rc,
                 bool checked, bool focused, bool pressed,
                 bool enabled) noexcept
{
  const int height = rc.GetHeight();
  /* thumb slightly smaller than track height */
  const int thumb_d = height - Layout::VptScale(7);
  const int thumb_r = thumb_d / 2;
  /* wider track to fit ON/OFF label text */
  const int track_w = (height * 5) / 2;
  const int track_h = height;

  /* center the track vertically within rc, align right */
  const int track_right = rc.right - Layout::GetTextPadding();
  const int track_left = track_right - track_w;
  const int track_top = rc.top + (rc.GetHeight() - track_h) / 2;
  const int track_bottom = track_top + track_h;

  Color track_color;
  if (!enabled) {
    track_color = Color(180, 180, 180);
  } else if (checked) {
    track_color = pressed ? Color(0, 160, 70) : Color(52, 199, 89);
  } else {
    track_color = pressed ? Color(160, 160, 160) : Color(209, 209, 214);
  }

  canvas.Select(Brush(track_color));
  canvas.SelectNullPen();
  /* fully pill-shaped: corner radius = half height */
  canvas.DrawRoundRectangle({track_left, track_top, track_right, track_bottom},
                            {track_h, track_h});

  if (focused && !pressed) {
    canvas.SelectHollowBrush();
    canvas.Select(Pen(Layout::ScaleFinePenWidth(2), Color(0, 122, 255)));
    canvas.DrawRoundRectangle({track_left - 2, track_top - 2,
                               track_right + 2, track_bottom + 2},
                              {track_h + 4, track_h + 4});
  }

  /* thumb position */
  const int padding = (track_h - thumb_d) / 2;
  int thumb_cx;
  if (checked)
    thumb_cx = track_right - padding - thumb_r;
  else
    thumb_cx = track_left + padding + thumb_r;
  const int thumb_cy = track_top + track_h / 2;

  /* ON / OFF label in the empty side of the track */
  canvas.SetTextColor(COLOR_WHITE);
  canvas.SetBackgroundTransparent();
  {
    const char *label = checked ? _("On") : _("Off");
    const PixelSize ts = canvas.CalcTextSize(label);
    int tx, text_area_left, text_area_right;
    if (checked) {
      text_area_left  = track_left + padding;
      text_area_right = thumb_cx - thumb_r - padding / 2;
    } else {
      text_area_left  = thumb_cx + thumb_r + padding / 2;
      text_area_right = track_right - padding;
    }
    tx = text_area_left + (text_area_right - text_area_left - (int)ts.width) / 2;
    const int ty = track_top + (track_h - (int)ts.height) / 2;
    if (tx >= text_area_left)
      canvas.DrawText({tx, ty}, label);
  }

  /* thumb drawn last so it sits on top of the label */
  canvas.Select(Brush(COLOR_WHITE));
  canvas.SelectNullPen();
  canvas.DrawCircle({thumb_cx, thumb_cy}, thumb_r);
}

void
WndProperty::OnPaint(Canvas &canvas) noexcept
{
  PixelRect visible_edit_rc = edit_rc;
  const int canvas_width = (int)canvas.GetWidth();
  const int canvas_height = (int)canvas.GetHeight();

  if (visible_edit_rc.left < 0)
    visible_edit_rc.left = 0;
  if (visible_edit_rc.top < 0)
    visible_edit_rc.top = 0;
  if (visible_edit_rc.right > canvas_width)
    visible_edit_rc.right = canvas_width;
  if (visible_edit_rc.bottom > canvas_height)
    visible_edit_rc.bottom = canvas_height;

  const bool focused = HasCursorKeys() && HasFocus();
  const bool is_boolean = data_field != nullptr &&
    data_field->GetType() == DataField::Type::BOOLEAN;

  /* background and selector */
  if (pressed)
    canvas.Clear(look.list.pressed.background_color);
  else if (focused)
    canvas.Clear(look.focused.background_color);
  else if (HaveClipping())
    /* with clipping, the parent's background does not extend into
       child windows, so we must fill the background ourselves */
    canvas.Clear(look.background_color);

  if (!caption.empty()) {
    canvas.SetTextColor(focused && !pressed
                          ? look.focused.text_color
                          : look.text_color);
    canvas.SetBackgroundTransparent();
    canvas.Select(look.text_font);

    PixelSize tsize = canvas.CalcTextSize(caption.c_str());

    PixelPoint org;
    unsigned clip_width;
    if (caption_width < 0) {
      org.x = edit_rc.left;
      org.y = edit_rc.top - tsize.height;
      clip_width = canvas.GetWidth();
    } else {
      org.x = Layout::GetTextPadding();
      org.y = (canvas.GetHeight() - tsize.height) / 2;
      clip_width = caption_width;
    }

    if (org.x < 1)
      org.x = 1;

    if (HaveClipping())
      canvas.DrawText(org, caption.c_str());
    else
      canvas.DrawClippedText(org, clip_width - org.x,
                             caption.c_str());
  }

  if (is_boolean) {
    const bool checked =
      static_cast<const DataFieldBoolean *>(data_field)->GetValue();
    if (HasHelp()) {
      const PixelRect icon_rc = GetHelpIconRect(edit_rc);
      DrawInfoIcon(canvas, icon_rc, pressed && help_icon_pressed, IsEnabled());
    }
    canvas.Select(look.text_font);
    DrawToggleSwitch(canvas, visible_edit_rc, checked,
                     focused, pressed && !help_icon_pressed, IsEnabled());
    return;
  }

  Color background_color, text_color;
  if (pressed) {
    background_color = look.list.pressed.background_color;
    text_color = look.list.pressed.text_color;
  } else if (focused) {
    background_color = look.list.focused.background_color;
    text_color = look.list.focused.text_color;
  } else if (IsEnabled()) {
    if (IsReadOnly()) {
      background_color = look.ReadOnlyValueBackground();
      text_color = look.list.text_color;
    } else {
      background_color = look.list.background_color;
      text_color = look.list.text_color;
    }
  } else {
    background_color = look.dark_mode
      ? DarkColor(look.list.background_color)
      : COLOR_LIGHT_GRAY;
    text_color = look.dark_mode ? COLOR_GRAY : COLOR_DARK_GRAY;
  }

  if (!visible_edit_rc.IsEmpty()) {
    canvas.DrawFilledRectangle(visible_edit_rc, background_color);

    canvas.SelectHollowBrush();
    canvas.Select(Pen(Layout::ScaleFinePenWidth(1),
                      look.ReadOnlyValueBorderColor()));
    canvas.DrawRectangle(visible_edit_rc);
  }

  if (!value.empty() && !visible_edit_rc.IsEmpty()) {
    canvas.SetTextColor(text_color);
    canvas.SetBackgroundTransparent();
    canvas.Select(look.text_font);

    const int x = visible_edit_rc.left + Layout::GetTextPadding() * 2;
    const int control_height = visible_edit_rc.GetHeight();
    const int text_height = canvas.GetFontHeight();
    const int y = visible_edit_rc.top + (control_height - text_height) / 2;

    // determine available pixel width for text inside edit rect
    const int avail = std::max(0,
                  static_cast<int>(visible_edit_rc.GetWidth()) -
                  static_cast<int>(Layout::GetTextPadding()) * 4);

    // measure full text width
    PixelSize tsize = canvas.CalcTextSize(value.c_str());
    const int text_width = tsize.width;

    int shift = 0;
    if (alignment == Alignment::RIGHT) {
      shift = std::max(0, text_width - avail);
    } else if (alignment == Alignment::AUTO) {
      if (text_width > avail)
        shift = std::max(0, text_width - avail);
    }

    canvas.TextAutoClipped({x - shift, y}, value.c_str());
  }
}

void
WndProperty::SetText(const char *_value) noexcept
{
  assert(_value != nullptr);

  if (value.compare(_value) == 0)
    return;

  value = _value;
  Invalidate();
}

void
WndProperty::RefreshDisplay() noexcept
{
  if (!data_field)
    return;

  SetText(data_field->GetAsDisplayString());
}

void
WndProperty::SetDataField(DataField *Value) noexcept
{
  assert(data_field == nullptr || data_field != Value);

  delete data_field;
  data_field = Value;

  UpdateLayout();

  RefreshDisplay();
}
