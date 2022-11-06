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
	$(Q)$(IM_PREFIX)convert +dither -type GrayScale -define png:color-type=0 $< $@

####### icons

SVG_ICONS = $(wildcard Data/icons/*.svg)
SVG_NOALIAS_ICONS = $(patsubst Data/icons/%.svg,$(DATA)/icons/%.svg,$(SVG_ICONS))
PNG_ICONS = $(patsubst Data/icons/%.svg,$(DATA)/icons/%.png,$(SVG_ICONS))
BMP_ICONS = $(PNG_ICONS:.png=.bmp)
PNG_ICONS_160 = $(patsubst Data/icons/%.svg,$(DATA)/icons/%_160.png,$(SVG_ICONS))
BMP_ICONS_160 = $(PNG_ICONS_160:.png=.bmp)

# modify working copy of SVG to improve rendering
$(SVG_NOALIAS_ICONS): $(DATA)/icons/%.svg: build/svg_preprocess.xsl Data/icons/%.svg | $(DATA)/icons/dirstamp
	@$(NQ)echo "  XSLT    $@"
	$(Q)xsltproc --nonet --stringparam DisableAA_Select "MASK_NOAA_" --output $@ $^

# render from SVG to PNG
# Default 100PPI (eg 320x240 4" display)
$(eval $(call rsvg-convert,$(PNG_ICONS),$(DATA)/icons/%.png,$(DATA)/icons/%.svg,--width=13,height=13))

#160PPI (eg 640x480 5" display)
$(eval $(call rsvg-convert,$(PNG_ICONS_160),$(DATA)/icons/%_160.png,$(DATA)/icons/%.svg,--width=22,--height=22))

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp,$(BMP_ICONS) $(BMP_ICONS_160),%.bmp,%_tile.png))

####### splash logo

SVG_SPLASH = Data/graphics/logo.svg Data/graphics/logo_red.svg
PNG_SPLASH_320 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_320.png,$(SVG_SPLASH))
BMP_SPLASH_320 = $(PNG_SPLASH_320:.png=.bmp)
PNG_SPLASH_160 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_160.png,$(SVG_SPLASH))
BMP_SPLASH_160 = $(PNG_SPLASH_160:.png=.bmp)
PNG_SPLASH_80 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_80.png,$(SVG_SPLASH))
BMP_SPLASH_80 = $(PNG_SPLASH_80:.png=.bmp)
PNG_SPLASH_1024 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_1024.png,$(SVG_SPLASH))
ICNS_SPLASH_1024 = $(PNG_SPLASH_1024:.png=.icns)

# render from SVG to PNG
$(eval $(call rsvg-convert,$(PNG_SPLASH_320),$(DATA)/graphics/%_320.png,Data/graphics/%.svg,--width=320))
$(eval $(call rsvg-convert,$(PNG_SPLASH_160),$(DATA)/graphics/%_160.png,Data/graphics/%.svg,--width=160))
$(eval $(call rsvg-convert,$(PNG_SPLASH_80),$(DATA)/graphics/%_80.png,Data/graphics/%.svg,--width=80))
$(eval $(call rsvg-convert,$(PNG_SPLASH_1024),$(DATA)/graphics/%_1024.png,Data/graphics/%.svg,--width=1024))

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp-white,$(BMP_SPLASH_160) $(BMP_SPLASH_80),%.bmp,%.png))

# convert to icns (mac os x icon)
$(ICNS_SPLASH_1024): %.icns: %.png
	@$(NQ)echo "  ICNS    $@"
	$(Q)$(IM_PREFIX)png2icns $@ $<

####### version

SVG_TITLE = Data/graphics/title.svg Data/graphics/title_red.svg
PNG_TITLE_110 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_110.png,$(SVG_TITLE))
BMP_TITLE_110 = $(PNG_TITLE_110:.png=.bmp)
PNG_TITLE_320 = $(patsubst Data/graphics/%.svg,$(DATA)/graphics/%_320.png,$(SVG_TITLE))
BMP_TITLE_320 = $(PNG_TITLE_320:.png=.bmp)

# render from SVG to PNG
$(eval $(call rsvg-convert,$(PNG_TITLE_110),$(DATA)/graphics/%_110.png,Data/graphics/%.svg,--width=110))
$(eval $(call rsvg-convert,$(PNG_TITLE_320),$(DATA)/graphics/%_320.png,Data/graphics/%.svg,--width=320))

# convert to uncompressed 8-bit BMP
$(eval $(call convert-to-bmp-white,$(BMP_TITLE_110) $(BMP_TITLE_320),%.bmp,%.png))

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
	$(Q)$(IM_PREFIX)convert $< $@

####### sounds

ifneq ($(TARGET),ANDROID)
ifneq ($(HAVE_WIN32),y)

WAV_SOUNDS = $(wildcard Data/sound/*.wav)
RAW_SOUNDS = $(patsubst Data/sound/%.wav,$(DATA)/sound/%.raw,$(WAV_SOUNDS))

$(RAW_SOUNDS): $(DATA)/sound/%.raw: Data/sound/%.wav | $(DATA)/sound/dirstamp
	@$(NQ)echo "  SOX     $@"
	$(Q)sox -V1 $< --bits 16 --rate 44100 --channels 1 $@

endif
endif

#######

TEXT_FILES = AUTHORS COPYING NEWS.txt

TEXT_COMPRESSED = $(patsubst %,$(DATA)/%.gz,$(TEXT_FILES))
$(TEXT_COMPRESSED): $(DATA)/%.gz: % | $(DATA)/dirstamp
	@$(NQ)echo "  GZIP    $@"
	$(Q)gzip --best <$< >$@.tmp
	$(Q)mv $@.tmp $@

RESOURCE_FILES = $(TEXT_COMPRESSED)

ifeq ($(TARGET),ANDROID)
RESOURCE_FILES += $(patsubst po/%.po,$(OUT)/po/%.mo,$(wildcard po/*.po))
else

ifeq ($(TARGET_IS_KOBO),y)
RESOURCE_FILES += $(patsubst po/%.po,$(OUT)/po/%.mo,$(wildcard po/*.po))
endif

ifeq ($(USE_WIN32_RESOURCES),y)
RESOURCE_FILES += $(BMP_BITMAPS)
else
RESOURCE_FILES += $(PNG_BITMAPS)
endif

RESOURCE_FILES += $(BMP_ICONS) $(BMP_ICONS_160) 
RESOURCE_FILES += $(BMP_SPLASH_160) $(BMP_SPLASH_80)
RESOURCE_FILES += $(BMP_DIALOG_TITLE) $(BMP_PROGRESS_BORDER)
RESOURCE_FILES += $(BMP_TITLE_320) $(BMP_TITLE_110)
RESOURCE_FILES += $(BMP_LAUNCH_ALL)

RESOURCE_FILES += $(RAW_SOUNDS)

ifeq ($(USE_WIN32_RESOURCES),n)

$(patsubst $(DATA)/icons/%.bmp,$(DATA)/icons2/%.png,$(filter $(DATA)/icons/%.bmp,$(RESOURCE_FILES))): $(DATA)/icons2/%.png: $(DATA)/icons/%.bmp | $(DATA)/icons2/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

$(patsubst $(DATA)/graphics/%.bmp,$(DATA)/graphics2/%.png,$(filter $(DATA)/graphics/%.bmp,$(RESOURCE_FILES))): $(DATA)/graphics2/%.png: $(DATA)/graphics/%.bmp | $(DATA)/graphics2/dirstamp
	$(Q)$(IM_PREFIX)convert $< $@

RESOURCE_FILES := $(patsubst $(DATA)/graphics/%.bmp,$(DATA)/graphics2/%.png,$(RESOURCE_FILES))
RESOURCE_FILES := $(patsubst $(DATA)/icons/%.bmp,$(DATA)/icons2/%.png,$(RESOURCE_FILES))
RESOURCE_FILES := $(patsubst %.bmp,%.png,$(RESOURCE_FILES))
endif

endif

$(OUT)/include/resource.h: src/Resources.hpp | $(OUT)/include/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) -ne 'print "#define $$1 $$2\n" if /^MAKE_RESOURCE\((\w+), (\d+)\);/;' $< >$@.tmp
	$(Q)mv $@.tmp $@

ifeq ($(USE_WIN32_RESOURCES),y)

RESOURCE_TEXT = Data/XCSoar.rc

RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/$(notdir $(RESOURCE_TEXT:.rc=.rsc))
RESOURCE_FILES += $(patsubst po/%.po,$(OUT)/po/%.mo,$(wildcard po/*.po))

$(RESOURCE_BINARY): $(RESOURCE_TEXT) $(OUT)/include/resource.h $(RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

else

$(TARGET_OUTPUT_DIR)/resources.c: $(TARGET_OUTPUT_DIR)/XCSoar.rc $(OUT)/include/resource.h $(RESOURCE_FILES) tools/LinkResources.pl tools/BinToC.pm | $(TARGET_OUTPUT_DIR)/resources/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) tools/LinkResources.pl $< $@

RESOURCES_SOURCES = $(TARGET_OUTPUT_DIR)/resources.c
$(eval $(call link-library,resources,RESOURCES))
RESOURCE_BINARY = $(RESOURCES_BIN)

endif
