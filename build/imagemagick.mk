# Detect if 'magick' is available (ImageMagick 7+)
IM_BIN := $(shell command -v magick 2>/dev/null)
# Fallback to legacy binaries 'convert' and 'montage' if 'magick' is not available (ImageMagick 6)
IM_CONVERT_BIN := $(shell command -v convert 2>/dev/null)
IM_MONTAGE_BIN := $(shell command -v montage 2>/dev/null)
# Define commands based on availability
IM_CONVERT := $(if $(IM_BIN),$(IM_BIN),$(IM_CONVERT_BIN))
IM_MONTAGE := $(if $(IM_BIN),$(IM_BIN) montage,$(IM_MONTAGE_BIN))
# Use -alpha Off for IM7, +matte for IM6
IM_DISABLE_ALPHA := $(if $(MAGICK_BIN),-alpha Off,+matte)

# extract alpha channel
%_alpha.png: %.png
	$(Q)$(IM_CONVERT) $< -alpha Extract $(IM_DISABLE_ALPHA) +dither -colors 8 $@

# extract RGB channels
%_rgb.png: %.png
	$(Q)$(IM_CONVERT) $< -background white -flatten $(IM_DISABLE_ALPHA) +dither -colors 64 $@

# tile both images
%_tile.png: %_alpha.png %_rgb.png
	$(Q)$(IM_MONTAGE) -tile 2x1 -geometry +0+0 $^ -depth 8 $@

# Convert a raster graphic file to 8 bit BMP.
#
# Arguments: OUTFILES, OUTPATTERN, INPATTERN, [PRE_OPTIONS], [POST_OPTIONS]
define convert-to-bmp

$(1): $(2): $(3) | $$(dir $$(firstword $(1)))/dirstamp
	@$$(NQ)echo "  BMP     $$@"
	$$(Q)$$(IM_CONVERT) $$< $(4) +dither -compress none -type optimize -colors 256 $(5) bmp3:$$@

endef

# Convert a raster graphic file to 8 bit BMP with white background.
#
# Arguments: OUTFILES, OUTPATTERN, INPATTERN, [PRE_OPTIONS], [POST_OPTIONS]
define convert-to-bmp-white

$(1): $(2): $(3) | $$(dir $$(firstword $(1)))/dirstamp
	@$$(NQ)echo "  BMP     $$@"
	$$(Q)$$(IM_CONVERT) $$< $(4) -background white -layers flatten $(IM_DISABLE_ALPHA) +dither -compress none -type optimize -colors 256 $(5) bmp3:$$@

endef

# Convert a raster graphic file to 8 bit BMP with white background.
#
# Arguments: OUTFILES, OUTPATTERN, INPATTERN, [PRE_OPTIONS], [POST_OPTIONS]
define convert-to-bmp-half

$(1): $(2): $(3) | $$(dir $$(firstword $(1)))/dirstamp
	@$$(NQ)echo "  BMP     $$@"
	@$$(NQ)echo "  BMP     $$(@:1.bmp=2.bmp)"
	$$(Q)$$(IM_CONVERT) $$< $(4) -layers flatten $(IM_DISABLE_ALPHA) +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 bmp3:$$(@:1.bmp=%d.bmp)

endef
