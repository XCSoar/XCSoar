
doco: FORCE
	tools/cloc --exclude-lang=D src > doc/cloc.txt
	doxygen doc/doxygen/Doxyfile | grep Warning

