# Bundle DejaVu fonts for Windows OpenGL builds
# These fonts are copied to $(TARGET_BIN_DIR)/fonts/ so the
# application can find them next to the executable.

ifeq ($(HAVE_WIN32)$(FREETYPE),yy)

# Source directory on the build host containing DejaVu TTF files.
# Callers can override via: make FONT_SRC_DIR=/path/to/dejavu
# Auto-detected from common installation paths if not explicitly set.
FONT_SRC_DIR ?= $(firstword $(wildcard \
  /usr/share/fonts/truetype/dejavu \
  /usr/share/fonts/dejavu \
  /usr/share/fonts/TTF \
  /opt/homebrew/share/fonts/dejavu-fonts-ttf \
  /usr/local/share/fonts/dejavu-fonts-ttf))

# Font files to bundle
FONT_NAMES = \
	DejaVuSansCondensed.ttf \
	DejaVuSansCondensed-Bold.ttf \
	DejaVuSansCondensed-Oblique.ttf \
	DejaVuSansCondensed-BoldOblique.ttf \
	DejaVuSansMono.ttf

FONT_DEST_DIR = $(TARGET_BIN_DIR)/fonts
FONT_TARGETS = $(addprefix $(FONT_DEST_DIR)/,$(FONT_NAMES))

ifeq ($(FONT_SRC_DIR),)
$(error Cannot find DejaVu fonts on the build host. \
  Install fonts-dejavu-core (Debian/Ubuntu), dejavu-fonts (Fedora/Arch), \
  or set FONT_SRC_DIR=/path/to/dir containing DejaVuSansCondensed.ttf. \
  Fonts are needed to populate $(FONT_DEST_DIR))
endif

$(FONT_DEST_DIR)/dirstamp: | $(TARGET_BIN_DIR)/dirstamp
	$(Q)$(MKDIR) -p $(FONT_DEST_DIR)
	@touch $@

$(FONT_DEST_DIR)/%.ttf: $(FONT_SRC_DIR)/%.ttf | $(FONT_DEST_DIR)/dirstamp
	@$(NQ)echo "  COPY    fonts/$(@F)"
	$(Q)cp $< $@

endif
