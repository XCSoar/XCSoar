SVG_ICONS = $(wildcard Data/icons/*.svg)
PNG_ICONS_20 = $(patsubst Data/icons/%.svg,output/data/icons/%-20.png,$(SVG_ICONS))
BMP_ICONS_20 = $(PNG_ICONS_20:.png=.bmp)

ifeq ($(WINHOST),y)
  IM_PREFIX := im-
else
  IM_PREFIX :=
endif

$(PNG_ICONS_20): output/data/icons/%-20.png: Data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=20 $< -o $@

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
$(BMP_ICONS_20): %.bmp: %-tile.png
	@$(NQ)echo "  BMP     $@"
	$(Q)$(IM_PREFIX)convert $< +dither -colors 256 $@

ifeq ($(HAVE_WIN32),y)

RESOURCE_TEXT = Data/XCSoar.rc
RESOURCE_BINARY = $(TARGET_OUTPUT_DIR)/$(notdir $(RESOURCE_TEXT:.rc=.rsc))
RESOURCE_FILES = $(wildcard Data/Dialogs/*.xml)
RESOURCE_FILES += $(BMP_ICONS_20)

$(RESOURCE_BINARY): $(RESOURCE_TEXT) $(RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/%/../dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

else

# no resources on UNIX
RESOURCE_BINARY =

endif
