/*******************************************************************************
 * test_bug1_timed_sync_init_ordering.c
 *
 * Bug #1: ADF4382A_SetupTimedSync() is called at line 175 of
 * adf4382a_manager.c BEFORE manager->initialized is set to true at line 191.
 * SetupTimedSync checks `manager->initialized` and returns -2 (NOT_INIT)
 * when false.  The error is then SWALLOWED — init returns OK anyway.
 *
 * Test strategy:
 *   1. Call ADF4382A_Manager_Init() with SYNC_METHOD_TIMED.
 *   2. Verify it returns OK (the bug is that it succeeds DESPITE sync failure).
 *   3. Verify the spy log contains ZERO SPY_ADF4382_SET_TIMED_SYNC records
 *      (because SetupTimedSync returned early before reaching the driver calls).
 *   4. This proves timed sync is NEVER actually configured.
 ******************************************************************************/
#include "adf4382a_manager.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    ADF4382A_Manager mgr;
    int ret;

    printf("=== Bug #1: Timed sync init ordering ===\n");

    /* ---- Test A: Init returns OK despite sync setup failure ---- */
    spy_reset();
    ret = ADF4382A_Manager_Init(&mgr, SYNC_METHOD_TIMED);

    printf("  Manager_Init returned: %d (expected 0=OK)\n", ret);
    assert(ret == ADF4382A_MANAGER_OK);
    printf("  PASS: Init returned OK\n");

    /* ---- Test B: No timed sync register writes reached the driver ---- */
    int timed_sync_count = spy_count_type(SPY_ADF4382_SET_TIMED_SYNC);
    printf("  SPY_ADF4382_SET_TIMED_SYNC records: %d (expected 0)\n", timed_sync_count);
    assert(timed_sync_count == 0);
    printf("  PASS: Zero timed sync driver calls — sync was NEVER configured\n");

    /* ---- Test C: Manager thinks it's initialized ---- */
    assert(mgr.initialized == true);
    printf("  PASS: manager->initialized == true (despite sync failure)\n");

    /* ---- Test D: After init, calling SetupTimedSync manually WORKS ---- */
    /* This confirms the bug is purely an ordering issue — the function
     * works fine when called AFTER initialized=true */
    spy_reset();
    ret = ADF4382A_SetupTimedSync(&mgr);
    printf("  Post-init SetupTimedSync returned: %d (expected 0)\n", ret);
    assert(ret == ADF4382A_MANAGER_OK);

    timed_sync_count = spy_count_type(SPY_ADF4382_SET_TIMED_SYNC);
    printf("  SPY_ADF4382_SET_TIMED_SYNC records: %d (expected 2 — TX + RX)\n", timed_sync_count);
    assert(timed_sync_count == 2);
    printf("  PASS: Manual post-init call succeeds with 2 driver writes\n");

    /* Cleanup */
    ADF4382A_Manager_Deinit(&mgr);

    printf("=== Bug #1: ALL TESTS PASSED ===\n\n");
    return 0;
}
