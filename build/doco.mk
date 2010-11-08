.PHONY: manual clean-manual doco

MANUAL_OUTPUT_DIR = $(OUT)/all/manual
DOXYGEN_OUTPUT_DIR = $(OUT)/doc

manual: $(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf

$(MANUAL_OUTPUT_DIR)/XCSoar-manual.pdf: $(DOC)/manual/XCSoar-manual.tex $(MANUAL_OUTPUT_DIR)/dirstamp
	# run TeX twice to make sure that all references are resolved
	cd $(<D) && pdflatex -output-directory $(abspath $(@D)) $(<F)
	cd $(<D) && pdflatex -output-directory $(abspath $(@D)) $(<F)

doco: FORCE
#	$(topdir)/ex/tools/cloc --exclude-lang=D src > doc/cloc.txt
	rm -rf $(DOXYGEN_OUTPUT_DIR)
	mkdir -p $(DOXYGEN_OUTPUT_DIR)
	cd $(DOC) && doxygen XCSoar.doxyfile
