DOXYGEN_OUTPUT_DIR = $(OUT)/doc

.PHONY: doco
doco: FORCE
#	$(topdir)/ex/tools/cloc --exclude-lang=D src > doc/cloc.txt
	rm -rf $(DOXYGEN_OUTPUT_DIR)
	mkdir -p $(DOXYGEN_OUTPUT_DIR)
	cd $(DOC) && doxygen XCSoar.doxyfile
