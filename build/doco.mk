
doco: FORCE
	$(topdir)/ex/tools/cloc --exclude-lang=D src > Doc/cloc.txt
	cd $(DOC) && doxygen XCSoar.doxyfile
