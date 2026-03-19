################################################################################
# Makefile -- MCU firmware unit test harness for AERIS-10
#
# Builds and runs host-side (macOS) tests for the 8 discovered firmware bugs.
# Uses mock HAL + spy/recording pattern to test real firmware code without
# hardware.
#
# Usage:
#   make          -- build and run all tests
#   make build    -- build all tests without running
#   make test     -- run all tests
#   make clean    -- remove build artifacts
#   make test_bug1  -- build and run just bug1 test
#
# Requirements: Apple Clang or gcc (any C11-capable compiler)
################################################################################

CC       := cc
CFLAGS   := -std=c11 -Wall -Wextra -Wno-unused-parameter -g -O0
# Shim headers come FIRST so they override real headers
INCLUDES := -Ishims -I. -I../9_1_1_C_Cpp_Libraries

# Real source files compiled against mock headers
REAL_SRC := ../9_1_1_C_Cpp_Libraries/adf4382a_manager.c

# Mock/stub object files (shared across tests)
MOCK_SRCS := stm32_hal_mock.c ad_driver_mock.c
MOCK_OBJS := $(MOCK_SRCS:.c=.o)

# Real source compiled as object (for tests that need it)
REAL_OBJ  := adf4382a_manager.o

# Tests that link against real adf4382a_manager.c + mocks
TESTS_WITH_REAL := test_bug1_timed_sync_init_ordering \
                   test_bug3_timed_sync_noop \
                   test_bug4_phase_shift_before_check \
                   test_bug5_fine_phase_gpio_only

# Tests that only need mocks (extracted patterns / static analysis)
TESTS_MOCK_ONLY := test_bug2_ad9523_double_setup \
                   test_bug6_timer_variable_collision \
                   test_bug7_gpio_pin_conflict \
                   test_bug8_uart_commented_out

ALL_TESTS := $(TESTS_WITH_REAL) $(TESTS_MOCK_ONLY)

.PHONY: all build test clean $(addprefix test_,bug1 bug2 bug3 bug4 bug5 bug6 bug7 bug8)

all: build test

build: $(ALL_TESTS)

test: build
	@echo "==============================================="
	@echo "  Running all 8 bug tests..."
	@echo "==============================================="
	@pass=0; fail=0; \
	for t in $(ALL_TESTS); do \
		echo "--- Running $$t ---"; \
		./$$t; \
		if [ $$? -eq 0 ]; then \
			pass=$$((pass + 1)); \
		else \
			fail=$$((fail + 1)); \
			echo "*** FAILED: $$t ***"; \
		fi; \
	done; \
	echo "==============================================="; \
	echo "  Results: $$pass passed, $$fail failed (of $(words $(ALL_TESTS)) total)"; \
	echo "==============================================="; \
	[ $$fail -eq 0 ]

# --- Object file rules ---

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Real source compiled with shim headers
$(REAL_OBJ): $(REAL_SRC) $(MOCK_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $(REAL_SRC) -o $@

# --- Test binary rules ---

# Tests that need real adf4382a_manager.o + mocks
$(TESTS_WITH_REAL): %: %.c $(MOCK_OBJS) $(REAL_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(MOCK_OBJS) $(REAL_OBJ) -o $@

# Tests that only need mocks
test_bug2_ad9523_double_setup: test_bug2_ad9523_double_setup.c $(MOCK_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(MOCK_OBJS) -o $@

test_bug6_timer_variable_collision: test_bug6_timer_variable_collision.c $(MOCK_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(MOCK_OBJS) -o $@

# Bug 7 and 8 don't even need mock objects — pure static analysis
test_bug7_gpio_pin_conflict: test_bug7_gpio_pin_conflict.c
	$(CC) $(CFLAGS) -I. $< -o $@

test_bug8_uart_commented_out: test_bug8_uart_commented_out.c
	$(CC) $(CFLAGS) -I. $< -o $@

# --- Individual test targets ---

test_bug1: test_bug1_timed_sync_init_ordering
	./test_bug1_timed_sync_init_ordering

test_bug2: test_bug2_ad9523_double_setup
	./test_bug2_ad9523_double_setup

test_bug3: test_bug3_timed_sync_noop
	./test_bug3_timed_sync_noop

test_bug4: test_bug4_phase_shift_before_check
	./test_bug4_phase_shift_before_check

test_bug5: test_bug5_fine_phase_gpio_only
	./test_bug5_fine_phase_gpio_only

test_bug6: test_bug6_timer_variable_collision
	./test_bug6_timer_variable_collision

test_bug7: test_bug7_gpio_pin_conflict
	./test_bug7_gpio_pin_conflict

test_bug8: test_bug8_uart_commented_out
	./test_bug8_uart_commented_out

# --- Clean ---

clean:
	rm -f *.o $(ALL_TESTS)
	@echo "Clean complete"
