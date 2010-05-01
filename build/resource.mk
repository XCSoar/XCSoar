SVG_ICONS = $(wildcard Data/icons/*.svg)
PNG_ICONS_20 = $(patsubst Data/icons/%.svg,output/data/icons/%-20.png,$(SVG_ICONS))
BMP_ICONS_20 = $(PNG_ICONS_20:.png=.bmp)

$(PNG_ICONS_20): output/data/icons/%-20.png: Data/icons/%.svg | output/data/icons/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg --width=20 $< $@

$(BMP_ICONS_20): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
# extract alpha channel
	$(Q)convert $< -alpha Extract +matte +dither -colors 8 $<.tmp1.png
# extract RGB channels
	$(Q)convert $< -background white -flatten +matte +dither -colors 64 $<.tmp2.png
# tile both images
	$(Q)montage -tile 2x1 -geometry +0+0 $<.tmp1.png $<.tmp2.png -depth 8 $<.tmp3.png
# convert to 8-bit BMP
	$(Q)convert $<.tmp3.png +dither -colors 256 $@

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
