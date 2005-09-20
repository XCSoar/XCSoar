# ==============================================================================
.SUFFIXES : .html .xml .gif .jpg .png .css .js

include Files
# Files to build locally (not found from xml) (also temp files)
FILES_LOCAL=readme.html input/default.html
# Other temp files to remove
# (XXX Crap name xcixml - XSL should use different entries depending
# automatically)
FILES_TMP=dest.tar.gz readme.xml input/default.xcixml input/default.xml input/tree.tmp

# ------------------------------------------------------------------------------

SSHUSER= $(USER)

build: depends $(FILES_DEST) $(FILES_LOCAL) dest.tar.gz

$(FILES_DEST): ss/wrap.xsl

depends:
	@perl files.pl > Files

readme.xml: ../../source/XCSoarV4-README.txt
	perl wrap.pl Readme < ../../source/XCSoarV4-README.txt > readme.xml

input/default.xcixml: ../../source/Common/Data/Input/default.xci xci2xml.pl
	cp ../../source/Common/Data/Input/default.xci input/default.xci
	perl xci2xml.pl ../../source/Common/Data/Input/default.xci > $@

clean: depends
	@-rm $(FILES_CLEAN) $(FILES_LOCAL) $(FILES_TMP) dest.tar.gz >/dev/null 2>/dev/null

dest.tar.gz: $(FILES_DEST) $(FILES_LOCAL)
	tar cvzf dest.tar.gz $(FILES_DEST) $(FILES_LOCAL)

nightlycvs:
	ssh $(SSHUSER)@shell.sourceforge.net "cvs -d:pserver:anonymous@cvs1:/cvsroot/xcsoar export -Dtomorrow source && tar czvf /home/groups/x/xc/xcsoar/htdocs/xcsoar-nightlyCVS.tar.gz source && rm -Rf source"

install: build nightlycvs
	scp dest.tar.gz $(SSHUSER)@shell.sourceforge.net:/home/groups/x/xc/xcsoar/htdocs/
	ssh $(SSHUSER)@shell.sourceforge.net "cd /home/groups/x/xc/xcsoar/htdocs/; tar xvzf dest.tar.gz; chmod -R g+rwX /home/groups/x/xc/xcsoar/htdocs"

installstyle: 
	scp style.css $(SSHUSER)@shell.sourceforge.net:/home/groups/x/xc/xcsoar/htdocs/

%.xml : %.xcixml ss/xci.xsl ss/xci_tree.xsl
	xsltproc ss/xci_tree.xsl $< > input/tree.tmp
	xsltproc ss/xci.xsl $< > $@

%.html : %.xml ss/wrap.xsl
	xsltproc ss/wrap.xsl $< > $@

