.PHONY: manual clean-manual doco

MANUAL_OUTPUT_DIR = $(OUT)/all/manual
DOXYGEN_OUTPUT_DIR = $(OUT)/doc
SOURCES = $(DOC)/manual/*.tex $(DOC)/manual/*.sty
FIGURES = $(DOC)/manual/figures/*.png

manual: $(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf

$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf: $(SOURCES) $(FIGURES) $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	cd $(<D) && pdflatex -output-directory $(abspath $(@D)) ./XCSoar-manual.tex
	cd $(<D) && pdflatex -output-directory $(abspath $(@D)) ./XCSoar-manual.tex

doco: FORCE
#	$(topdir)/ex/tools/cloc --exclude-lang=D src > doc/cloc.txt
	rm -rf $(DOXYGEN_OUTPUT_DIR)
	mkdir -p $(DOXYGEN_OUTPUT_DIR)
	cd $(DOC) && doxygen XCSoar.doxyfile
