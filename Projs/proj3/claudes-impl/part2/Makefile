# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
CFLAGS = -Wall -Werror -g -pthread
CC = gcc
SHELL = /bin/bash

port_base ?= 8000
primary_port := $(port_base)
secondary_port := $(shell echo $$(($(primary_port) + 1)))
# Extra test n binds to extra_port_base + n, so no two server runs share a port
# (a port stays unusable for about a minute after the connections on it close)
extra_port_base := $(shell echo $$(($(port_base) + 50)))

.PHONY: all test test-setup test-extra test-all clean clean-tests zip

all: concurrent_open.so

concurrent_open.so: concurrent_open.c
	$(CC) $(CFLAGS) -shared -fpic -o $@ $^ -ldl

test-setup:
	@rm -rf downloaded_files

ifdef testnum
test: test-setup clean-tests concurrent_open.so
	PORT1=$(primary_port) PORT2=$(secondary_port) testius test_cases/tests.json -v -n $(testnum)
else
test: test-setup clean-tests concurrent_open.so
	PORT1=$(primary_port) PORT2=$(secondary_port) testius test_cases/tests.json
endif

# Extra tests, written alongside this implementation rather than shipped with
# the starter code. They live in test_cases/extra/ and run under testius too.
#   make test-extra              Run all extra test cases (ports 8051-8065)
#   make test-extra testnum=10   Run extra test case #10 only
ifdef testnum
test-extra: test-setup concurrent_open.so
	PORT_BASE=$(extra_port_base) testius test_cases/extra/tests_extra.json -v -n $(testnum)
else
test-extra: test-setup concurrent_open.so
	PORT_BASE=$(extra_port_base) testius test_cases/extra/tests_extra.json
endif

# Both suites. testius empties test_results/ when it starts, so only the
# second suite leaves its per-test logs behind.
test-all:
	$(MAKE) test
	$(MAKE) test-extra

clean:
	rm -rf *.o concurrent_open.so __pycache__

clean-tests:
	rm -rf test_results
	rm -rf downloaded_files

zip:
	@echo "ERROR: You cannot run 'make zip' from the part2 subdirectory. Change to the main proj3-code directory and run 'make zip' there."
