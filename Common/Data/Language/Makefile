# ==============================================================================
.SUFFIXES : .txt

FILES_MISSING=italian.missing italian.duplicate italian.rev german.missing german.duplicate german.rev test.xcl template.duplicate

# ------------------------------------------------------------------------------
build: $(FILES_MISSING)

%.missing : %.xcl template.xcl
	perl missing.pl $< > $@

%.rev : %.xcl template.xcl
	perl rev.pl $< > $@

%.duplicate : %.xcl template.xcl
	perl dup.pl $< > $@

test.xcl: template.xcl test.pl
	perl test.pl template.xcl > test.xcl
