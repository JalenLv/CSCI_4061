# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
AN = proj3
SHELL = /bin/bash
CWD = $(shell pwd | sed 's/.*\///g')

port_base ?= 8000
py_port_base := $(shell echo $$(($(primary_port) + 3)))

.PHONY: all clean clean-tests zip part1 part2 test-part1 test-part2 test-extra-part1 test-extra-part2 test-extra test-all

all: part1 part2

part1:
	$(MAKE) -C part1

part2:
	$(MAKE) -C part2

test: all clean clean-tests
	$(MAKE) -C part1 test port_base=$(port_base)
	$(MAKE) -C part2 test port_base=$(py_port_base)

# The targets below were added alongside this reference implementation. The
# starter's 'test' above computes py_port_base from primary_port, which this
# Makefile never defines, so Part 2 lands on ports 3 and 4 (fine as root).
# These give every suite its own ports instead, with port_base=8000:
#   test-part1 8000-8001, test-extra-part1 8011-8028,
#   test-part2 8003-8004, test-extra-part2 8051-8065
test-part1: part1
	$(MAKE) -C part1 test port_base=$(port_base)

test-part2: part2
	$(MAKE) -C part2 test port_base=$$(($(port_base) + 3))

test-extra-part1: part1
	$(MAKE) -C part1 test-extra port_base=$(port_base)

test-extra-part2: part2
	$(MAKE) -C part2 test-extra port_base=$(port_base)

test-extra: test-extra-part1 test-extra-part2

# All four suites. testius empties test_results/ at the start of each run, so
# only the last suite of each part leaves its per-test logs behind.
test-all: test-part1 test-extra-part1 test-part2 test-extra-part2

clean:
	$(MAKE) -C part1 clean
	$(MAKE) -C part2 clean

clean-tests:
	$(MAKE) -C part1 clean-tests
	$(MAKE) -C part2 clean-tests

zip: clean clean-tests
	rm -f $(AN)-code.zip
	cd .. && zip "$(CWD)/$(AN)-code.zip" -r "$(CWD)" -x "$(CWD)/*/server_files/*" "$(CWD)/*/test_cases/*"
	@echo Zip created in $(AN)-code.zip
	@if (( $$(stat -c '%s' $(AN)-code.zip) > 10*(2**20) )); then echo "WARNING: $(AN)-code.zip seems REALLY big, check there are no abnormally large test files"; du -h $(AN)-code.zip; fi
	@if (( $$(unzip -t $(AN)-code.zip | wc -l) > 256 )); then echo "WARNING: $(AN)-code.zip has 256 or more files in it which may cause submission problems"; fi
