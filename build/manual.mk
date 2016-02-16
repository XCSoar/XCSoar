MANUAL_OUTPUT_DIR = $(OUT)/manual
TEX_INCLUDES = $(wildcard $(DOC)/manual/*.tex) $(wildcard $(DOC)/manual/*.sty)
TEX_FILES_EN = $(wildcard $(DOC)/manual/en/*.tex)
TEX_INCLUDES_EN = $(wildcard $(DOC)/manual/en/*.sty)
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
	map_bridge \
	mode_abort \
	mode_climb \
	mode_cruise \
	mode_finalglide \
	map_flag \
	gps_acquiring \
	gps_disconnected
SVG_ICONS = $(patsubst %,$(MANUAL_OUTPUT_DIR)/icons/%.pdf,$(SVG_ICON_LIST))

SVG_PO_FILES = $(wildcard $(topdir)/doc/manual/figures/*.po)
SVG_LINGUAS = $(patsubst $(topdir)/doc/manual/figures/%.po,%,$(SVG_PO_FILES))
SVG_TRANSLATABLES_LIST = \
	figure_auto_maccready \
	figure_optimal_cruise \
	figure_speed_to_fly \
	figure_terrain
SVG_TRANSLATE_DE = $(patsubst %,$(MANUAL_OUTPUT_DIR)/figures/%-de.pdf,$(SVG_TRANSLATABLES_LIST))
SVG_TRANSLATE_FR = $(patsubst %,$(MANUAL_OUTPUT_DIR)/figures/%-fr.pdf,$(SVG_TRANSLATABLES_LIST))
SVG_TRANSLATE_PT_BR = $(patsubst %,$(MANUAL_OUTPUT_DIR)/figures/%-pt_BR.pdf,$(SVG_TRANSLATABLES_LIST))

SVG_FIGURES_SHARED = $(wildcard $(DOC)/manual/figures/*.svg)
SVG_FIGURES = $(patsubst $(DOC)/manual/figures/%.svg,$(MANUAL_OUTPUT_DIR)/figures/%.pdf,$(SVG_FIGURES_SHARED))

SVG_GRAPHICS_DATA = $(wildcard $(topdir)/Data/graphics/*.svg)
SVG_GRAPHICS = $(patsubst $(topdir)/Data/graphics/%.svg,$(MANUAL_OUTPUT_DIR)/graphics/%.pdf,$(SVG_GRAPHICS_DATA))

SVG_LOGOS_DATA = $(wildcard $(topdir)/Data/graphics/logo*.svg)
SVG_LOGOS = $(patsubst $(topdir)/Data/graphics/%.svg,$(MANUAL_OUTPUT_DIR)/graphics/%.png,$(SVG_LOGOS_DATA))

TEX_INCLUDES_BLITZ_DE = $(wildcard $(DOC)/manual/de/Blitz/*.sty)
FIGURES_BLITZ_DE = $(wildcard $(DOC)/manual/de/Blitz/figures/*.png)
 
TEX_FILES_DE = $(wildcard $(DOC)/manual/de/*.tex)
TEX_INCLUDES_DE = $(wildcard $(DOC)/manual/de/*.sty)
FIGURES_DE = $(wildcard $(DOC)/manual/de/figures/*.png)
 
TEX_FILES_FR = $(wildcard $(DOC)/manual/fr/*.tex)
TEX_INCLUDES_FR = $(wildcard $(DOC)/manual/fr/*.sty)
FIGURES_FR = $(wildcard $(DOC)/manual/fr/figures/*.png)

TEX_FILES_PL = $(wildcard $(DOC)/manual/pl/*.tex)
TEX_INCLUDES_PL = $(wildcard $(DOC)/manual/pl/*.sty)
FIGURES_PL = $(wildcard $(DOC)/manual/pl/figures/*.png)

TEX_FILES_PT_BR = $(wildcard $(DOC)/manual/pt_BR/*.tex)
TEX_INCLUDES_PT_BR = $(wildcard $(DOC)/manual/pt_BR/*.sty)
FIGURES_PT_BR = $(wildcard $(DOC)/manual/pt_BR/figures/*.png)

TEX_VARS = TEXINPUTS="$(<D):$(DOC)/manual:$(MANUAL_OUTPUT_DIR):.:$(DOC)/manual/en:"
TEX_FLAGS = --interaction=nonstopmode --no-shell-escape --output-format=pdf
TEX_RUN = $(TEX_VARS) lualatex $(TEX_FLAGS) --output-directory=$(@D)
IDX_RUN = makeindex -q 

LATEX2HTML = latex2html
LATEX2HTML_RUN = $(TEX_VARS) L2HINIT_NAME=$(DOC)/manual/latex2html.config $(LATEX2HTML) -local_icons -verbosity 0 -split 3

MANUAL_PDF = \
	$(MANUAL_OUTPUT_DIR)/XCSoar-in-a-flash.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-developer-manual.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-Blitzeinstieg.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual-de.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-Prise-en-main.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual-fr.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual-pl.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-in-a-flash-pt_BR.pdf \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual-pt_BR.pdf

.PHONY: manual
manual: $(MANUAL_PDF)

Handbuch: \
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual-de.pdf

manuel-fr:\
	$(MANUAL_OUTPUT_DIR)/XCSoar-manual-fr.pdf

priseenmain: \
	$(MANUAL_OUTPUT_DIR)/XCSoar-Prise-en-main.pdf

# Generate a redistributable ZIP file that allows manual editors
# without the full XCSoar development chain to compile the XCSoar
# manual.  It contains all generated files.
manual-dev-dist: $(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev.zip


$(MANUAL_OUTPUT_DIR)/XCSoar-in-a-flash.pdf: $(DOC)/manual/en/XCSoar-in-a-flash.tex \
	$(TEX_INCLUDES) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf: $(DOC)/manual/en/XCSoar-manual.tex \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) $(TEX_INCLUDES) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

# Generate a HTML version of the manual with latex2html
$(MANUAL_OUTPUT_DIR)/html/en/index.html: $(DOC)/manual/en/XCSoar-manual.tex \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) $(TEX_INCLUDES) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/html/en/dirstamp
	$(LATEX2HTML_RUN) -dir $(@D) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-developer-manual.pdf: $(DOC)/manual/en/XCSoar-developer-manual.tex $(DOC)/manual/en/tpl_format.tex \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) $(TEX_INCLUDES) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/html/developer/index.html: $(DOC)/manual/en/XCSoar-developer-manual.tex \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) $(TEX_INCLUDES) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/html/developer/dirstamp
	$(LATEX2HTML_RUN) -dir $(@D) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-Blitzeinstieg.pdf: $(DOC)/manual/de/XCSoar-Blitzeinstieg.tex \
	$(TEX_INCLUDES_DE) $(FIGURES_DE) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-manual-de.pdf: $(DOC)/manual/de/XCSoar-manual-de.tex \
	$(TEX_FILES_DE) $(TEX_INCLUDES_DE) $(TEX_INCLUDES) \
	$(FIGURES_DE) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_TRANSLATE_DE) $(SVG_GRAPHICS) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<
	$(IDX_RUN) $(@:.pdf=.idx)
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-Prise-en-main.pdf: $(DOC)/manual/fr/XCSoar-Prise-en-main.tex $(TEX_INCLUDES_FR) $(TEX_INCLUDES) \
	$(FIGURES_FR) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-manual-fr.pdf: $(DOC)/manual/fr/XCSoar-manual-fr.tex \
	$(TEX_FILES_FR) $(TEX_INCLUDES_FR) $(TEX_INCLUDES) \
	$(FIGURES_FR) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_TRANSLATE_FR) $(SVG_GRAPHICS) $(SVG_LOGOS)| $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-manual-pl.pdf: $(DOC)/manual/pl/XCSoar-manual-pl.tex \
	$(TEX_FILES_PL) $(TEX_INCLUDES_PL) $(TEX_INCLUDES) \
	$(FIGURES_PL) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) $(SVG_LOGOS)| $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-in-a-flash-pt_BR.pdf: $(DOC)/manual/pt_BR/XCSoar-in-a-flash-pt_BR.tex \
	$(TEX_INCLUDES) $(SVG_LOGOS) | $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(MANUAL_OUTPUT_DIR)/XCSoar-manual-pt_BR.pdf: $(DOC)/manual/pt_BR/XCSoar-manual-pt_BR.tex \
	$(TEX_FILES_PT_BR) $(TEX_INCLUDES_PT_BR) $(TEX_INCLUDES) \
	$(FIGURES_PT_BR) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_TRANSLATE_PT_BR) $(SVG_GRAPHICS) $(SVG_LOGOS)| $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	$(TEX_RUN) $<
	$(TEX_RUN) $<

$(SVG_ICONS): $(MANUAL_OUTPUT_DIR)/icons/%.pdf: $(topdir)/Data/icons/%.svg | $(MANUAL_OUTPUT_DIR)/icons/dirstamp
	rsvg-convert -a -f pdf -w 32 $< -o $@

# $(call get-translate,<po file>,<target file>,<svg file>)
define svg-translate
$(Q)echo "  GEN     $2"
$(Q)$(PERL) $(topdir)/tools/svgl10n.pl -l $1 -o $2.tmp.svg $3
$(Q)rsvg-convert -a -f pdf $2.tmp.svg -o $2
endef

$(SVG_TRANSLATE_DE): $(MANUAL_OUTPUT_DIR)/figures/%-de.pdf: $(topdir)/doc/manual/figures/%.svg \
	$(topdir)/doc/manual/figures/de.po
	$(call svg-translate,./doc/manual/figures/de.po,$@,$<)

$(SVG_TRANSLATE_FR): $(MANUAL_OUTPUT_DIR)/figures/%-fr.pdf: $(topdir)/doc/manual/figures/%.svg \
	$(topdir)/doc/manual/figures/fr.po
	$(call svg-translate,./doc/manual/figures/fr.po,$@,$<)

$(SVG_TRANSLATE_PT_BR): $(MANUAL_OUTPUT_DIR)/figures/%-pt_BR.pdf: $(topdir)/doc/manual/figures/%.svg \
	$(topdir)/doc/manual/figures/pt_BR.po
	$(call svg-translate,./doc/manual/figures/pt_BR.po,$@,$<)

$(SVG_FIGURES): $(MANUAL_OUTPUT_DIR)/figures/%.pdf: $(topdir)/doc/manual/figures/%.svg | $(MANUAL_OUTPUT_DIR)/figures/dirstamp
	rsvg-convert -a -f pdf $< -o $@

$(MANUAL_OUTPUT_DIR)/figures.pot: $(SVG_FIGURES_SHARED) | $(MANUAL_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  GEN     $@"
	$(Q)$(PERL) $(topdir)/tools/svg2po.pl $^ >$@.tmp
	$(Q)mv $@.tmp $@

update-manual-po: $(MANUAL_OUTPUT_DIR)/figures.pot
	$(Q)for i in $(SVG_PO_FILES); do $(MSGMERGE) -o $$i $$i $(MANUAL_OUTPUT_DIR)/figures.pot; done

$(SVG_GRAPHICS): $(MANUAL_OUTPUT_DIR)/graphics/%.pdf: $(topdir)/Data/graphics/%.svg | $(MANUAL_OUTPUT_DIR)/graphics/dirstamp
	rsvg-convert -a -f pdf $< -o $@

$(SVG_LOGOS): $(MANUAL_OUTPUT_DIR)/graphics/%.png: $(topdir)/Data/graphics/%.svg | $(MANUAL_OUTPUT_DIR)/graphics/dirstamp
	rsvg-convert -a -z 1.5 -f png $< -o $@

$(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev.zip: T=$(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev
$(MANUAL_OUTPUT_DIR)/XCSoar-manual-dev.zip: VERSION.txt \
	$(TEX_FILES_EN) $(TEX_INCLUDES_EN) $(TEX_INCLUDES) \
	$(FIGURES_EN) $(SVG_ICONS) $(SVG_FIGURES) $(SVG_GRAPHICS) $(SVG_LOGOS) \
	$(SVG_TRANSLATE_DE) $(SVG_TRANSLATE_FR) $(SVG_TRANSLATE_PT_BR) \
	$(TEX_FILES_FR) $(TEX_INCLUDES_FR) $(FIGURES_FR)
	rm -rf $(T)
	$(MKDIR) -p $(T)/figures $(T)/en/figures
	echo $(GIT_COMMIT_ID) >$(T)/git.txt
	cp VERSION.txt $(TEX_INCLUDES) $(T)/.
	cp $(MANUAL_OUTPUT_DIR)/figures/*.pdf $(T)/figures/.
	cp -r $(MANUAL_OUTPUT_DIR)/graphics $(MANUAL_OUTPUT_DIR)/icons $(T)/.
	# Incl. the English original 
	cp $(TEX_FILES_EN) $(TEX_INCLUDES_EN) $(T)/en/.
	cp $(FIGURES_EN) $(T)/en/figures/.
	# Incl. the French translation
	$(MKDIR) -p $(T)/fr/figures
	cp $(TEX_FILES_FR) $(TEX_INCLUDES_FR) $(T)/fr/.
	cp $(FIGURES_FR) $(T)/fr/figures/.
	# Incl. the Polish translation
	$(MKDIR) -p $(T)/pl/figures
	cp $(TEX_FILES_PL) $(TEX_INCLUDES_PL) $(T)/pl/.
	##cp $(FIGURES_PL) $(T)/pl/figures/.
	# Incl. the Portuguese (Brazilian) translation
	$(MKDIR) -p $(T)/pt_BR/figures
	cp $(TEX_FILES_PT_BR) $(TEX_INCLUDES_PT_BR) $(T)/pt_BR/.
	##cp $(FIGURES_PT_BR) $(T)/pt_BR/figures/.
	# Incl. both German translation
	$(MKDIR) -p $(T)/de/figures
	cp $(TEX_FILES_DE) $(TEX_INCLUDES_DE) $(T)/de/.
	cp $(FIGURES_DE) $(T)/de/figures/.
	# Create an example bash to generate the manuals
	echo -e "#!/bin/bash\n\n# This is an example how the manuals get generated\n\n$(MKDIR) -p output" > $(T)/generate_manuals.sh
	make manual -ns|grep -v mkdir|grep -v touch|sed s#doc/manual#.#g|sed s#output/manual#output#g >> $(T)/generate_manuals.sh
	chmod +x $(T)/generate_manuals.sh
	# Copy an example bat file to generate the manuals with MikTex on Windows
	cp $(DOC)/manual/generate_manuals.bat $(T)/.
	cd $(@D) && zip -r XCSoar-manual-dev.zip XCSoar-manual-dev

upload-html-manual: $(MANUAL_OUTPUT_DIR)/html/en/index.html $(MANUAL_OUTPUT_DIR)/html/developer/index.html
	rsync -aP --delete-after --chmod=ugo+rX $(MANUAL_OUTPUT_DIR)/html/ max@www.xcsoar.org:/var/www/xcsoar/doc/
