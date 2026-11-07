# SPDX-FileCopyrightText: 2026 John Kolb <jhkolb@umn.edu>
# SPDX-License-Identifier: GPL-3.0-or-later
CFLAGS = -Wall -Werror -g -pthread
TSAN_CFLAGS = -Wall -Werror -g -pthread -fno-omit-frame-pointer -fsanitize=thread
CC = gcc
SHELL = /bin/bash

port_base ?= 8000
primary_port := $(port_base)
secondary_port := $(shell echo $$(($(primary_port) + 1)))
# Extra test n binds to extra_port_base + n, so no two server runs share a port
# (a port stays unusable for about a minute after the connections on it close)
extra_port_base := $(shell echo $$(($(port_base) + 10)))

.PHONY: all test test-setup test-extra test-all clean clean-tests zip

all: http_server http_server_tsan concurrent_open.so

http_server: http_server.o http.o connection_queue.o
	$(CC) $(CFLAGS) -o $@ $^

http_server_tsan: http_server_tsan.o http_tsan.o connection_queue_tsan.o
	$(CC) $(TSAN_CFLAGS) -o $@ $^

http_server.o: http_server.c
	$(CC) $(CFLAGS) -c $<

http_server_tsan.o: http_server.c
	$(CC) $(TSAN_CFLAGS) -c $< -o $@

http.o: http.c http.h
	$(CC) $(CFLAGS) -c $<

http_tsan.o: http.c http.h
	$(CC) $(TSAN_CFLAGS) -c $< -o $@

connection_queue.o: connection_queue.c connection_queue.h
	$(CC) $(CFLAGS) -c $<

connection_queue_tsan.o: connection_queue.c connection_queue.h
	$(CC) $(TSAN_CFLAGS) -c $< -o $@

concurrent_open.so: concurrent_open.c
	$(CC) $(CFLAGS) -shared -fpic -o $@ $^ -ldl

test-setup:
	@rm -rf downloaded_files

ifdef testnum
test: test-setup http_server http_server_tsan clean-tests concurrent_open.so
	PORT1=$(primary_port) PORT2=$(secondary_port) testius test_cases/tests.json -v -n $(testnum)
else
test: test-setup http_server http_server_tsan clean-tests concurrent_open.so
	PORT1=$(primary_port) PORT2=$(secondary_port) testius test_cases/tests.json
endif

# Extra tests, written alongside this implementation rather than shipped with
# the starter code. They live in test_cases/extra/ and run under testius too.
#   make test-extra              Run all extra test cases (ports 8011-8028)
#   make test-extra testnum=11   Run extra test case #11 only
ifdef testnum
test-extra: test-setup http_server http_server_tsan concurrent_open.so
	PORT_BASE=$(extra_port_base) testius test_cases/extra/tests_extra.json -v -n $(testnum)
else
test-extra: test-setup http_server http_server_tsan concurrent_open.so
	PORT_BASE=$(extra_port_base) testius test_cases/extra/tests_extra.json
endif

# Both suites. testius empties test_results/ when it starts, so only the
# second suite leaves its per-test logs behind.
test-all:
	$(MAKE) test
	$(MAKE) test-extra

clean:
	rm -rf *.o concurrent_open.so http_server http_server_tsan

clean-tests:
	rm -rf test_results
	rm -rf downloaded_files

zip:
	@echo "ERROR: You cannot run 'make zip' from the part1 subdirectory. Change to the main proj3-code directory and run 'make zip' there."
