SVG_ICONS = $(wildcard Data/icons/*.svg)
SVG_NOALIAS_ICONS = $(patsubst Data/icons/%.svg,output/data/icons/%.svg,$(SVG_ICONS))
PNG_ICONS = $(patsubst Data/icons/%.svg,output/data/icons/%.png,$(SVG_ICONS))
BMP_ICONS = $(PNG_ICONS:.png=.bmp)
PNG_ICONS_160 = $(patsubst Data/icons/%.svg,output/data/icons/%-160.png,$(SVG_ICONS))
BMP_ICONS_160 = $(PNG_ICONS_160:.png=.bmp)

SVG_SPLASH = Data/graphics/xcsoarswiftsplash.svg
PNG_SPLASH_220 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-220.png,$(SVG_SPLASH))
BMP_SPLASH_220 = $(PNG_SPLASH_220:.png=.bmp)
PNG_SPLASH_160 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-160.png,$(SVG_SPLASH))
BMP_SPLASH_160 = $(PNG_SPLASH_160:.png=.bmp)

SVG_TITLE = Data/graphics/title.svg
PNG_TITLE_110 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-110.png,$(SVG_TITLE))
BMP_TITLE_110 = $(PNG_TITLE_110:.png=.bmp)
PNG_TITLE_320 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-320.png,$(SVG_TITLE))
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
$(PNG_ICONS_160): output/data/icons/%-160.png: output/data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --x-zoom=1.6316 --y-zoom=1.6316 $< -o $@

# extract alpha channel
%-alpha.png: %.png
	$(Q)$(IM_PREFIX)convert $< -alpha Extract +matte +dither -colors 8 $@

# extract RGB channels
%-rgb.png: %.png
	$(Q)$(IM_PREFIX)convert $< -background white -flatten +matte +dither -colors 64 $@

# tile both images
%-tile.png: %-alpha.png %-rgb.png
	$(Q)$(IM_PREFIX)montage -tile 2x1 -geometry +0+0 $^ -depth 8 $@

# convert to uncompressed 8-bit BMP
$(BMP_ICONS): %.bmp: %-tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -compress none -type optimize -colors 256 $@
$(BMP_ICONS_160): %.bmp: %-tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -compress none -type optimize -colors 256 $@

####### splash logo

# render from SVG to PNG
$(PNG_SPLASH_220): output/data/graphics/%-220.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=220 $< -o $@
$(PNG_SPLASH_160): output/data/graphics/%-160.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=160 $< -o $@

# convert to uncompressed 8-bit BMP
$(BMP_SPLASH_220): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@
$(BMP_SPLASH_160): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@

####### version

# render from SVG to PNG
$(PNG_TITLE_110): output/data/graphics/%-110.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=110 $< -o $@

# convert to uncompressed 8-bit BMP
$(BMP_TITLE_110): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@

# render from SVG to PNG
$(PNG_TITLE_320): output/data/graphics/%-320.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=320 $< -o $@

# convert to uncompressed 8-bit BMP
$(BMP_TITLE_320): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $@


####### launcher graphics

SVG_LAUNCH = Data/graphics/launcher.svg
PNG_LAUNCH_224 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-224.png,$(SVG_LAUNCH))
BMP_LAUNCH_FLY_224 = $(PNG_LAUNCH_224:.png=-1.bmp)
BMP_LAUNCH_SIM_224 = $(PNG_LAUNCH_224:.png=-2.bmp)

# render from SVG to PNG
$(PNG_LAUNCH_224): output/data/graphics/%-224.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=224 $< -o $@

# split into two uncompressed 8-bit BMPs (single 'convert' operation)
$(BMP_LAUNCH_FLY_224): %-1.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	@$(NQ)echo "  BMP     $(@:1.bmp=2.bmp)"
	$(Q)$(IM_PREFIX)convert $< -background blue -layers flatten +matte +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 $(@:1.bmp=%d.bmp)
$(BMP_LAUNCH_SIM_224): %-2.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	@$(NQ)echo "  BMP     $(@:1.bmp=2.bmp)"
	$(Q)$(IM_PREFIX)convert $< -background blue -layers flatten +matte +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 $(@:1.bmp=%d.bmp)

RESOURCE_FILES = $(wildcard Data/Dialogs/*.xml) $(wildcard Data/bitmaps/*.bmp)
RESOURCE_FILES += $(BMP_ICONS) $(BMP_ICONS_160) 
RESOURCE_FILES += $(BMP_SPLASH_220) $(BMP_SPLASH_160)
RESOURCE_FILES += $(BMP_TITLE_320) $(BMP_TITLE_110)
RESOURCE_FILES += $(BMP_LAUNCH_FLY_224) $(BMP_LAUNCH_SIM_224)

ifeq ($(HAVE_WIN32),y)

RESOURCE_TEXT = Data/XCSoar.rc
RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/$(notdir $(RESOURCE_TEXT:.rc=.rsc))
RESOURCE_FILES += output/po/de.mo output/po/fr.mo output/po/hu.mo output/po/nl.mo output/po/pl.mo output/po/pt_BR.mo output/po/sk.mo

$(RESOURCE_BINARY): $(RESOURCE_TEXT) $(RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

else

# no resources on UNIX
RESOURCE_BINARY =

endif
