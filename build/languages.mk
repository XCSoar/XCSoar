# Scan the list of translated languages in po/*.po

PO_FILES = $(wildcard po/*.po)
MO_FILES = $(patsubst po/%.po,$(OUT)/po/%.mo,$(PO_FILES))
LINGUAS = $(patsubst po/%.po,%,$(PO_FILES))
