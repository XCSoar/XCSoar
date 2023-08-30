set (BIN_FILES
   AUTHORS.gz
   COPYING.gz
   OpenSoar-News.md.gz
   NEWS.txt.gz
   Data/other/egm96s.dem
)

file(GLOB ICON_FILES        "icons/*.svg")
set(GRAPHIC_FILES )
list(APPEND GRAPHIC_FILES   "${_DATA_INPUT}/graphics/logo.svg")
list(APPEND GRAPHIC_FILES   "${_DATA_INPUT}/graphics/progress_border.svg")
list(APPEND GRAPHIC_FILES   "${_DATA_INPUT}/graphics/title.svg")

set(SCRIPT_FILES )
set(C_FILES)  # Reset to empty...

