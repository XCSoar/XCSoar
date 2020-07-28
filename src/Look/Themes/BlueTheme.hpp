#include "Screen/Color.hpp"

/**
 * additional colors (beside Screen/Color.hpp) to be used in this theme
 */
static constexpr Color COLOR_THEME_BLUE_LIGHT = Color(55,120,249);
static constexpr Color COLOR_THEME_BLUE_DARK = Color(44, 93, 191);
static constexpr Color COLOR_THEME_ORANGE_LIGHT = Color(244,174,43);
static constexpr Color COLOR_THEME_ORANGE_DARK = Color(217, 149, 39);


/**
 * BUTTON: Normal
 */
static constexpr Color COLOR_BUTTON_NORMAL_BACKGROUND_LIGHT = COLOR_THEME_BLUE_LIGHT; 
static constexpr Color COLOR_BUTTON_NORMAL_BACKGROUND_DARK = COLOR_THEME_BLUE_DARK;
static constexpr Color COLOR_BUTTON_NORMAL_FOREGROUND = COLOR_WHITE;


/**
 * BUTTON: Focussed
 */
static constexpr Color COLOR_BUTTON_FOCUSSED_BACKGROUND_LIGHT = COLOR_THEME_BLUE_LIGHT;
static constexpr Color COLOR_BUTTON_FOCUSSED_BACKGROUND_DARK = COLOR_THEME_ORANGE_LIGHT;
static constexpr Color COLOR_BUTTON_FOCUSSED_FOREGROUND = COLOR_WHITE;


/**
 * BUTTON: Pressed
 */
static constexpr Color COLOR_BUTTON_PRESSED_BACKGROUND_LIGHT = COLOR_THEME_ORANGE_LIGHT;
static constexpr Color COLOR_BUTTON_PRESSED_BACKGROUND_DARK = COLOR_THEME_ORANGE_DARK;
static constexpr Color COLOR_BUTTON_PRESSED_FOREGROUND = COLOR_WHITE;

/**
 * BUTTON: Disabled
 */
static constexpr Color COLOR_BUTTON_DISABLED_BACKGROUND_LIGHT = COLOR_LIGHT_GRAY;
static constexpr Color COLOR_BUTTON_DISABLED_BACKGROUND_DARK = COLOR_GRAY;
static constexpr Color COLOR_BUTTON_DISABLED_FOREGROUND = COLOR_GRAY;

/**
 * DIALOG
 */
static constexpr Color COLOR_DIALOG_BACKGROUND = COLOR_WHITE;
static constexpr Color COLOR_DIALOG_FOREGROUND = COLOR_BLACK;

/**
 * DIALOG Caption
 */
static constexpr Color COLOR_DIALOG_CAPTION_FOREGROUND = COLOR_WHITE;
static constexpr Color COLOR_DIALOG_CAPTION_BACKGROUND = COLOR_BLACK;
static constexpr Color COLOR_DIALOG_INACTIVE_BACKGROUND = COLOR_GRAY;


/** 
 * PROGRESSBAR
 */
static constexpr Color COLOR_PROGRESSBAR_FOREGROUND = COLOR_GRAY;

/**
 * LIST: Normal
 */
static constexpr Color COLOR_LIST_BACKGROUND = COLOR_WHITE;
static constexpr Color COLOR_LIST_FOREGROUND = COLOR_BLACK;

/** 
 * LIST: Selected 
 */
static constexpr Color COLOR_LIST_SELECTED_BACKGROUND = COLOR_THEME_ORANGE_LIGHT;
static constexpr Color COLOR_LIST_SELECTED_FOREGROUND = COLOR_BLACK;

/** 
 * LIST: Focused 
 */
static constexpr Color COLOR_LIST_FOCUSED_BACKGROUND = COLOR_THEME_ORANGE_DARK;
static constexpr Color COLOR_LIST_FOCUSED_FOREGROUND = COLOR_WHITE;

/** 
 * LIST: Pressed 
 */
static constexpr Color COLOR_LIST_PRESSED_BACKGROUND = COLOR_THEME_ORANGE_LIGHT;
static constexpr Color COLOR_LIST_PRESSED_FOREGROUND = COLOR_BLACK;
