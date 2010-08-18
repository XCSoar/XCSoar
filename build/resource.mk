SVG_ICONS = $(wildcard Data/icons/*.svg)
SVG_NOALIAS_ICONS = $(patsubst Data/icons/%.svg,output/data/icons/%.svg,$(SVG_ICONS))
PNG_ICONS_19 = $(patsubst Data/icons/%.svg,output/data/icons/%-19.png,$(SVG_ICONS))
BMP_ICONS_19 = $(PNG_ICONS_19:.png=.bmp)
PNG_ICONS_31 = $(patsubst Data/icons/%.svg,output/data/icons/%-31.png,$(SVG_ICONS))
BMP_ICONS_31 = $(PNG_ICONS_31:.png=.bmp)

ifeq ($(WINHOST),y)
  IM_PREFIX := im-
else
  IM_PREFIX :=
endif

$(SVG_NOALIAS_ICONS): output/data/icons/%.svg: build/no_anti_aliasing.xsl Data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  XSLT    $@"
	$(Q)xsltproc --output $@ $^

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

# convert to 8-bit BMP
$(BMP_ICONS_19): %.bmp: %-tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -colors 256 $@
$(BMP_ICONS_31): %.bmp: %-tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -colors 256 $@

ifeq ($(HAVE_WIN32),y)

RESOURCE_TEXT = Data/XCSoar.rc
RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/$(notdir $(RESOURCE_TEXT:.rc=.rsc))
RESOURCE_FILES = $(wildcard Data/Dialogs/*.xml)
RESOURCE_FILES += $(BMP_ICONS_19) $(BMP_ICONS_31)

$(RESOURCE_BINARY): $(RESOURCE_TEXT) $(RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

else

# no resources on UNIX
RESOURCE_BINARY =

endif
