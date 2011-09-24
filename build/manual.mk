MANUAL_OUTPUT_DIR = $(OUT)/manual
GENERATED_DIR = $(OUT)/all/tex
TEX_FILES_EN = $(wildcard $(DOC)/manual/en/*.tex)
TEX_INCLUDES_EN = $(wildcard $(DOC)/manual/*.sty) $(wildcard $(DOC)/manual/en/*.sty)
FIGURES_EN = $(DOC)/manual/en/figures/*.png

SVG_ICON_LIST = \
	alt2_landable_airport \
	alt2_landable_field \
	alt_landable_airport \
	alt_landable_field \
	alt_reachable_airport \
	alt_reachable_field \
	winpilot_landable \
	winpilot_marginal \
	winpilot_reachable \
	map_turnpoint \
	mode_abort \
	mode_climb \
	mode_cruise \
	mode_finalglide \
	map_flag \
	gps_acquiring \
	gps_disconnected
SVG_ICONS = $(patsubst %,$(GENERATED_DIR)/icons/%.pdf,$(SVG_ICON_LIST))

SVG_FIGURES_SHARED = $(wildcard $(DOC)/manual/shared/figures/*.svg)
SVG_FIGURES = $(patsubst $(DOC)/manual/shared/figures/%.svg,$(GENERATED_DIR)/figures/%.pdf,$(SVG_FIGURES_SHARED))

SVG_GRAPHICS_DATA = $(wildcard $(topdir)/Data/graphics/*.svg)
SVG_GRAPHICS = $(patsubst $(topdir)/Data/graphics/%.svg,$(GENERATED_DIR)/graphics/%.pdf,$(SVG_GRAPHICS_DATA))

TEX_INCLUDES_DE =  $(wildcard $(DOC)/manual/*.sty) $(wildcard $(DOC)/manual/de/*.sty)
FIGURES_DE = $(DOC)/manual/de/Bilder/*.png
 
TEX_VARS = TEXINPUTS=".:..:../shared:../../..:"
TEX_FLAGS = -halt-on-error -interaction=nonstopmode
TEX_RUN = cd $(<D) && $(TEX_VARS) pdflatex $(TEX_FLAGS) -output-directory $(abspath $(@D))

.PHONY: manual
manual: \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-developer-manual.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-Handbuch.pdf

$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf: $(DOC)/manual/en/XCSoar-manual.tex \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/dirstamp
ifeq ($(DEBUG),n)
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $(<F)
endif
	$(TEX_RUN) $(<F)

$(MANUAL_OUTPUT_DIR)/XCSoar-developer-manual.pdf: $(DOC)/manual/en/XCSoar-developer-manual.tex $(TEX_INCLUDES_EN) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $(<F)
	$(TEX_RUN) $(<F)

$(MANUAL_OUTPUT_DIR)/XCSoar-Handbuch.pdf: $(DOC)/manual/de/XCSoar-Handbuch.tex $(DOC)/manual/de/Blitzeinstieg.tex $(TEX_INCLUDES_DE) \
	$(FIGURES_DE) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/dirstamp
ifeq ($(DEBUG),n)
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $(<F)
endif
	$(TEX_RUN) $(<F)

$(SVG_ICONS): $(GENERATED_DIR)/icons/%.pdf: $(topdir)/Data/icons/%.svg | $(GENERATED_DIR)/icons/dirstamp
	rsvg-convert -a -f pdf -w 32 $< -o $@

$(SVG_FIGURES): $(GENERATED_DIR)/figures/%.pdf: $(topdir)/doc/manual/shared/figures/%.svg | $(GENERATED_DIR)/figures/dirstamp
	rsvg-convert -a -f pdf $< -o $@

$(SVG_GRAPHICS): $(GENERATED_DIR)/graphics/%.pdf: $(topdir)/Data/graphics/%.svg | $(GENERATED_DIR)/graphics/dirstamp
	rsvg-convert -a -f pdf $< -o $@
