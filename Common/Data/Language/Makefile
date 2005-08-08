# ==============================================================================
.SUFFIXES : .txt

FILES_MISSING=italian.missing

# ------------------------------------------------------------------------------
build: $(FILES_MISSING)

%.missing : %.txt
	perl ../../../../tools/language/code/missing.pl $< > $@
