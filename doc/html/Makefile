# ==============================================================================
.SUFFIXES : .html .xml .gif .jpg .png .css .js

include Files
# Files to build locally (not found from xml) (also temp files)
FILES_LOCAL=readme.html
# Other temp files to remove
FILES_TMP=dest.tar.gz readme.xml

# ------------------------------------------------------------------------------

SSHUSER= $(USER)

build: depends $(FILES_DEST) $(FILES_LOCAL) dest.tar.gz

$(FILES_DEST): ss/wrap.xsl

depends:
	@perl files.pl > Files

readme.xml: ../../source/XCSoarV4-README.txt
	perl wrap.pl Readme < ../../source/XCSoarV4-README.txt > readme.xml

clean: depends
	@-rm $(FILES_CLEAN) $(FILES_LOCAL) $(FILES_TMP) >/dev/null 2>/dev/null

dest.tar.gz: $(FILES_DEST) $(FILES_LOCAL)
	tar cvzf dest.tar.gz $(FILES_DEST) $(FILES_LOCAL)

install: build
	scp dest.tar.gz $(SSHUSER)@shell.sourceforge.net:/home/groups/x/xc/xcsoar/htdocs/
	ssh $(SSHUSER)@shell.sourceforge.net "cd /home/groups/x/xc/xcsoar/htdocs/; tar xvzf dest.tar.gz; chmod -R g+rwX /home/groups/x/xc/xcsoar/htdocs"

%.html : %.xml
	xsltproc ss/wrap.xsl $< > $@
