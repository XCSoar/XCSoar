# ==============================================================================
.SUFFIXES : .txt

FILES_MISSING=italian.missing italian.duplicate german.missing german.duplicate test.xcl

# ------------------------------------------------------------------------------
build: $(FILES_MISSING)

%.missing : %.xcl
	perl missing.pl $< > $@

%.duplicate : %.xcl
	perl dup.pl $< > $@

test.xcl: template.xcl test.pl
	perl test.pl template.xcl > test.xcl
