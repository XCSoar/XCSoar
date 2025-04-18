# Convert a SVG file to PNG.
#
# Example: $(eval $(call rsvg-convert ... TODO
#
# Arguments: OUTFILES, OUTPATTERN, INPATTERN, [OPTIONS]
define rsvg-convert

$(1): $(2): $(3) | $$(dir $$(firstword $(1)))/dirstamp
	@$$(NQ)echo "  SVG     $$@"
	$$(Q)rsvg-convert $(4) $$< -o $$@

endef
