# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
AN = proj3
SHELL = /bin/bash
CWD = $(shell pwd | sed 's/.*\///g')

port_base ?= 8000
py_port_base := $(shell echo $$(($(primary_port) + 3)))

.PHONY: all clean clean-tests zip part1 part2

all: part1 part2

part1:
	$(MAKE) -C part1

part2:
	$(MAKE) -C part2

test: all clean clean-tests
	$(MAKE) -C part1 test port_base=$(port_base)
	$(MAKE) -C part2 test port_base=$(py_port_base)

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
