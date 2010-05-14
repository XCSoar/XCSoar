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

$(BMP_ICONS_20): %.bmp: %.png
	@$(NQ)echo "  BMP     $@"
# extract alpha channel
	$(Q)$(IM_PREFIX)convert $< -alpha Extract +matte +dither -colors 8 $<.tmp1.png
# extract RGB channels
	$(Q)$(IM_PREFIX)convert $< -background white -flatten +matte +dither -colors 64 $<.tmp2.png
# tile both images
	$(Q)$(IM_PREFIX)montage -tile 2x1 -geometry +0+0 $<.tmp1.png $<.tmp2.png -depth 8 $<.tmp3.png
# convert to 8-bit BMP
	$(Q)$(IM_PREFIX)convert $<.tmp3.png +dither -colors 256 $@
# remove temporary images
	$(Q)rm $<.tmp1.png
	$(Q)rm $<.tmp2.png
	$(Q)rm $<.tmp3.png

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
