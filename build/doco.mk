.PHONY: manual clean-manual doco

MANUAL_OUTPUT_DIR = $(OUT)/all/manual
DOXYGEN_OUTPUT_DIR = $(OUT)/doc
SOURCES = $(DOC)/manual/*.tex $(DOC)/manual/*.sty
FIGURES = $(DOC)/manual/figures/*.png
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
	map_turnpoint
SVG_ICONS = $(patsubst %,$(DOC)/manual/generated/icons/%.png,$(SVG_ICON_LIST))

manual: $(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf

$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf: $(SOURCES) $(FIGURES) $(SVG_ICONS) $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	cd $(<D) && pdflatex -output-directory $(abspath $(@D)) ./XCSoar-manual.tex
	cd $(<D) && pdflatex -output-directory $(abspath $(@D)) ./XCSoar-manual.tex

$(SVG_ICONS): $(DOC)/manual/generated/icons/%.png: $(topdir)/Data/icons/%.svg | $(DOC)/manual/generated/icons/dirstamp
	rsvg-convert -a -w 32 $< -o $@

doco: FORCE
#	$(topdir)/ex/tools/cloc --exclude-lang=D src > doc/cloc.txt
	rm -rf $(DOXYGEN_OUTPUT_DIR)
	mkdir -p $(DOXYGEN_OUTPUT_DIR)
	cd $(DOC) && doxygen XCSoar.doxyfile
