SPHINX_OUTPUT_DIR = $(OUT)/sphinx

SPHINX ?= sphinx-build

SPHINX_OPTIONS =

ifneq ($(V),2)
SPHINX_OPTIONS += -q
endif

.PHONY: sphinx
sphinx: $(SPHINX_OUTPUT_DIR)/index.html
$(SPHINX_OUTPUT_DIR)/index.html: $(DOC)/conf.py $(wildcard $(DOC)/*.rst)
	@$(NQ)echo "  DOC     $@"
	$(Q)$(SPHINX) $(SPHINX_OPTIONS) $(DOC) $(SPHINX_OUTPUT_DIR)
