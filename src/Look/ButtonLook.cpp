// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonLook.hpp"
#include "Colors.hpp"
#include "Asset.hpp"

void
ButtonLook::Initialise(const Font &_font, bool dark_mode)
{
  font = &_font;

  if (dark_mode) {
    standard.foreground_color = COLOR_WHITE;
    standard.foreground_brush.Create(standard.foreground_color);
    /* a dithered display knows two levels; in dark mode the page is
       the black one, so a button face is black with a white outline
       and the states move towards white */
    standard.background_color = IsDithered()
      ? COLOR_BLACK
      : COLOR_DARK_THEME_BUTTON;
    /* a smaller step from the face than in light mode: on the map a
       mid gray outline lands between the dark face and a bright
       slope instead of edging it */
    standard.ring_color = IsDithered()
      ? COLOR_WHITE
      : MixColors(COLOR_WHITE, standard.background_color, 0x35);

    /* dithered: the ring is already the strongest mark there is, so
       the face stays that of an enabled button.  Gray scale has the
       shades to back it with a tint, which reads before the ring */
    focused.foreground_color = COLOR_WHITE;
    focused.foreground_brush.Create(focused.foreground_color);
    focused.background_color = IsDithered()
      ? standard.background_color
      : (!HasColors()
         ? MixColors(COLOR_WHITE, standard.background_color, 0x20)
         : COLOR_XCSOAR);
    focused.pressed_background_color = IsDithered()
      ? COLOR_WHITE
      : (!HasColors() ? COLOR_VERY_LIGHT_GRAY : COLOR_XCSOAR_PRESSED);
    /* solid faces have no border ring in Nuxt UI */
    focused.ring_color = focused.background_color;

    /* dithered takes the level the page is not; without colors the
       light end of the scale, as light mode takes the dark one; with
       colors only half way there, or the ring fights the page */
    focus_ring_color = IsDithered()
      ? COLOR_WHITE
      : (!HasColors()
         ? COLOR_VERY_LIGHT_GRAY
         : MixColors(COLOR_XCSOAR_LIGHT, COLOR_XCSOAR, 0x80));

    disabled.color = COLOR_GRAY;
    /* a step towards the page background, and a border in that
       background color, so the face has no visible outline */
    disabled.background_color = IsDithered()
      ? COLOR_BLACK
      : MixColors(standard.background_color, COLOR_DARK_THEME_BACKGROUND, 0x66);
    disabled.ring_color = IsDithered()
      ? COLOR_BLACK
      : COLOR_DARK_THEME_BACKGROUND;
    disabled.brush.Create(disabled.color);
  } else {
    standard.foreground_color = COLOR_BLACK;
    standard.foreground_brush.Create(standard.foreground_color);
    standard.background_color = IsDithered() ? COLOR_WHITE : COLOR_BUTTON_FACE;
    /* a gray scale display needs no palette of its own: it turns the
       colors into the very shades they were picked for.  Only a
       dithered one, which has two levels and no shades at all, does */
    standard.ring_color = IsDithered() ? COLOR_BLACK : COLOR_BUTTON_RING;

    /* gray scale has the shades, but not the hue that lets a primary
       fill carry white text: the face becomes the first card below
       the white one and keeps the black caption */
    focused.foreground_color = IsDithered() || !HasColors()
      ? standard.foreground_color
      : COLOR_WHITE;
    focused.foreground_brush.Create(focused.foreground_color);
    focused.background_color = IsDithered()
      ? standard.background_color
      : (!HasColors() ? COLOR_VERY_LIGHT_GRAY : COLOR_XCSOAR);
    focused.pressed_background_color = IsDithered()
      ? COLOR_BLACK
      : COLOR_XCSOAR_PRESSED;
    /* solid faces have no border ring in Nuxt UI; the focus is
       marked by the ring outside the face */
    focused.ring_color = focused.background_color;

    /* dithered takes the level the page is not; without colors the
       shadow below a light card, because a lighter ring has nowhere
       to go; with colors the light primary, like ring-primary-300 */
    focus_ring_color = IsDithered()
      ? COLOR_BLACK
      : (!HasColors() ? COLOR_DARK_GRAY : COLOR_XCSOAR_LIGHT);

    if (IsDithered()) {
      /* no shades available: keep the white face and let the grey
         caption carry the state */
      disabled.color = COLOR_GRAY;
      disabled.background_color = standard.background_color;
      disabled.ring_color = COLOR_WHITE;
    } else {
      /* a step towards the page background, and a border in that
         background color, so the face has no visible outline */
      disabled.color = COLOR_BUTTON_DISABLED_TEXT;
      disabled.background_color = COLOR_BUTTON_DISABLED;
      disabled.ring_color = COLOR_DIALOG_BACKGROUND;
    }
    disabled.brush.Create(disabled.color);
  }

  /* unless a state says otherwise, a pressed button keeps its
     caption color */
  focused.pressed_foreground_color = IsDithered() || !HasColors()
    /* the pressed face is the opposite level of the focused one, and
       without colors it is several shades away from it, so the two
       cannot share a caption color */
    ? (dark_mode ? COLOR_BLACK : COLOR_WHITE)
    : focused.foreground_color;
  focused.pressed_foreground_brush.Create(focused.pressed_foreground_color);
}
