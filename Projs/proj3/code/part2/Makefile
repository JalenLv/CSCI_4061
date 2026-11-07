# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
CFLAGS = -Wall -Werror -g -pthread
CC = gcc
SHELL = /bin/bash

port_base ?= 8000
primary_port := $(port_base)
secondary_port := $(shell echo $$(($(primary_port) + 1)))

.PHONY: all test test-setup clean clean-tests zip

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

clean:
	rm -rf *.o concurrent_open.so __pycache__

clean-tests:
	rm -rf test_results
	rm -rf downloaded_files

zip:
	@echo "ERROR: You cannot run 'make zip' from the part2 subdirectory. Change to the main proj3-code directory and run 'make zip' there."
