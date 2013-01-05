MANUAL_OUTPUT_DIR = $(OUT)/manual
TEX_INCLUDES = $(wildcard $(DOC)/manual/*.tex) $(wildcard $(DOC)/manual/*.sty)
TEX_FILES_EN = $(wildcard $(DOC)/manual/en/*.tex)
TEX_INCLUDES_EN = $(TEX_INCLUDES) $(wildcard $(DOC)/manual/en/*.sty)
FIGURES_EN = $(DOC)/manual/en/figures/*.png $(DOC)/manual/en/figures/*.pdf

SVG_ICON_LIST = \
	alt2_landable_airport \
	alt2_landable_field \
	alt2_marginal_airport \
	alt2_marginal_field \
	alt_landable_airport \
	alt_landable_field \
	alt_marginal_airport \
	alt_marginal_field \
	alt_reachable_airport \
	alt_reachable_field \
	winpilot_landable \
	winpilot_marginal \
	winpilot_reachable \
	map_turnpoint \
	map_mountain_top \
	map_obstacle \
	map_pass \
	map_power_plant \
	map_tower \
	map_tunnel \
	map_weather_station \
	mode_abort \
	mode_climb \
	mode_cruise \
	mode_finalglide \
	map_flag \
	gps_acquiring \
	gps_disconnected
SVG_ICONS = $(patsubst %,$(MANUAL_OUTPUT_DIR)/icons/%.pdf,$(SVG_ICON_LIST))

SVG_FIGURES_SHARED = $(wildcard $(DOC)/manual/figures/*.svg)
SVG_FIGURES = $(patsubst $(DOC)/manual/figures/%.svg,$(MANUAL_OUTPUT_DIR)/figures/%.pdf,$(SVG_FIGURES_SHARED))

SVG_GRAPHICS_DATA = $(wildcard $(topdir)/Data/graphics/*.svg)
SVG_GRAPHICS = $(patsubst $(topdir)/Data/graphics/%.svg,$(MANUAL_OUTPUT_DIR)/graphics/%.pdf,$(SVG_GRAPHICS_DATA))

TEX_INCLUDES_BLITZ_DE =  $(TEX_INCLUDES) $(wildcard $(DOC)/manual/de/Blitz/*.sty)
FIGURES_BLITZ_DE = $(DOC)/manual/de/Blitz/Bilder/*.png
 
TEX_INCLUDES_FR =  $(TEX_INCLUDES) $(wildcard $(DOC)/manual/fr/*.sty)
#FIGURES_FR = $(DOC)/manual/fr/images/*.png

TEX_VARS = TEXINPUTS="$(<D):$(DOC)/manual:$(MANUAL_OUTPUT_DIR):.:"
TEX_FLAGS = -halt-on-error -interaction=nonstopmode
TEX_RUN = $(TEX_VARS) pdflatex $(TEX_FLAGS) -output-directory $(@D)

.PHONY: manual
manual: \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-developer-manual.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-Prise-en-main.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-Blitzeinstieg.pdf

# Generate a redistributable ZIP file that allows manual editors
# without the full XCSoar development chain to compile the XCSoar
# manual.  It contains all generated files.
manual-dev-dist: $(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev.zip

$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf: $(DOC)/manual/en/XCSoar-manual.tex \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/dirstamp
ifeq ($(DEBUG),n)
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
endif
	$(TEX_RUN) $<

# Generate a HTML version of the manual with latex2html
$(MANUAL_OUTPUT_DIR)/html/index.html: $(DOC)/manual/en/XCSoar-manual.tex \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/html/dirstamp
	$(TEX_VARS) latex2html -dir $(@D) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-developer-manual.pdf: $(DOC)/manual/en/XCSoar-developer-manual.tex $(TEX_INCLUDES_EN) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/dirstamp
ifeq ($(DEBUG),n)
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
endif
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-Blitzeinstieg.pdf: $(DOC)/manual/de/Blitz/XCSoar-Blitzeinstieg.tex $(DOC)/manual/de/Blitz/Blitzeinstieg.tex \
$(TEX_INCLUDES_BLITZ_DE) $(FIGURES_BLITZ_DE) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/dirstamp
ifeq ($(DEBUG),n)
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
endif
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-Prise-en-main.pdf: $(DOC)/manual/fr/XCSoar-Prise-en-main.tex $(TEX_INCLUDES_FR) \
	$(FIGURES_FR) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) | $(MANUAL_OUTPUT_DIR)/dirstamp
ifeq ($(DEBUG),n)
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
endif
	$(TEX_RUN) $<

$(SVG_ICONS): $(MANUAL_OUTPUT_DIR)/icons/%.pdf: $(topdir)/Data/icons/%.svg | $(MANUAL_OUTPUT_DIR)/icons/dirstamp
	rsvg-convert -a -f pdf -w 32 $< -o $@

$(SVG_FIGURES): $(MANUAL_OUTPUT_DIR)/figures/%.pdf: $(topdir)/doc/manual/figures/%.svg | $(MANUAL_OUTPUT_DIR)/figures/dirstamp
	rsvg-convert -a -f pdf $< -o $@

$(SVG_GRAPHICS): $(MANUAL_OUTPUT_DIR)/graphics/%.pdf: $(topdir)/Data/graphics/%.svg | $(MANUAL_OUTPUT_DIR)/graphics/dirstamp
	rsvg-convert -a -f pdf $< -o $@

$(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev.zip: T=$(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev
$(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev.zip: VERSION.txt \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS)
	rm -rf $(T)
	mkdir $(T) $(T)/figures
	echo $(GIT_COMMIT_ID) >$(T)/git.txt
	cp VERSION.txt $(TEX_FILES_EN) $(TEX_INCLUDES_EN) $(T)/
	cp $(FIGURES_EN) $(SVG_FIGURES) $(T)/figures
	cp -r $(MANUAL_OUTPUT_DIR)/graphics $(MANUAL_OUTPUT_DIR)/icons $(T)/
	cd $(@D) && zip -r XCSoar-manual-dev.zip XCSoar-manual-dev
