include build/rsvg.mk
include build/imagemagick.mk

USE_WIN32_RESOURCES = $(call bool_and,$(HAVE_WIN32),$(call bool_not,$(ENABLE_SDL)))

ifeq ($(USE_WIN32_RESOURCES),y)
TARGET_CPPFLAGS += -DUSE_WIN32_RESOURCES
endif

####### market icons

SVG_MARKET_ICONS = Data/graphics/logo.svg Data/graphics/logo_red.svg
PNG_MARKET_ICONS = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_market.png,$(SVG_MARKET_ICONS))

market-icons: $(PNG_MARKET_ICONS)
$(eval $(call rsvg-convert,$(PNG_MARKET_ICONS),$(DATA)/graphics/%_market.png,Data/graphics/%.svg,--width=512))

####### bitmaps

BMP_BITMAPS = $(wildcard Data/bitmaps/*.bmp)
PNG_BITMAPS = $(patsubst Data/bitmaps/%.bmp,$(DATA)/bitmaps/%.png,$(BMP_BITMAPS))

$(PNG_BITMAPS): $(DATA)/bitmaps/%.png: Data/bitmaps/%.bmp | $(DATA)/bitmaps/dirstamp
	$(Q)$(IM_CONVERT) +dither -type GrayScale -define png:color-type=0 $< $@

####### icons

SVG_ICONS = $(wildcard Data/icons/*.svg)
SVG_NOALIAS_ICONS = $(patsubst Data/icons/%.svg,$(DATA)/icons/%.svg,$(SVG_ICONS))

BMP_ICONS_ALL =

define generate-icon-scale
PNG_ICONS_$(1) = $$(patsubst Data/icons/%.svg,$$(DATA)/icons/%_$(1).png,$$(SVG_ICONS))
BMP_ICONS_$(1) = $$(PNG_ICONS_$(1):.png=.bmp)
BMP_ICONS_ALL += $$(BMP_ICONS_$(1))
$$(eval $$(call rsvg-convert,$$(PNG_ICONS_$(1)),$$(DATA)/icons/%_$(1).png,$$(DATA)/icons/%.svg,--x-zoom=$2 --y-zoom=$2))
endef

# Default 100PPI (eg 320x240 4" display)
$(eval $(call generate-icon-scale,96,1.0))

#160PPI (eg 640x480 5" display)
$(eval $(call generate-icon-scale,160,1.6316))

# 300dpi
$(eval $(call generate-icon-scale,300,3.0))

# modify working copy of SVG to improve rendering
$(SVG_NOALIAS_ICONS): $(DATA)/icons/%.svg: build/svg_preprocess.xsl Data/icons/%.svg | $(DATA)/icons/dirstamp
	@$(NQ)echo "  XSLT    $@"
	$(Q)xsltproc --nonet --stringparam DisableAA_Select "MASK_NOAA_" --output $@ $^

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp,$(BMP_ICONS_ALL),%.bmp,%_tile.png))

####### splash logo

SVG_SPLASH = Data/graphics/logo.svg Data/graphics/logo_red.svg
PNG_SPLASH_320 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_320.png,$(SVG_SPLASH))
BMP_SPLASH_320 = $(PNG_SPLASH_320:.png=.bmp)
PNG_SPLASH_160 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_160.png,$(SVG_SPLASH))
BMP_SPLASH_160 = $(PNG_SPLASH_160:.png=.bmp)
PNG_SPLASH_80 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_80.png,$(SVG_SPLASH))
BMP_SPLASH_80 = $(PNG_SPLASH_80:.png=.bmp)
PNG_SPLASH_160_RGBA = $(patsubst Data/graphics/%.svg,$(DATA)/graphics2/%_160_rgba.png,$(SVG_SPLASH))

# render from SVG to PNG
$(eval $(call rsvg-convert,$(PNG_SPLASH_320),$(DATA)/graphics/%_320.png,Data/graphics/%.svg,--width=320))
$(eval $(call rsvg-convert,$(PNG_SPLASH_160),$(DATA)/graphics/%_160.png,Data/graphics/%.svg,--width=160))
$(eval $(call rsvg-convert,$(PNG_SPLASH_80),$(DATA)/graphics/%_80.png,Data/graphics/%.svg,--width=80))
$(eval $(call rsvg-convert,$(PNG_SPLASH_160_RGBA),$(DATA)/graphics2/%_160_rgba.png,Data/graphics/%.svg,--width=160))

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp-white,$(BMP_SPLASH_320) $(BMP_SPLASH_160) $(BMP_SPLASH_80),%.bmp,%.png))

# macOS-specific: generate 1024px PNG and convert to .icns (macOS icon format)
ifeq ($(TARGET_IS_OSX),y)
PNG_SPLASH_1024 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_1024.png,$(SVG_SPLASH))
ICNS_SPLASH_1024 = $(PNG_SPLASH_1024:.png=.icns)

# render 1024px PNG from SVG (needed for macOS icon)
$(eval $(call rsvg-convert,$(PNG_SPLASH_1024),$(DATA)/graphics/%_1024.png,Data/graphics/%.svg,--width=1024))

# convert to icns (macOS icon) using macOS-specific tools
$(ICNS_SPLASH_1024): %.icns: %.png
	@$(NQ)echo "  ICNS    $@"
	$(Q)mkdir -p $@.iconset && \
		sips -z 1024 1024 $< --out $@.iconset/icon_512x512@2x.png >/dev/null && \
		sips -z 512 512 $< --out $@.iconset/icon_512x512.png >/dev/null && \
		sips -z 512 512 $< --out $@.iconset/icon_256x256@2x.png >/dev/null && \
		sips -z 256 256 $< --out $@.iconset/icon_256x256.png >/dev/null && \
		sips -z 256 256 $< --out $@.iconset/icon_128x128@2x.png >/dev/null && \
		sips -z 128 128 $< --out $@.iconset/icon_128x128.png >/dev/null && \
		sips -z 64 64 $< --out $@.iconset/icon_32x32@2x.png >/dev/null && \
		sips -z 32 32 $< --out $@.iconset/icon_32x32.png >/dev/null && \
		sips -z 32 32 $< --out $@.iconset/icon_16x16@2x.png >/dev/null && \
		sips -z 16 16 $< --out $@.iconset/icon_16x16.png >/dev/null && \
		iconutil -c icns $@.iconset -o $@ && \
		rm -rf $@.iconset
endif

####### version

SVG_TITLE = Data/graphics/title.svg Data/graphics/title_red.svg
PNG_TITLE_110 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_110.png,$(SVG_TITLE))
BMP_TITLE_110 = $(PNG_TITLE_110:.png=.bmp)
PNG_TITLE_320 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_320.png,$(SVG_TITLE))
BMP_TITLE_320 = $(PNG_TITLE_320:.png=.bmp)
PNG_TITLE_640 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_640.png,$(SVG_TITLE))
BMP_TITLE_640 = $(PNG_TITLE_640:.png=.bmp)
PNG_TITLE_320_RGBA = $(patsubst Data/graphics/%.svg,$(DATA)/graphics2/%_320_rgba.png,$(SVG_TITLE))

SVG_TITLE_WHITE = Data/graphics/title_white.svg Data/graphics/title_red_white.svg
PNG_TITLE_WHITE_320_RGBA = $(patsubst Data/graphics/%.svg,$(DATA)/graphics2/%_320_rgba.png,$(SVG_TITLE_WHITE))
PNG_TITLE_WHITE_640_RGBA = $(patsubst Data/graphics/%.svg,$(DATA)/graphics2/%_640_rgba.png,$(SVG_TITLE_WHITE))

# render from SVG to PNG
$(eval $(call rsvg-convert,$(PNG_TITLE_110),$(DATA)/graphics/%_110.png,Data/graphics/%.svg,--width=110))
$(eval $(call rsvg-convert,$(PNG_TITLE_320),$(DATA)/graphics/%_320.png,Data/graphics/%.svg,--width=320))
$(eval $(call rsvg-convert,$(PNG_TITLE_640),$(DATA)/graphics/%_640.png,Data/graphics/%.svg,--width=640))
$(eval $(call rsvg-convert,$(PNG_TITLE_320_RGBA),$(DATA)/graphics2/%_320_rgba.png,Data/graphics/%.svg,--width=320))
$(eval $(call rsvg-convert,$(PNG_TITLE_WHITE_320_RGBA),$(DATA)/graphics2/%_320_rgba.png,Data/graphics/%.svg,--width=320))
$(eval $(call rsvg-convert,$(PNG_TITLE_WHITE_640_RGBA),$(DATA)/graphics2/%_640_rgba.png,Data/graphics/%.svg,--width=640))

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp-white,$(BMP_TITLE_110) $(BMP_TITLE_320) $(BMP_TITLE_640),%.bmp,%.png))

####### dialog title

SVG_DIALOG_TITLE = Data/graphics/dialog_title.svg Data/graphics/dialog_title_red.svg
PNG_DIALOG_TITLE = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%.png,$(SVG_DIALOG_TITLE))
BMP_DIALOG_TITLE = $(PNG_DIALOG_TITLE:.png=.bmp)

# render from SVG to PNG
$(eval $(call rsvg-convert,$(PNG_DIALOG_TITLE),$(DATA)/graphics/%.png,Data/graphics/%.svg,))

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp-white,$(BMP_DIALOG_TITLE),%.bmp,%.png))

####### progress bar border

SVG_PROGRESS_BORDER = Data/graphics/progress_border.svg Data/graphics/progress_border_red.svg
PNG_PROGRESS_BORDER = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%.png,$(SVG_PROGRESS_BORDER))
BMP_PROGRESS_BORDER = $(PNG_PROGRESS_BORDER:.png=.bmp)

# render from SVG to PNG
$(eval $(call rsvg-convert,$(PNG_PROGRESS_BORDER),$(DATA)/graphics/%.png,Data/graphics/%.svg,))

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp-white,$(BMP_PROGRESS_BORDER),%.bmp,%.png))

####### launcher graphics

SVG_LAUNCH = Data/graphics/launcher.svg Data/graphics/launcher_red.svg
PNG_LAUNCH_640 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_640.png,$(SVG_LAUNCH))
BMP_LAUNCH_FLY_640 = $(PNG_LAUNCH_640:.png=_1.bmp)
BMP_LAUNCH_SIM_640 = $(PNG_LAUNCH_640:.png=_2.bmp)
BMP_LAUNCH_DLL_FLY_640 = $(PNG_LAUNCH_640:.png=_dll_1.bmp)
BMP_LAUNCH_DLL_SIM_640 = $(PNG_LAUNCH_640:.png=_dll_2.bmp)

BMP_LAUNCH_ALL = $(BMP_LAUNCH_FLY_640) $(BMP_LAUNCH_SIM_640)
ifeq ($(USE_WIN32_RESOURCES),y)
BMP_LAUNCH_ALL += $(BMP_LAUNCH_DLL_FLY_640) $(BMP_LAUNCH_DLL_SIM_640)
endif

# render from SVG to PNG
$(eval $(call rsvg-convert,$(PNG_LAUNCH_640),$(DATA)/graphics/%_640.png,Data/graphics/%.svg,--width=640))

# split into two uncompressed 8-bit BMPs (single 'convert' operation)
$(eval $(call convert-to-bmp-half,$(BMP_LAUNCH_FLY_640),%_1.bmp,%.png,-background white))
$(BMP_LAUNCH_SIM_640): $(BMP_LAUNCH_FLY_640)

# split into two uncompressed 8-bit BMPs (single 'convert' operation)
$(eval $(call convert-to-bmp-half,$(BMP_LAUNCH_DLL_FLY_640),%_dll_1.bmp,%.png,-background blue))
$(BMP_LAUNCH_DLL_SIM_640): $(BMP_LAUNCH_DLL_FLY_640)

# back to PNG

PNG_LAUNCH_ALL = $(patsubst %.bmp,%.png,$(BMP_LAUNCH_ALL))
$(PNG_LAUNCH_ALL): %.png: %.bmp
	$(Q)$(IM_CONVERT) $< $@

####### sounds

ifneq ($(TARGET),ANDROID)
ifneq ($(TARGET),IOS)
ifneq ($(HAVE_WIN32),y)

WAV_SOUNDS = $(wildcard Data/sound/*.wav)
RAW_SOUNDS = $(patsubst Data/sound/%.wav,$(DATA)/sound/%.raw,$(WAV_SOUNDS))

$(RAW_SOUNDS): $(DATA)/sound/%.raw: Data/sound/%.wav | $(DATA)/sound/dirstamp
	@$(NQ)echo "  SOX     $@"
	$(Q)sox -V1 $< --bits 16 --rate 44100 --channels 1 $@

endif
endif
endif

#######

TEXT_FILES = AUTHORS COPYING NEWS.txt

TEXT_COMPRESSED = $(patsubst %,$(DATA)/%.gz,$(TEXT_FILES))
$(TEXT_COMPRESSED): $(DATA)/%.gz: % | $(DATA)/dirstamp
	@$(NQ)echo "  GZIP    $@"
	$(Q)gzip --best <$< >$@.tmp
	$(Q)mv $@.tmp $@

RESOURCE_FILES =

# Stamp file to track XCSOAR_TESTING state (what actually affects resources.txt)
# For Android: based on package name (org.xcsoar.testing)
# For non-Android: based on TESTING flag
RESOURCE_FLAGS_STAMP = $(TARGET_OUTPUT_DIR)/.resource_flags.stamp
$(RESOURCE_FLAGS_STAMP): FORCE | $(TARGET_OUTPUT_DIR)/dirstamp
	@if [ "$(TARGET_IS_ANDROID)" = "y" ]; then \
		if [ "$(FOSS)" = "y" ]; then pkg=org.xcsoar.foss; \
		elif [ "$(PLAY)" = "y" ]; then pkg=org.xcsoar.play; \
		elif [ "$(TESTING)" = "y" ]; then pkg=org.xcsoar.testing; \
		else pkg=org.xcsoar; fi; \
		if [ "$$pkg" = "org.xcsoar.testing" ]; then \
			value=y; \
		else \
			value=n; \
		fi; \
	else \
		value=$(TESTING); \
	fi; \
	if [ ! -f $@ ] || [ "$$(cat $@ 2>/dev/null)" != "XCSOAR_TESTING=$$value" ]; then \
		echo "XCSOAR_TESTING=$$value" > $@.tmp && mv $@.tmp $@; \
	fi

$(TARGET_OUTPUT_DIR)/resources.txt: Data/resources.txt $(RESOURCE_FLAGS_STAMP) | $(TARGET_OUTPUT_DIR)/dirstamp $(BUILD_TOOLCHAIN_TARGET)
	@$(NQ)echo "  CPP     $@"
	$(Q)cat $< |$(CC) -E -o $@ -I$(OUT)/include $(TARGET_CPPFLAGS) $(OPENGL_CPPFLAGS) $(GDI_CPPFLAGS) -

RANDOM_NUMBER := $(shell od -vAn -N4 -tu4 < /dev/urandom| tr -d ' ')

$(TARGET_OUTPUT_DIR)/include/MakeResource.hpp: $(TARGET_OUTPUT_DIR)/resources.txt tools/GenerateMakeResource.pl | $(TARGET_OUTPUT_DIR)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/GenerateMakeResource.pl <$< >$@.$(RANDOM_NUMBER).tmp
	$(Q)mv $@.$(RANDOM_NUMBER).tmp $@

$(TARGET_OUTPUT_DIR)/include/ResourceLookup_entries.cpp: $(TARGET_OUTPUT_DIR)/resources.txt tools/GenerateResourceLookup.pl | $(TARGET_OUTPUT_DIR)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/GenerateResourceLookup.pl <$< >$@.tmp
	$(Q)mv $@.tmp $@

$(call SRC_TO_OBJ,$(SRC)/ResourceLookup.cpp): $(TARGET_OUTPUT_DIR)/include/ResourceLookup_entries.cpp $(TARGET_OUTPUT_DIR)/include/MakeResource.hpp

ifeq ($(TARGET_IS_ANDROID),n)
ifneq ($(TARGET),IOS)

ifeq ($(USE_WIN32_RESOURCES),y)
RESOURCE_FILES += $(BMP_BITMAPS)
else
RESOURCE_FILES += $(PNG_BITMAPS)
endif

####### permission disclosure graphics

SVG_DISCLOSURE = Data/graphics/location_pin.svg Data/graphics/notification_bell.svg Data/graphics/bluetooth.svg Data/graphics/warning_triangle.svg
PNG_DISCLOSURE_DST = $(patsubst Data/graphics/%.svg,$(DATA)/graphics2/%.png,$(SVG_DISCLOSURE))
PNG_DISCLOSURE_WIN = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%.png,$(SVG_DISCLOSURE))
BMP_DISCLOSURE_WIN = $(PNG_DISCLOSURE_WIN:.png=.bmp)

$(eval $(call rsvg-convert,$(PNG_DISCLOSURE_DST), \
	$(DATA)/graphics2/%.png, \
	Data/graphics/%.svg, \
	--width=80 --height=80))
$(eval $(call rsvg-convert,$(PNG_DISCLOSURE_WIN), \
	$(DATA)/graphics/%.png, \
	Data/graphics/%.svg, \
	--width=80 --height=80))
$(eval $(call convert-to-bmp-white,$(BMP_DISCLOSURE_WIN),%.bmp,%.png))

####### add gesture icons from docs

GESTURES = down dl dr du left ldr ldrdl right rd rl up ud uldr urd urdl
GESTURES_DST = $(addprefix $(DATA)/graphics2/gesture_, \
	$(addsuffix .png,$(GESTURES)))
GESTURES_PNG_WIN = $(addprefix $(DATA)/graphics/gesture_, \
	$(addsuffix .png,$(GESTURES)))
GESTURES_BMP_WIN = $(GESTURES_PNG_WIN:.png=.bmp)

$(DATA)/graphics2/dirstamp:
	@$(NQ)echo "  MKDIR   $(DATA)/graphics2/"
	$(Q)mkdir -p $(DATA)/graphics2
	@touch $@

$(eval $(call rsvg-convert,$(GESTURES_DST), \
	$(DATA)/graphics2/gesture_%.png, \
	doc/manual/figures/gesture_%.svg, \
	--width=82 --height=82))
$(eval $(call rsvg-convert,$(GESTURES_PNG_WIN), \
	$(DATA)/graphics/gesture_%.png, \
	doc/manual/figures/gesture_%.svg, \
	--width=82 --height=82))
$(eval $(call convert-to-bmp-white,$(GESTURES_BMP_WIN),%.bmp,%.png))

RESOURCE_FILES += $(GESTURES_DST)
RESOURCE_FILES += $(PNG_DISCLOSURE_DST)
RESOURCE_FILES += $(BMP_ICONS_ALL)
RESOURCE_FILES += $(BMP_SPLASH_320) $(BMP_SPLASH_160) $(BMP_SPLASH_80)
RESOURCE_FILES += $(BMP_DIALOG_TITLE) $(BMP_PROGRESS_BORDER)
RESOURCE_FILES += $(BMP_TITLE_640) $(BMP_TITLE_320) $(BMP_TITLE_110)
ifneq ($(USE_WIN32_RESOURCES),y)
RESOURCE_FILES += $(PNG_SPLASH_160_RGBA)
RESOURCE_FILES += $(PNG_TITLE_320_RGBA)
RESOURCE_FILES += $(PNG_TITLE_WHITE_320_RGBA)
RESOURCE_FILES += $(PNG_TITLE_WHITE_640_RGBA)
endif
RESOURCE_FILES += $(BMP_LAUNCH_ALL)

RESOURCE_FILES += $(RAW_SOUNDS)

ifeq ($(USE_WIN32_RESOURCES),n)

$(patsubst $(DATA)/icons/%.bmp,$(DATA)/icons2/%.png,$(filter $(DATA)/icons/%.bmp,$(RESOURCE_FILES))): $(DATA)/icons2/%.png: $(DATA)/icons/%.bmp | $(DATA)/icons2/dirstamp
	$(Q)$(IM_CONVERT) $< $@

$(patsubst $(DATA)/graphics/%.bmp,$(DATA)/graphics2/%.png,$(filter $(DATA)/graphics/%.bmp,$(RESOURCE_FILES))): $(DATA)/graphics2/%.png: $(DATA)/graphics/%.bmp | $(DATA)/graphics2/dirstamp
	$(Q)$(IM_CONVERT) $< $@

RESOURCE_FILES := $(patsubst $(DATA)/graphics/%.bmp,$(DATA)/graphics2/%.png,$(RESOURCE_FILES))
RESOURCE_FILES := $(patsubst $(DATA)/icons/%.bmp,$(DATA)/icons2/%.png,$(RESOURCE_FILES))
RESOURCE_FILES := $(patsubst %.bmp,%.png,$(RESOURCE_FILES))
endif #!USE_WIN32_RESOURCES

ifeq ($(USE_WIN32_RESOURCES),y)
RESOURCE_FILES += $(GESTURES_BMP_WIN)
RESOURCE_FILES += $(BMP_DISCLOSURE_WIN)
endif #USE_WIN32_RESOURCES

endif #TARGET!=IOS
endif #!TARGET_IS_ANDROID

ifeq ($(TARGET_IS_ANDROID),n)

ifeq ($(USE_WIN32_RESOURCES),y)

$(TARGET_OUTPUT_DIR)/XCSoar.rc: $(TARGET_OUTPUT_DIR)/resources.txt Data/XCSoar.rc tools/GenerateWindowsResources.pl
	@$(NQ)echo "  GEN     $@"
	$(Q)cp Data/XCSoar.rc $@.tmp
	$(Q)$(PERL) tools/GenerateWindowsResources.pl $< >>$@.tmp
	$(Q)mv $@.tmp $@

$(TARGET_OUTPUT_DIR)/include/resource.h: $(TARGET_OUTPUT_DIR)/include/MakeResource.hpp | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) -ne 'print "#define $$1 $$2\n" if /^MAKE_RESOURCE\((\w+), \S+, (\d+)\);/;' $< >$@.tmp
	$(Q)mv $@.tmp $@

RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/XCSoar.rsc

$(TARGET_OUTPUT_DIR)/XCSoar.rsc: %.rsc: %.rc $(TARGET_OUTPUT_DIR)/include/resource.h $(RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/%/../dirstamp $(BUILD_TOOLCHAIN_TARGET)
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) --include-dir output/data --include-dir Data -o $@ $<

else # USE_WIN32_RESOURCES

$(TARGET_OUTPUT_DIR)/resources.c: export TARGET_IS_ANDROID:=$(TARGET_IS_ANDROID)
$(TARGET_OUTPUT_DIR)/resources.c: export ENABLE_OPENGL:=$(OPENGL)
$(TARGET_OUTPUT_DIR)/resources.c: $(TARGET_OUTPUT_DIR)/resources.txt $(RESOURCE_FILES) tools/LinkResources.pl tools/BinToC.pm | $(TARGET_OUTPUT_DIR)/resources/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/LinkResources.pl <$< >$@.tmp
	$(Q)mv $@.tmp $@

RESOURCES_SOURCES = $(TARGET_OUTPUT_DIR)/resources.c
$(eval $(call link-library,resources,RESOURCES))
RESOURCE_BINARY = $(RESOURCES_BIN)

endif

endif # !TARGET_IS_ANDROID
