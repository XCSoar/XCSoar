
doco: FORCE
#	$(topdir)/ex/tools/cloc --exclude-lang=D src > doc/cloc.txt
	cd $(DOC) && doxygen XCSoar.doxyfile
