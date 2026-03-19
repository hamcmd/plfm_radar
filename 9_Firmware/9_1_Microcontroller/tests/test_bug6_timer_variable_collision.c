/*******************************************************************************
 * test_bug6_timer_variable_collision.c
 *
 * Bug #6: In main.cpp, the lock-check timer uses `last_check` (line 1981)
 * and the temperature timer uses `last_check1` (line 2005).  But at line 2038,
 * the temperature block writes to `last_check` INSTEAD OF `last_check1`.
 *
 * Effect:
 *   - After the first 5s window, temperature reads fire continuously
 *     (because last_check1 is never updated).
 *   - The lock-check timer gets reset by the temperature block's write to
 *     last_check, corrupting its timing.
 *
 * Test strategy:
 *   Simulate the two timer blocks from the main loop and show:
 *   1. After both blocks fire once, only last_check is updated (both blocks
 *      write to it), while last_check1 stays at 0.
 *   2. On the next iteration, the temperature block fires immediately
 *      (because last_check1 hasn't changed), while the lock check is delayed.
 ******************************************************************************/
#include "stm32_hal_mock.h"
#include <assert.h>
#include <stdio.h>

/*
 * Extracted from main.cpp lines 1980-2039.
 * We reproduce the exact variable declarations and timer logic.
 */

/* Counters to track how many times each block fires */
static int lock_check_fired = 0;
static int temp_check_fired = 0;

/*
 * Simulates one iteration of the main loop timer blocks.
 * Uses the EXACT code pattern from main.cpp — including the bug.
 */
static void main_loop_iteration(uint32_t *last_check_p, uint32_t *last_check1_p)
{
    /* ---- Lock check block (lines 1981-1999) ---- */
    if (HAL_GetTick() - *last_check_p > 5000) {
        /* Would call ADF4382A_CheckLockStatus here */
        lock_check_fired++;
        *last_check_p = HAL_GetTick();   /* line 1998: correct */
    }

    /* ---- Temperature check block (lines 2005-2039) ---- */
    if (HAL_GetTick() - *last_check1_p > 5000) {
        /* Would read temperature sensors here */
        temp_check_fired++;

        /* BUG: line 2038 writes to last_check instead of last_check1 */
        *last_check_p = HAL_GetTick();   /* THE BUG */
        /* Correct code would be: *last_check1_p = HAL_GetTick(); */
    }
}

int main(void)
{
    uint32_t last_check = 0;
    uint32_t last_check1 = 0;

    printf("=== Bug #6: Timer variable collision ===\n");

    /* ---- Iteration 1: t=0 — nothing fires (0 - 0 == 0, not > 5000) ---- */
    spy_reset();
    mock_set_tick(0);
    lock_check_fired = 0;
    temp_check_fired = 0;

    main_loop_iteration(&last_check, &last_check1);
    printf("  t=0ms: lock_fired=%d temp_fired=%d\n", lock_check_fired, temp_check_fired);
    assert(lock_check_fired == 0);
    assert(temp_check_fired == 0);
    printf("  PASS: Neither fires at t=0\n");

    /* ---- Iteration 2: t=5001 — both should fire ---- */
    mock_set_tick(5001);
    main_loop_iteration(&last_check, &last_check1);
    printf("  t=5001ms: lock_fired=%d temp_fired=%d\n", lock_check_fired, temp_check_fired);
    assert(lock_check_fired == 1);
    assert(temp_check_fired == 1);
    printf("  PASS: Both fire at t=5001\n");

    /* Check variable state after first fire */
    printf("  After first fire: last_check=%u last_check1=%u\n", last_check, last_check1);
    /* last_check was written by BOTH blocks (lock: 5001, then temp: 5001) */
    assert(last_check == 5001);
    /* BUG: last_check1 was NEVER updated — still 0 */
    assert(last_check1 == 0);
    printf("  PASS: last_check1 is still 0 (BUG confirmed — never written)\n");

    /* ---- Iteration 3: t=5002 — temp fires AGAIN (because last_check1==0) ---- */
    mock_set_tick(5002);
    main_loop_iteration(&last_check, &last_check1);
    printf("  t=5002ms: lock_fired=%d temp_fired=%d\n", lock_check_fired, temp_check_fired);
    assert(lock_check_fired == 1);  /* Did NOT fire — 5002-5001=1, not >5000 */
    assert(temp_check_fired == 2);  /* FIRED AGAIN — 5002-0=5002, >5000 */
    printf("  PASS: Temperature fired again immediately (continuous polling bug)\n");

    /* ---- Iteration 4: t=5003 — temp fires AGAIN ---- */
    mock_set_tick(5003);
    main_loop_iteration(&last_check, &last_check1);
    printf("  t=5003ms: lock_fired=%d temp_fired=%d\n", lock_check_fired, temp_check_fired);
    assert(temp_check_fired == 3);  /* Fires every iteration now! */
    printf("  PASS: Temperature fires continuously — last_check1 never advances\n");

    /* ---- Verify last_check is corrupted by temp block ---- */
    /* After temp fires at t=5003, it writes last_check=5003.
     * So lock check's timer just got reset. */
    printf("  last_check=%u (was set by TEMP block, not lock block)\n", last_check);
    assert(last_check == 5003);
    printf("  PASS: Lock check timer corrupted by temperature block\n");

    /* ---- Iteration 5: t=10004 — lock check should fire but timer was reset ---- */
    mock_set_tick(10004);
    main_loop_iteration(&last_check, &last_check1);
    printf("  t=10004ms: lock_fired=%d (expected 2 if timer weren't corrupted)\n",
           lock_check_fired);
    /* With correct code, lock would fire at ~10001. With bug, last_check
     * keeps getting reset by temp block, so it depends on last temp write.
     * last_check was 5003, so 10004-5003=5001 > 5000, lock fires. */
    assert(lock_check_fired == 2);
    /* But temp has been firing EVERY iteration since t=5001, so it fires here too */
    printf("  temp_fired=%d (fires every iteration since t=5001)\n", temp_check_fired);
    assert(temp_check_fired >= 4);
    printf("  PASS: Both timers corrupted — lock delayed, temp runs continuously\n");

    printf("=== Bug #6: ALL TESTS PASSED ===\n\n");
    return 0;
}
