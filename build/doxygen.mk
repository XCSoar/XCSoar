DOXYGEN_OUTPUT_DIR = $(OUT)/doc

.PHONY: doco
doco: FORCE
	rm -rf $(DOXYGEN_OUTPUT_DIR)
	$(call create-directory,$(DOXYGEN_OUTPUT_DIR))
	cd $(DOC) && doxygen XCSoar.doxyfile
