# ==============================================================================
.SUFFIXES : .html .xml .gif .jpg .png .css .js

include Files

# ------------------------------------------------------------------------------
build: depends $(FILES_DEST)

$(FILES_DEST): ss/wrap.xsl

depends:
	@perl files.pl > Files

clean: depends
	@-rm $(FILES_CLEAN) dest.tar.gz >/dev/null 2>/dev/null

install: build
	# Note - hard coded readme here now... (temporary)
	cp ../../source/XCSoarV4-README.txt .
	tar cvzf dest.tar.gz $(FILES_DEST) XCSoarV4-README.txt
	scp dest.tar.gz shell.sourceforge.net:/home/groups/x/xc/xcsoar/htdocs/
	ssh shell.sourceforge.net "cd /home/groups/x/xc/xcsoar/htdocs/; tar xvzf dest.tar.gz"

%.html : %.xml
	xsltproc ss/wrap.xsl $< > $@
