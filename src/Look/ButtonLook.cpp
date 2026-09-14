// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonLook.hpp"
#include "Colors.hpp"
#include "Asset.hpp"

#include <initializer_list>

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
    /* pressed goes down the scale, like a Tailwind
       `active:bg-gray-800` below a gray-700 face.  Without colors
       there is no scale below the face to go down on, so it mirrors
       the light mode and steps towards white instead */
    standard.pressed_background_color = IsDithered()
      ? COLOR_WHITE
      : (!HasColors()
         ? MixColors(COLOR_WHITE, standard.background_color, 0x28)
         : MixColors(COLOR_BLACK, standard.background_color, 0x50));
    /* a smaller step from the face than in light mode: on the map a
       mid gray outline lands between the dark face and a bright
       slope instead of edging it */
    standard.ring_color = IsDithered()
      ? COLOR_WHITE
      : MixColors(COLOR_WHITE, standard.background_color, 0x35);

    /* "soft" primary tint on the dark background */
    selected.foreground_color = COLOR_WHITE;
    selected.foreground_brush.Create(selected.foreground_color);
    selected.background_color =
      MixColors(COLOR_XCSOAR, COLOR_DARK_THEME_BACKGROUND, 0x99);
    selected.pressed_background_color =
      MixColors(COLOR_XCSOAR, COLOR_DARK_THEME_BACKGROUND, 0x4d);
    selected.ring_color = selected.background_color;

    /* the focused face is the pattern of a dark gray, one step off
       the black page, and the pressed one flips to white.  Without
       colors the states line up the way they do in light mode, only
       towards white instead of towards black */
    focused.foreground_color = COLOR_WHITE;
    focused.foreground_brush.Create(focused.foreground_color);
    focused.background_color = IsDithered()
      ? COLOR_DARK_GRAY
      : (!HasColors()
         ? MixColors(COLOR_WHITE, standard.background_color, 0x20)
         : COLOR_XCSOAR);
    focused.pressed_background_color = IsDithered()
      ? COLOR_WHITE
      : (!HasColors() ? COLOR_VERY_LIGHT_GRAY : COLOR_XCSOAR_PRESSED);
    /* solid faces have no border ring in Nuxt UI */
    focused.ring_color = focused.background_color;

    /* the palette's light primary, like Tailwind's
       ring-primary-300: lighter than face and page, but saturated
       enough not to read as a glow */
    /* both rings are the pattern of a mid gray on a dithered display:
       white would read as a second border on the dark page.  Without
       colors they take the light end of the scale, as they take the
       dark one in light mode */
    focus_ring_color = IsDithered()
      ? COLOR_GRAY
      : (!HasColors() ? COLOR_VERY_LIGHT_GRAY : COLOR_XCSOAR_LIGHT);
    selected_ring_color = IsDithered()
      ? COLOR_GRAY
      : (!HasColors() ? COLOR_WHITE : COLOR_XCSOAR_PRESSED);

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
    standard.pressed_background_color = IsDithered()
      ? COLOR_VERY_LIGHT_GRAY
      : COLOR_BUTTON_PRESSED;
    standard.ring_color = IsDithered() ? COLOR_BLACK : COLOR_BUTTON_RING;

    /* "soft" primary tint: light primary wash with primary text */
    if (IsDithered()) {
      selected.foreground_color = COLOR_BLACK;
      selected.background_color = COLOR_VERY_LIGHT_GRAY;
      selected.ring_color = COLOR_BLACK;
    } else {
      selected.foreground_color = COLOR_XCSOAR_DARK;
      /* one shade deeper than a classic primary-50 wash, so the
         tint remains visible on the page background */
      selected.background_color = MixColors(COLOR_XCSOAR, COLOR_WHITE, 0x40);
    }
    selected.foreground_brush.Create(selected.foreground_color);
    if (IsDithered())
      selected.pressed_background_color = COLOR_VERY_LIGHT_GRAY;
    else {
      /* several steps down the primary scale */
      selected.pressed_background_color =
        MixColors(COLOR_XCSOAR, COLOR_WHITE, 0x99);
    }

    /* gray scale has the shades, but not the hue that lets a primary
       fill carry white text: the face becomes the first card below
       the white one and keeps the black caption */
    focused.foreground_color = IsDithered() || !HasColors()
      ? COLOR_BLACK
      : COLOR_WHITE;
    focused.foreground_brush.Create(focused.foreground_color);
    focused.background_color = IsDithered()
      ? COLOR_LIGHT_GRAY
      : (!HasColors() ? COLOR_VERY_LIGHT_GRAY : COLOR_XCSOAR);
    focused.pressed_background_color = IsDithered()
      ? COLOR_BLACK
      : COLOR_XCSOAR_PRESSED;
    /* solid faces have no border ring in Nuxt UI; the focus is
       marked by the ring outside the face */
    focused.ring_color = focused.background_color;

    /* the palette's light primary, like Tailwind's
       ring-primary-300: lighter than the face, but fully saturated
       instead of a white-washed pastel, which read as a pale glow */
    /* the ring of a focused button is the pattern of a mid gray on a
       dithered display: solid black would read as a second border.
       Without colors it is the shadow below a light card, because a
       ring lighter than the face has nowhere left to go above it */
    focus_ring_color = IsDithered()
      ? COLOR_GRAY
      : (!HasColors() ? COLOR_DARK_GRAY : COLOR_XCSOAR_LIGHT);
    /* the ring of a selected button is the pattern of a mid gray on a
       dithered display: black would melt into its face.  The two
       states share one face, so without colors the rings carry them
       alone and this one goes all the way down */
    selected_ring_color = IsDithered()
      ? COLOR_GRAY
      : (!HasColors() ? COLOR_BLACK : COLOR_XCSOAR_PRESSED);

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
  for (StateLook *state : {&standard, &selected, &focused}) {
    state->pressed_foreground_color = state->foreground_color;
    state->pressed_foreground_brush.Create(state->pressed_foreground_color);
  }

  if (IsDithered() || !HasColors()) {
    /* the pressed face is the opposite level of the focused one, and
       without colors it is several shades away from it, so the two
       cannot share a caption color */
    focused.pressed_foreground_color = dark_mode ? COLOR_BLACK : COLOR_WHITE;
    focused.pressed_foreground_brush.Create(focused.pressed_foreground_color);
  }
}
