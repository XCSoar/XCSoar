# ==============================================================================
.SUFFIXES : .txt

FILES_MISSING=italian.missing

# ------------------------------------------------------------------------------
build: $(FILES_MISSING)

%.missing : %.txt
	perl ../../../../tools/language/code/missing.pl $< > $@

test.xcl: template.xcl test.pl
	perl test.pl template.xcl > test.xcl
