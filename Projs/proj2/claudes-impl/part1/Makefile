# SPDX-FileCopyrightText: John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
CFLAGS = -Wall -Werror -g
CC = gcc $(CFLAGS)
SHELL = /bin/bash
ARCH = $(shell uname -m)

.PHONY: clean test-setup test test-extra-setup test-extra test-all clean-tests zip

swish: swish.o string_vector.o swish_funcs.o swish_funcs_provided_$(ARCH).o
	$(CC) -o $@ $^

swish.o: swish.c
	$(CC) -c $<

string_vector.o: string_vector.c string_vector.h
	$(CC) -c $<

swish_funcs.o: swish_funcs.c swish_funcs.h
	$(CC) -c $<

clean:
	rm -f swish swish.o string_vector.o swish_funcs.o

test-setup:
	@rm -f out.txt

ifdef testnum
test: test-setup swish
	testius test_cases/tests.json -v -n $(testnum)
else
test: swish
	testius test_cases/tests.json
endif

# Extra tests, written alongside this implementation rather than shipped with
# the starter code. They live in test_cases/extra/ and run under the same
# testius harness (and the same valgrind setting) as 'make test' above.
#   make test-extra              Run all extra test cases
#   make test-extra testnum=13   Run extra test case #13 only
test-extra-setup:
	@rm -f out.txt

ifdef testnum
test-extra: test-extra-setup swish
	testius test_cases/extra/tests_extra.json -v -n $(testnum)
else
test-extra: test-extra-setup swish
	testius test_cases/extra/tests_extra.json
endif

# Both suites, one after the other. Note that testius empties test_results/
# when it starts, so only the second suite leaves its per-test logs behind.
# The pass/fail summary for each is printed to the terminal either way.
test-all:
	$(MAKE) test
	$(MAKE) test-extra

# The extra tests only write out.txt and test_results/child_leaks/, which this
# already removes.
clean-tests:
	rm -rf test_results out.txt

zip:
	@echo "ERROR: You cannot run 'make zip' from the part1 subdirectory. Change to the main proj2-code directory and run 'make zip' there."
