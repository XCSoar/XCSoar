.phony: 

BUILDTESTS=\
	t/01_test_tap.exe

test: .phony $(BUILDTESTS)
	# prove
	perl tools/testall.pl t/*

# TODO generalise
t/01_test_tap.exe: src/01_test_tap.c
	gcc -o $@ $<

clean:
	rm $(BUILDTESTS)

