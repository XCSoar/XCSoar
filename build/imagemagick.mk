IM_PREFIX :=

# extract alpha channel
%_alpha.png: %.png
	$(Q)$(IM_PREFIX)convert $< -alpha Extract +matte +dither -colors 8 $@

# extract RGB channels
%_rgb.png: %.png
	$(Q)$(IM_PREFIX)convert $< -background white -flatten +matte +dither -colors 64 $@

# tile both images
%_tile.png: %_alpha.png %_rgb.png
	$(Q)$(IM_PREFIX)montage -tile 2x1 -geometry +0+0 $^ -depth 8 $@

# Convert a raster graphic file to 8 bit BMP.
#
# Arguments: OUTFILES, OUTPATTERN, INPATTERN, [PRE_OPTIONS], [POST_OPTIONS]
define convert-to-bmp

$(1): $(2): $(3) | $$(dir $$(firstword $(1)))/dirstamp
	@$$(NQ)echo "  BMP     $$@"
	$$(Q)$$(IM_PREFIX)convert $$< $(4) +dither -compress none -type optimize -colors 256 $(5) bmp3:$$@

endef

# Convert a raster graphic file to 8 bit BMP with white background.
#
# Arguments: OUTFILES, OUTPATTERN, INPATTERN, [PRE_OPTIONS], [POST_OPTIONS]
define convert-to-bmp-white

$(1): $(2): $(3) | $$(dir $$(firstword $(1)))/dirstamp
	@$$(NQ)echo "  BMP     $$@"
	$$(Q)$$(IM_PREFIX)convert $$< $(4) -background white -layers flatten +matte +dither -compress none -type optimize -colors 256 $(5) bmp3:$$@

endef

# Convert a raster graphic file to 8 bit BMP with white background.
#
# Arguments: OUTFILES, OUTPATTERN, INPATTERN, [PRE_OPTIONS], [POST_OPTIONS]
define convert-to-bmp-half

$(1): $(2): $(3) | $$(dir $$(firstword $(1)))/dirstamp
	@$$(NQ)echo "  BMP     $$@"
	@$$(NQ)echo "  BMP     $$(@:1.bmp=2.bmp)"
	$$(Q)$$(IM_PREFIX)convert $$< $(4) -layers flatten +matte +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 bmp3:$$(@:1.bmp=%d.bmp)

endef
