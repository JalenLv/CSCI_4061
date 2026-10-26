# SPDX-FileCopyrightText: John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
SHELL = /bin/bash
ARCH = $(shell uname -m)

.PHONY: clean test clean-tests zip test-extra test-all

hadamard.cpython-310-$(ARCH)-linux-gnu.so: hadamard_pipe.c hadamard_shmem.c hadamard_threads.c
	pip install -e . -qq --no-build-isolation

ifdef testnum
test: hadamard.cpython-310-$(ARCH)-linux-gnu.so
	testius test_cases/tests.json -v -n $(testnum)
else
test: hadamard.cpython-310-$(ARCH)-linux-gnu.so
	testius test_cases/tests.json
endif

# Extra tests, written alongside this implementation rather than shipped with the
# starter code. They live in test_cases/extra/ and run under the same testius harness.
#   make test-extra              Run all extra test cases
#   make test-extra testnum=12   Run extra test case #12 only
ifdef testnum
test-extra: hadamard.cpython-310-$(ARCH)-linux-gnu.so
	testius test_cases/extra/tests_extra.json -v -n $(testnum)
else
test-extra: hadamard.cpython-310-$(ARCH)-linux-gnu.so
	testius test_cases/extra/tests_extra.json
endif

# Both suites, one after the other. testius empties test_results/ when it starts,
# so only the second suite leaves its per-test logs behind. The pass/fail summary
# for each is printed to the terminal either way.
test-all:
	$(MAKE) test
	$(MAKE) test-extra

clean:
	rm -rf build hadamard.egg-info *.so

clean-tests:
	rm -rf test_results

zip:
	@echo "ERROR: You cannot run 'make zip' from the part2 subdirectory. Change to the main proj2-code directory and run 'make zip' there."
