.phony: 

# XXX better name for test
BUILDTESTS=\
	t/01_test_tap

test: .phony src/01test_tap
	# prove
	perl tools/testall.pl t/*

t/01test_tap: src/01test_tap.c
	gcc -o $@ $<

clean:
	rm $(BUILDTESTS)

