SVG_ICONS = $(wildcard Data/icons/*.svg)
SVG_NOALIAS_ICONS = $(patsubst Data/icons/%.svg,output/data/icons/%.svg,$(SVG_ICONS))
PNG_ICONS = $(patsubst Data/icons/%.svg,output/data/icons/%.png,$(SVG_ICONS))
BMP_ICONS = $(PNG_ICONS:.png=.bmp)
PNG_ICONS_160 = $(patsubst Data/icons/%.svg,output/data/icons/%_160.png,$(SVG_ICONS))
BMP_ICONS_160 = $(PNG_ICONS_160:.png=.bmp)

SVG_SPLASH = Data/graphics/xcsoarswiftsplash.svg Data/graphics/xcsoarswiftsplash_red.svg
PNG_SPLASH_160 = $(patsubst Data/graphics/%.svg,output/data/graphics/%_160.png,$(SVG_SPLASH))
BMP_SPLASH_160 = $(PNG_SPLASH_160:.png=.bmp)
PNG_SPLASH_80 = $(patsubst Data/graphics/%.svg,output/data/graphics/%_80.png,$(SVG_SPLASH))
BMP_SPLASH_80 = $(PNG_SPLASH_80:.png=.bmp)
PNG_SPLASH_128 = $(patsubst Data/graphics/%.svg,output/data/graphics/%_128.png,$(SVG_SPLASH))
ICNS_SPLASH_128 = $(PNG_SPLASH_128:.png=.icns)

SVG_TITLE = Data/graphics/title.svg
PNG_TITLE_110 = $(patsubst Data/graphics/%.svg,output/data/graphics/%_110.png,$(SVG_TITLE))
BMP_TITLE_110 = $(PNG_TITLE_110:.png=.bmp)
PNG_TITLE_320 = $(patsubst Data/graphics/%.svg,output/data/graphics/%_320.png,$(SVG_TITLE))
BMP_TITLE_320 = $(PNG_TITLE_320:.png=.bmp)


ifeq ($(WINHOST),y)
  IM_PREFIX := im-
else
  IM_PREFIX :=
endif

####### icons

# modify working copy of SVG to improve rendering
$(SVG_NOALIAS_ICONS): output/data/icons/%.svg: build/svg_preprocess.xsl Data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  XSLT    $@"
	$(Q)xsltproc --stringparam DisableAA_Select "MASK_NOAA_" --output $@ $^

# render from SVG to PNG
# Default 100PPI (eg 320x240 4" display)
$(PNG_ICONS): output/data/icons/%.png: output/data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --x-zoom=1.0 --y-zoom=1.0 $< -o $@

#160PPI (eg 640x480 5" display)
$(PNG_ICONS_160): output/data/icons/%_160.png: output/data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --x-zoom=1.6316 --y-zoom=1.6316 $< -o $@

# extract alpha channel
%_alpha.png: %.png
	$(Q)$(IM_PREFIX)convert $< -alpha Extract +matte +dither -colors 8 $@

# extract RGB channels
%_rgb.png: %.png
	$(Q)$(IM_PREFIX)convert $< -background white -flatten +matte +dither -colors 64 $@

# tile both images
%_tile.png: %_alpha.png %_rgb.png
	$(Q)$(IM_PREFIX)montage -tile 2x1 -geometry +0+0 $^ -depth 8 $@

# convert to uncompressed 8-bit BMP
$(BMP_ICONS): %.bmp: %_tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -compress none -type optimize -colors 256 $@
$(BMP_ICONS_160): %.bmp: %_tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -compress none -type optimize -colors 256 $@

####### splash logo

# render from SVG to PNG
$(PNG_SPLASH_160): output/data/graphics/%_160.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=160 $< -o $@
$(PNG_SPLASH_80): output/data/graphics/%_80.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=80 $< -o $@
$(PNG_SPLASH_128): output/data/graphics/%_128.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=128 $< -o $@

# convert to uncompressed 8-bit BMP
$(BMP_SPLASH_160): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@
$(BMP_SPLASH_80): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@

# convert to icns (mac os x icon)
$(ICNS_SPLASH_128): %.icns: %.png
	@$(NQ)echo "  ICNS    $@"
	$(Q)$(IM_PREFIX)png2icns $@ $<

####### version

# render from SVG to PNG
$(PNG_TITLE_110): output/data/graphics/%_110.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=110 $< -o $@

# convert to uncompressed 8-bit BMP
$(BMP_TITLE_110): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@

# render from SVG to PNG
$(PNG_TITLE_320): output/data/graphics/%_320.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=320 $< -o $@

# convert to uncompressed 8-bit BMP
$(BMP_TITLE_320): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@


####### launcher graphics

SVG_LAUNCH = Data/graphics/launcher.svg
PNG_LAUNCH_224 = $(patsubst Data/graphics/%.svg,output/data/graphics/%_224.png,$(SVG_LAUNCH))
BMP_LAUNCH_FLY_224 = $(PNG_LAUNCH_224:.png=_1.bmp)
BMP_LAUNCH_SIM_224 = $(PNG_LAUNCH_224:.png=_2.bmp)

# render from SVG to PNG
$(PNG_LAUNCH_224): output/data/graphics/%_224.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=224 $< -o $@

# split into two uncompressed 8-bit BMPs (single 'convert' operation)
$(BMP_LAUNCH_FLY_224): %_1.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	@$(NQ)echo "  BMP     $(@:1.bmp=2.bmp)"
	$(Q)$(IM_PREFIX)convert $< -background blue -layers flatten +matte +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 $(@:1.bmp=%d.bmp)
$(BMP_LAUNCH_SIM_224): %_2.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	@$(NQ)echo "  BMP     $(@:1.bmp=2.bmp)"
	$(Q)$(IM_PREFIX)convert $< -background blue -layers flatten +matte +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 $(@:1.bmp=%d.bmp)

RESOURCE_FILES = $(wildcard Data/Dialogs/*.xml)
RESOURCE_FILES += $(wildcard Data/Dialogs/Infobox/*.xml)

ifeq ($(TARGET),ANDROID)
RESOURCE_FILES += $(patsubst po/%.po,$(OUT)/po/%.mo,$(wildcard po/*.po))
else
RESOURCE_FILES += $(wildcard Data/bitmaps/*.bmp)
RESOURCE_FILES += $(BMP_ICONS) $(BMP_ICONS_160) 
RESOURCE_FILES += $(BMP_SPLASH_160) $(BMP_SPLASH_80)
RESOURCE_FILES += $(BMP_TITLE_320) $(BMP_TITLE_110)
RESOURCE_FILES += $(BMP_LAUNCH_FLY_224) $(BMP_LAUNCH_SIM_224)
endif

ifeq ($(HAVE_WIN32),y)

RESOURCE_TEXT = Data/XCSoar.rc
RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/$(notdir $(RESOURCE_TEXT:.rc=.rsc))
RESOURCE_FILES += $(patsubst po/%.po,$(OUT)/po/%.mo,$(wildcard po/*.po))

$(RESOURCE_BINARY): $(RESOURCE_TEXT) $(RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

else

# no resources on UNIX
RESOURCE_BINARY =

endif
