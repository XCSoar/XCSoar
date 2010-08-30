SVG_ICONS = $(wildcard Data/icons/*.svg)
SVG_NOALIAS_ICONS = $(patsubst Data/icons/%.svg,output/data/icons/%.svg,$(SVG_ICONS))
PNG_ICONS_19 = $(patsubst Data/icons/%.svg,output/data/icons/%-19.png,$(SVG_ICONS))
BMP_ICONS_19 = $(PNG_ICONS_19:.png=.bmp)
PNG_ICONS_31 = $(patsubst Data/icons/%.svg,output/data/icons/%-31.png,$(SVG_ICONS))
BMP_ICONS_31 = $(PNG_ICONS_31:.png=.bmp)

SVG_SPLASH = Data/graphics/xcsoarswiftsplash.svg
PNG_SPLASH_220 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-220.png,$(SVG_SPLASH))
BMP_SPLASH_220 = $(PNG_SPLASH_220:.png=.bmp)
PNG_SPLASH_160 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-160.png,$(SVG_SPLASH))
BMP_SPLASH_160 = $(PNG_SPLASH_160:.png=.bmp)

ifeq ($(WINHOST),y)
  IM_PREFIX := im-
else
  IM_PREFIX :=
endif

####### icons

# modify working copy of SVG to improve rendering
$(SVG_NOALIAS_ICONS): output/data/icons/%.svg: build/no_anti_aliasing.xsl Data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  XSLT    $@"
	$(Q)xsltproc --output $@ $^

# render from SVG to PNG
$(PNG_ICONS_19): output/data/icons/%-19.png: output/data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=19 $< -o $@
$(PNG_ICONS_31): output/data/icons/%-31.png: output/data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=31 $< -o $@

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
$(BMP_ICONS_19): %.bmp: %-tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -compress none -type optimize -colors 256 $@
$(BMP_ICONS_31): %.bmp: %-tile.png
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

ifeq ($(HAVE_WIN32),y)

RESOURCE_TEXT = Data/XCSoar.rc
RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/$(notdir $(RESOURCE_TEXT:.rc=.rsc))
RESOURCE_FILES = $(wildcard Data/Dialogs/*.xml)
RESOURCE_FILES += $(BMP_ICONS_19) $(BMP_ICONS_31) 
RESOURCE_FILES += $(BMP_SPLASH_220) $(BMP_SPLASH_160)
RESOURCE_FILES += $(BMP_LAUNCH_FLY_224) $(BMP_LAUNCH_SIM_224)


$(RESOURCE_BINARY): $(RESOURCE_TEXT) $(RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

else

# no resources on UNIX
RESOURCE_BINARY =

endif
